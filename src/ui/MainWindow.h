#ifndef CMI_UI_MAINWINDOW_H
#define CMI_UI_MAINWINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QButtonGroup>
#include <QPoint>
#include <vector>

#include "PreviewWidget.h"
#include "CapturesTray.h"
#include "PreviewModal.h"
#include "AboutModal.h"
#include "SegmentedControl.h"
#include "ToggleSwitch.h"

class QComboBox;
class QPushButton;
class QPropertyAnimation;
class QScrollArea;

namespace cmi {

class DeviceManager;
class CaptureDevice;
class ControlPanel;
class PhotoEncoder;
class VideoEncoder;
class ControlSliders;
class TrayIcon;

class ShutterButton : public QAbstractButton {
    Q_OBJECT
public:
    enum class Mode { Photo, Video };

    explicit ShutterButton(QWidget *parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    void setRecording(bool rec);
    bool isRecording() const { return m_recording; }

    QSize sizeHint() const override { return QSize(58, 58); }
    QSize minimumSizeHint() const override { return QSize(58, 58); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Mode m_mode = Mode::Photo;
    bool m_recording = false;
    bool m_hovered = false;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    enum class AppMode { Photo, Video };
    enum class ResizeEdge {
        None,
        Left,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onDevicesChanged();
    void onDeviceSelected(int index);
    void onFrameReady(const uchar *data, int bytes, QSize size, uint32_t pixFmt);
    void onShutterClicked();
    void onCaptureError(const QString &message);
    void onCaptureItemDeleted(const MediaItem &item);

private:
    void buildUi();
    void applyStyle();
    void openDevice(const QString &node);
    void closeDevice();
    void updateTrayState();
    QString captureDir() const;

    void setAppMode(AppMode mode);
    void toggleSidebar(bool show);
    void executePhotoCaptureSequence();
    void doSinglePhotoCapture();
    void startRecording();
    void stopRecording();
    void playShutterSfx();
    void playBeepSfx(bool highPitch);
    ResizeEdge calculateResizeEdge(const QPoint &pos) const;
    void updateCursorForEdge(ResizeEdge edge);

    DeviceManager *m_devMgr = nullptr;
    CaptureDevice *m_capture = nullptr;  // lives on m_captureThread
    QThread m_captureThread;
    QTimer *m_grabTimer = nullptr;
    ControlPanel *m_controls = nullptr;
    VideoEncoder *m_encoder = nullptr;
    TrayIcon *m_tray = nullptr;

    // Window Chrome & TitleBar
    QWidget *m_centralRoot = nullptr;
    QWidget *m_titleBar = nullptr;
    QPushButton *m_closeDot = nullptr;
    QPushButton *m_minDot = nullptr;
    QPushButton *m_maxDot = nullptr;
    QPushButton *m_headerAboutBtn = nullptr;
    QPushButton *m_headerSettingsBtn = nullptr;

    // Viewfinder & Stage
    PreviewWidget *m_preview = nullptr;
    QLabel *m_hudStatus = nullptr;
    QLabel *m_recDot = nullptr;
    QLabel *m_countdownLabel = nullptr;
    QWidget *m_emptyState = nullptr;
    CapturesTray *m_capturesTray = nullptr;
    PreviewModal *m_previewModal = nullptr;
    AboutModal *m_aboutModal = nullptr;

    // Dock controls
    QPushButton *m_modePhotoBtn = nullptr;
    QPushButton *m_modeVideoBtn = nullptr;
    ShutterButton *m_shutterBtn = nullptr;
    QPushButton *m_switchCamBtn = nullptr;
    QPushButton *m_dockSettingsBtn = nullptr;

    // Slide-out Sidebar & Backdrop
    QWidget *m_sidebar = nullptr;
    QWidget *m_sidebarBackdrop = nullptr;
    QComboBox *m_deviceCombo = nullptr;
    QLabel *m_deviceStatus = nullptr;
    SegmentedControl *m_arSegment = nullptr;
    SegmentedControl *m_qualitySegment = nullptr;
    SegmentedControl *m_denoiseSegment = nullptr;
    SegmentedControl *m_timerSegment = nullptr;
    SegmentedControl *m_burstSegment = nullptr;
    QButtonGroup *m_fxGroup = nullptr;
    std::vector<QPushButton *> m_fxButtons;

    ToggleSwitch *m_gridToggle = nullptr;
    ToggleSwitch *m_mirrorToggle = nullptr;
    ToggleSwitch *m_flashToggle = nullptr;
    ToggleSwitch *m_soundToggle = nullptr;
    ToggleSwitch *m_micToggle = nullptr;
    ControlSliders *m_sliders = nullptr;

    // Window Drag & Resize State
    bool m_isTitleDragging = false;
    QPoint m_dragStartPos;
    ResizeEdge m_currentResizeEdge = ResizeEdge::None;
    bool m_isResizing = false;
    QRect m_resizeStartGeometry;
    QPoint m_resizeStartPos;

    // App State
    AppMode m_appMode = AppMode::Photo;
    bool m_recording = false;
    QString m_currentDeviceKey;
    int m_denoiseLevel = 0;
    int m_timerDuration = 0; // seconds (0, 3, 5, 10)
    int m_burstCount = 1;    // 1, 3, 5
    int m_burstRemaining = 0;
    int m_countdownRemaining = 0;
    bool m_flashEnabled = true;
    bool m_soundEnabled = true;
    bool m_micEnabled = true;

    QTimer *m_countdownTimer = nullptr;
    QTimer *m_recordTimer = nullptr;
    QElapsedTimer m_recordElapsed;
};

} // namespace cmi

#endif // CMI_UI_MAINWINDOW_H
