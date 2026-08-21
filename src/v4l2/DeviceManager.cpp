#include "DeviceManager.h"

#include <QDir>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <cstring>

namespace cmi {

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
    refresh();

    m_udev = udev_new();
    if (!m_udev)
        return;

    m_mon = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_mon)
        return;

    udev_monitor_filter_add_match_subsystem_devtype(m_mon, "video4linux", nullptr);
    udev_monitor_enable_receiving(m_mon);

    int fd = udev_monitor_get_fd(m_mon);
    if (fd >= 0) {
        m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated,
                this, &DeviceManager::onUdevEvent);
    }
}

DeviceManager::~DeviceManager()
{
    if (m_mon)
        udev_monitor_unref(m_mon);
    if (m_udev)
        udev_unref(m_udev);
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
    // Drain the monitor queue; any add/remove on video4linux triggers a rescan.
    while (struct udev_device *dev = udev_monitor_receive_device(m_mon)) {
        udev_device_unref(dev);
    }
    refresh();
}

} // namespace cmi
