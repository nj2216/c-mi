#ifndef CMI_V4L2_CONTROLPANEL_H
#define CMI_V4L2_CONTROLPANEL_H

#include <QObject>
#include <QString>
#include <QVector>

#include <linux/videodev2.h>

namespace cmi {

// Thin wrapper around VIDIOC_QUERYCTRL / VIDIOC_G_CTRL / VIDIOC_S_CTRL for a
// device node. Enumerates user controls (brightness, contrast, saturation,
// gamma, sharpness, exposure, white balance, gain, pan/tilt/zoom...) and
// exposes typed values to Qt widgets.
class ControlPanel : public QObject {
    Q_OBJECT
public:
    struct Control {
        uint32_t id = 0;
        QString  name;
        int32_t  minimum = 0;
        int32_t  maximum = 0;
        int32_t  step = 1;
        int32_t  defaultValue = 0;
        int32_t  value = 0;
        bool     isMenu = false;
        bool     isBoolean = false;
        QVector<QPair<int32_t, QString>> menuItems; // for isMenu
    };

    explicit ControlPanel(QObject *parent = nullptr);
    ~ControlPanel() override;

    bool open(const QString &node);
    void close();
    bool isOpen() const { return m_fd >= 0; }

    QVector<Control> controls() const { return m_controls; }
    const Control *controlById(uint32_t id) const;

    bool setValue(uint32_t id, int32_t value);
    bool refreshValue(uint32_t id);
    void refreshAll();

    // Persist/restore via QSettings (INI), keyed by a device-unique name.
    void savePreset(const QString &deviceKey, const QString &presetName) const;
    void loadPreset(const QString &deviceKey, const QString &presetName);
    static QStringList presets(const QString &deviceKey);

signals:
    void controlChanged(uint32_t id, int32_t value);

private:
    void enumerateControls();
    void enumerateMenu(Control &ctrl);

    int m_fd = -1;
    QVector<Control> m_controls;
};

} // namespace cmi

#endif // CMI_V4L2_CONTROLPANEL_H
