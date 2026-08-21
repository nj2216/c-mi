#ifndef CMI_V4L2_CAPTUREDEVICE_H
#define CMI_V4L2_CAPTUREDEVICE_H

#include <QObject>
#include <QSize>
#include <QString>
#include <QVector>

#include <linux/videodev2.h>
#include <stdint.h>

namespace cmi {

// Raw V4L2 capture via mmap: REQBUFS -> QUERYBUF -> mmap -> QBUF/DQBUF.
// Runs on a dedicated std::thread (started via moveToThread semantics of the
// owner); frames are delivered through the frameReady signal carrying a
// zero-copy view into the mmap'd buffer (valid until the signal returns).
class CaptureDevice : public QObject {
    Q_OBJECT
public:
    enum class PreferredFormat { MJPEG, YUYV, H264 };

    explicit CaptureDevice(QObject *parent = nullptr);
    ~CaptureDevice() override;

    bool isOpen() const { return m_fd >= 0; }
    bool isStreaming() const { return m_streaming; }

    QString node() const { return m_node; }
    QSize frameSize() const { return m_frameSize; }
    uint32_t pixelFormat() const { return m_pixFmt; }

    struct FormatInfo {
        uint32_t pixFmt;
        QString  description;
    };
    static QVector<FormatInfo> enumFormats(const QString &node);

public slots:
    bool open(const QString &node);
    void close();
    bool start(const QSize &resolution, uint32_t pixFmt = 0 /*0 = auto*/);
    void stop();
    // Called periodically (timer) by the owning thread: dequeues one buffer.
    void grabFrame();

signals:
    // data points into an mmap'd V4L2 buffer; copy if needed beyond the slot.
    void frameReady(const uchar *data, int bytes, QSize size, uint32_t pixFmt);
    void errorOccurred(const QString &message);

private:
    bool setFormat(const QSize &resolution, uint32_t pixFmt);
    bool requestBuffers();
    void releaseBuffers();
    bool queueBuffer(int index);
    uint32_t pickPixelFormat() const;

    int      m_fd = -1;
    QString  m_node;
    QSize    m_frameSize;
    uint32_t m_pixFmt = 0;
    bool     m_streaming = false;

    struct Buffer {
        void  *start = nullptr;
        size_t length = 0;
    };
    QVector<Buffer> m_buffers;
};

} // namespace cmi

#endif // CMI_V4L2_CAPTUREDEVICE_H
