#ifndef CMI_UI_TRAYICON_H
#define CMI_UI_TRAYICON_H

#include <QSystemTrayIcon>

namespace cmi {

// System tray indicator for camera-in-use and recording state.
class TrayIcon : public QSystemTrayIcon {
    Q_OBJECT
public:
    explicit TrayIcon(QObject *parent = nullptr);

    void setCameraInUse(bool inUse);
    void setRecording(bool recording);

signals:
    void showWindowRequested();
    void quitRequested();

private:
    void updateIcon();

    bool m_inUse = false;
    bool m_recording = false;
};

} // namespace cmi

#endif // CMI_UI_TRAYICON_H
