#include "DeviceManager.h"

#include <QDir>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/videodev2.h>
#include <cstring>

namespace cmi {

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
    refresh();

    m_ueventFd = ::socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (m_ueventFd < 0)
        return;

    sockaddr_nl address{};
    address.nl_family = AF_NETLINK;
    address.nl_pid = static_cast<unsigned int>(::getpid());
    address.nl_groups = 1;
    if (::bind(m_ueventFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        ::close(m_ueventFd);
        m_ueventFd = -1;
        return;
    }

    m_notifier = new QSocketNotifier(m_ueventFd, QSocketNotifier::Read, this);
    if (m_notifier) {
        connect(m_notifier, &QSocketNotifier::activated,
                this, &DeviceManager::onUdevEvent);
    }
}

DeviceManager::~DeviceManager()
{
    if (m_ueventFd >= 0)
        ::close(m_ueventFd);
}

void DeviceManager::refresh()
{
    QVector<DeviceInfo> found;

    QDir devDir(QStringLiteral("/dev"));
    const QStringList nodes = devDir.entryList(
        {QStringLiteral("video*")}, QDir::System, QDir::Name);

    for (const QString &node : nodes) {
        DeviceInfo info;
        if (probeCaptureCaps(devDir.filePath(node), info))
            found.push_back(info);
    }

    // Compare by node list; emit only when the set actually changed.
    bool changed = (found.size() != m_devices.size());
    if (!changed) {
        for (int i = 0; i < found.size(); ++i) {
            if (found[i].node != m_devices[i].node) {
                changed = true;
                break;
            }
        }
    }

    m_devices = found;
    if (changed)
        emit devicesChanged();
}

bool DeviceManager::probeCaptureCaps(const QString &node, DeviceInfo &out)
{
    int fd = ::open(node.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return false;

    struct v4l2_capability cap;
    std::memset(&cap, 0, sizeof(cap));
    bool ok = (::ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0);

    if (ok) {
        quint32 caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS)
                           ? cap.device_caps
                           : cap.capabilities;
        ok = (caps & V4L2_CAP_VIDEO_CAPTURE) != 0;
    }

    if (ok) {
        out.node = node;
        out.name = QString::fromLatin1(reinterpret_cast<const char *>(cap.card)).trimmed();
        if (out.name.isEmpty())
            out.name = node;
        out.busInfo = QString::fromLatin1(reinterpret_cast<const char *>(cap.bus_info));
        out.captureCapable = true;
    }

    ::close(fd);
    return ok;
}

void DeviceManager::onUdevEvent()
{
    char buffer[4096];
    while (true) {
        const ssize_t length = ::recv(m_ueventFd, buffer, sizeof(buffer), MSG_DONTWAIT);
        if (length <= 0)
            break;

        const QByteArray event(buffer, static_cast<int>(length));
        if (event.contains("\0SUBSYSTEM=video4linux\0") ||
            event.startsWith("video4linux@") || event.contains("\0video4linux\0"))
            refresh();
    }
}

} // namespace cmi
