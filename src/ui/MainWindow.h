#ifndef CMI_UI_MAINWINDOW_H
#define CMI_UI_MAINWINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QThread>
#include <QTimer>

class QComboBox;
class QPushButton;

namespace cmi {

class DeviceManager;
class CaptureDevice;
class ControlPanel;
class PhotoEncoder;
class VideoEncoder;
class PreviewWidget;
class ControlSliders;
class TrayIcon;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onDevicesChanged();
    void onDeviceSelected(int index);
    void onFrameReady(const uchar *data, int bytes, QSize size, uint32_t pixFmt);
    void onPhoto();
    void onRecordToggled();
    void onCaptureError(const QString &message);

private:
    void buildUi();
    void applyStyle();
    void openDevice(const QString &node);
    void closeDevice();
    void updateTrayState();
    QString captureDir() const;

    DeviceManager *m_devMgr = nullptr;
    CaptureDevice *m_capture = nullptr;  // lives on m_captureThread
    QThread m_captureThread;
    QTimer *m_grabTimer = nullptr;
    ControlPanel *m_controls = nullptr;
    VideoEncoder *m_encoder = nullptr;
    TrayIcon *m_tray = nullptr;

    PreviewWidget *m_preview = nullptr;
    ControlSliders *m_sliders = nullptr;
    QComboBox *m_deviceCombo = nullptr;
    QComboBox *m_formatCombo = nullptr;
    QPushButton *m_photoBtn = nullptr;
    QPushButton *m_recordBtn = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_onAirLabel = nullptr;  // persistent camera-in-use indicator

    bool m_recording = false;
    QString m_currentDeviceKey;
};

} // namespace cmi

#endif // CMI_UI_MAINWINDOW_H
