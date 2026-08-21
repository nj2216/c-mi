#ifndef CMI_V4L2_DEVICEMANAGER_H
#define CMI_V4L2_DEVICEMANAGER_H

#include <QObject>
#include <QSocketNotifier>
#include <QString>
#include <QVector>

#include <libudev.h>

namespace cmi {

struct DeviceInfo {
    QString node;        // e.g. /dev/video0
    QString name;        // card name from VIDIOC_QUERYCAP
    QString busInfo;     // bus info string (usb-...)
    bool    captureCapable = false;
};

// Enumerates /dev/video* capture-capable nodes and watches udev for
// video4linux add/remove events, emitting signals for hot-reload.
class DeviceManager : public QObject {
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager() override;

    QVector<DeviceInfo> devices() const { return m_devices; }
    void refresh();

signals:
    void devicesChanged();

private slots:
    void onUdevEvent();

private:
    static bool probeCaptureCaps(const QString &node, DeviceInfo &out);

    struct udev       *m_udev = nullptr;
    struct udev_monitor *m_mon = nullptr;
    QSocketNotifier   *m_notifier = nullptr;
    QVector<DeviceInfo> m_devices;
};

} // namespace cmi

#endif // CMI_V4L2_DEVICEMANAGER_H
