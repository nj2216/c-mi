#include "CaptureDevice.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

namespace cmi {

namespace {
constexpr int kBufferCount = 4;

int xioctl(int fd, unsigned long request, void *arg)
{
    int r;
    do {
        r = ::ioctl(fd, request, arg);
    } while (r == -1 && errno == EINTR);
    return r;
}

QString fourccToString(uint32_t f)
{
    char s[5] = {static_cast<char>(f & 0xFF),
                 static_cast<char>((f >> 8) & 0xFF),
                 static_cast<char>((f >> 16) & 0xFF),
                 static_cast<char>((f >> 24) & 0xFF), 0};
    return QString::fromLatin1(s);
}
} // namespace

CaptureDevice::CaptureDevice(QObject *parent) : QObject(parent) {}

CaptureDevice::~CaptureDevice()
{
    close();
}

QVector<CaptureDevice::FormatInfo> CaptureDevice::enumFormats(const QString &node)
{
    QVector<FormatInfo> out;
    int fd = ::open(node.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return out;

    struct v4l2_fmtdesc fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (xioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0) {
        FormatInfo info;
        info.pixFmt = fmt.pixelformat;
        info.description = QString::fromLatin1(
            reinterpret_cast<const char *>(fmt.description));
        info.description += QStringLiteral(" (") + fourccToString(fmt.pixelformat) + QStringLiteral(")");
        out.push_back(info);
        ++fmt.index;
    }
    ::close(fd);
    return out;
}

bool CaptureDevice::open(const QString &node)
{
    if (isOpen())
        close();

    m_fd = ::open(node.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
    if (m_fd < 0) {
        emit errorOccurred(QStringLiteral("Cannot open %1: %2")
                               .arg(node, QString::fromUtf8(strerror(errno))));
        return false;
    }

    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(cap));
    if (xioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0 ||
        !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
        !(cap.capabilities & V4L2_CAP_STREAMING)) {
        emit errorOccurred(QStringLiteral("%1 is not a streaming capture device").arg(node));
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    m_node = node;
    return true;
}

void CaptureDevice::close()
{
    stop();
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_node.clear();
}

uint32_t CaptureDevice::pickPixelFormat() const
{
    // Preference order: MJPEG > YUYV > H264.
    static const uint32_t prefs[] = {
        V4L2_PIX_FMT_MJPEG,
        V4L2_PIX_FMT_YUYV,
        V4L2_PIX_FMT_H264,
    };

    struct v4l2_fmtdesc fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    QVector<uint32_t> supported;
    while (xioctl(m_fd, VIDIOC_ENUM_FMT, &fmt) == 0) {
        supported.push_back(fmt.pixelformat);
        ++fmt.index;
    }

    for (uint32_t p : prefs) {
        if (supported.contains(p))
            return p;
    }
    return supported.isEmpty() ? 0u : supported.first();
}

bool CaptureDevice::setFormat(const QSize &resolution, uint32_t pixFmt)
{
    struct v4l2_format f;
    memset(&f, 0, sizeof(f));
    f.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    f.fmt.pix.width = resolution.width();
    f.fmt.pix.height = resolution.height();
    f.fmt.pix.pixelformat = pixFmt;
    f.fmt.pix.field = V4L2_FIELD_ANY;

    if (xioctl(m_fd, VIDIOC_S_FMT, &f) < 0) {
        emit errorOccurred(QStringLiteral("VIDIOC_S_FMT failed: %1")
                               .arg(QString::fromUtf8(strerror(errno))));
        return false;
    }

    m_frameSize = QSize(f.fmt.pix.width, f.fmt.pix.height);
    m_pixFmt = f.fmt.pix.pixelformat;

    // Request 30 fps; driver may adjust.
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = 30;
    xioctl(m_fd, VIDIOC_S_PARM, &parm);

    return true;
}

bool CaptureDevice::requestBuffers()
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = kBufferCount;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(m_fd, VIDIOC_REQBUFS, &req) < 0) {
        emit errorOccurred(QStringLiteral("VIDIOC_REQBUFS failed: %1")
                               .arg(QString::fromUtf8(strerror(errno))));
        return false;
    }
    if (req.count < 2) {
        emit errorOccurred(QStringLiteral("Insufficient buffer memory"));
        return false;
    }

    m_buffers.resize(req.count);
    for (unsigned i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(m_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            emit errorOccurred(QStringLiteral("VIDIOC_QUERYBUF failed: %1")
                                   .arg(QString::fromUtf8(strerror(errno))));
            releaseBuffers();
            return false;
        }

        m_buffers[i].length = buf.length;
        m_buffers[i].start = ::mmap(nullptr, buf.length,
                                    PROT_READ | PROT_WRITE, MAP_SHARED,
                                    m_fd, buf.m.offset);
        if (m_buffers[i].start == MAP_FAILED) {
            emit errorOccurred(QStringLiteral("mmap failed: %1")
                                   .arg(QString::fromUtf8(strerror(errno))));
            m_buffers[i].start = nullptr;
            releaseBuffers();
            return false;
        }
    }
    return true;
}

void CaptureDevice::releaseBuffers()
{
    for (Buffer &b : m_buffers) {
        if (b.start && b.length)
            ::munmap(b.start, b.length);
        b.start = nullptr;
        b.length = 0;
    }
    m_buffers.clear();

    if (m_fd >= 0) {
        struct v4l2_requestbuffers req;
        memset(&req, 0, sizeof(req));
        req.count = 0;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        xioctl(m_fd, VIDIOC_REQBUFS, &req);
    }
}

bool CaptureDevice::queueBuffer(int index)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;
    return xioctl(m_fd, VIDIOC_QBUF, &buf) == 0;
}

bool CaptureDevice::start(const QSize &resolution, uint32_t pixFmt)
{
    if (!isOpen()) {
        emit errorOccurred(QStringLiteral("Device not open"));
        return false;
    }
    if (m_streaming)
        return true;

    if (pixFmt == 0)
        pixFmt = pickPixelFormat();
    if (pixFmt == 0) {
        emit errorOccurred(QStringLiteral("No supported pixel format found"));
        return false;
    }

    if (!setFormat(resolution, pixFmt))
        return false;
    if (!requestBuffers())
        return false;

    for (int i = 0; i < m_buffers.size(); ++i) {
        if (!queueBuffer(i)) {
            emit errorOccurred(QStringLiteral("VIDIOC_QBUF failed: %1")
                                   .arg(QString::fromUtf8(strerror(errno))));
            releaseBuffers();
            return false;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(m_fd, VIDIOC_STREAMON, &type) < 0) {
        emit errorOccurred(QStringLiteral("VIDIOC_STREAMON failed: %1")
                               .arg(QString::fromUtf8(strerror(errno))));
        releaseBuffers();
        return false;
    }

    m_streaming = true;
    return true;
}

void CaptureDevice::stop()
{
    if (!isOpen())
        return;

    if (m_streaming) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(m_fd, VIDIOC_STREAMOFF, &type);
        m_streaming = false;
    }
    releaseBuffers();
}

void CaptureDevice::grabFrame()
{
    if (!m_streaming || m_fd < 0)
        return;

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(m_fd, VIDIOC_DQBUF, &buf) < 0) {
        if (errno != EAGAIN && errno != EIO)
            emit errorOccurred(QStringLiteral("VIDIOC_DQBUF failed: %1")
                                   .arg(QString::fromUtf8(strerror(errno))));
        return;
    }

    if (buf.index < static_cast<unsigned>(m_buffers.size()) && buf.bytesused > 0) {
        const Buffer &b = m_buffers[buf.index];
        emit frameReady(static_cast<const uchar *>(b.start),
                        static_cast<int>(buf.bytesused),
                        m_frameSize, m_pixFmt);
    }

    // Immediately requeue to keep the pipeline full.
    if (!queueBuffer(buf.index))
        emit errorOccurred(QStringLiteral("VIDIOC_QBUF (requeue) failed"));
}

} // namespace cmi
