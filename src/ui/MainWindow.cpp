#include "MainWindow.h"
#include "SvgIcons.h"

#include "../v4l2/DeviceManager.h"
#include "../v4l2/CaptureDevice.h"
#include "../v4l2/ControlPanel.h"
#include "../encode/PhotoEncoder.h"
#include "../encode/VideoEncoder.h"
#include "PreviewWidget.h"
#include "ControlSliders.h"
#include "TrayIcon.h"

#include <QApplication>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSettings>
#include <QScrollArea>
#include <QStandardPaths>
#include <QPainter>
#include <QPainterPath>
#include <QDesktopServices>
#include <QUrl>

#include <linux/videodev2.h>

namespace cmi {

// ---------------- ShutterButton Implementation ----------------

ShutterButton::ShutterButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setFixedSize(58, 58);
    setCursor(Qt::PointingHandCursor);
}

void ShutterButton::setMode(Mode mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        update();
    }
}

void ShutterButton::setRecording(bool rec)
{
    if (m_recording != rec) {
        m_recording = rec;
        update();
    }
}

void ShutterButton::enterEvent(QEnterEvent *)
{
    m_hovered = true;
    update();
}

void ShutterButton::leaveEvent(QEvent *)
{
    m_hovered = false;
    update();
}

void ShutterButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal w = width();
    const qreal h = height();
    const QPointF center(w / 2.0, h / 2.0);

    // Outer ring (58px, 3px border)
    p.setPen(QPen(QColor(255, 255, 255, m_hovered ? 220 : 90), 3.0));
    p.setBrush(Qt::white);
    p.drawEllipse(center, 26.5, 26.5);

    // Inner shape
    p.setPen(Qt::NoPen);
    if (m_mode == Mode::Photo) {
        // White inner circle (44px) with subtle inset border
        p.setBrush(isDown() ? QColor(228, 228, 232) : Qt::white);
        p.drawEllipse(center, 22.0, 22.0);

        p.setPen(QPen(QColor(0, 0, 0, 25), 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(center, 21.0, 21.0);
    } else {
        // Video mode
        if (m_recording) {
            // Red rounded square (22x22, radius 5)
            p.setBrush(QColor(255, 59, 48)); // #ff3b30
            QRectF sq(center.x() - 11, center.y() - 11, 22, 22);
            p.drawRoundedRect(sq, 5, 5);
        } else {
            // Red inner circle (44px)
            p.setBrush(isDown() ? QColor(220, 40, 32) : QColor(255, 59, 48));
            p.drawEllipse(center, 22.0, 22.0);
        }
    }
}

// ---------------- MainWindow Implementation ----------------

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Frameless window with custom Window Chrome
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    setWindowTitle(QStringLiteral("C~Mi."));

    m_devMgr = new DeviceManager(this);
    m_capture = new CaptureDevice;
    m_capture->moveToThread(&m_captureThread);
    m_captureThread.start();

    m_controls = new ControlPanel(this);
    m_encoder = new VideoEncoder(this);
    m_tray = new TrayIcon(this);

    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setInterval(1000);
    connect(m_countdownTimer, &QTimer::timeout, this, [this] {
        m_countdownRemaining--;
        if (m_countdownRemaining > 0) {
            m_countdownLabel->setText(QString::number(m_countdownRemaining));
            playBeepSfx(false);
        } else {
            m_countdownTimer->stop();
            m_countdownLabel->hide();
            playBeepSfx(true);
            m_burstRemaining = m_burstCount;
            doSinglePhotoCapture();
        }
    });

    m_recordTimer = new QTimer(this);
    m_recordTimer->setInterval(500);
    connect(m_recordTimer, &QTimer::timeout, this, [this] {
        qint64 secs = m_recordElapsed.elapsed() / 1000;
        int m = static_cast<int>(secs / 60);
        int s = static_cast<int>(secs % 60);
        m_hudStatus->setText(QStringLiteral("REC %1:%2")
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0')));
    });

    connect(m_tray, &TrayIcon::showWindowRequested, this, [this] {
        showNormal();
        raise();
        activateWindow();
    });
    connect(m_tray, &TrayIcon::quitRequested, this, [this] {
        m_tray->setVisible(false);
        qApp->quit();
    });

    connect(m_devMgr, &DeviceManager::devicesChanged,
            this, &MainWindow::onDevicesChanged);

    // Capture thread -> GUI thread wiring
    connect(m_capture, &CaptureDevice::frameReady,
            this, &MainWindow::onFrameReady, Qt::QueuedConnection);
    connect(m_capture, &CaptureDevice::errorOccurred,
            this, &MainWindow::onCaptureError, Qt::QueuedConnection);
    connect(m_encoder, &VideoEncoder::errorOccurred,
            this, &MainWindow::onCaptureError);
    connect(m_encoder, &VideoEncoder::recordingStopped,
            this, [this](const QString &path) {
        m_hudStatus->setText(QStringLiteral("Video Saved"));
        QTimer::singleShot(2000, this, [this] {
            if (!m_recording)
                m_hudStatus->setText(m_appMode == AppMode::Video ? QStringLiteral("Video Ready") : QStringLiteral("Ready"));
        });

        // Add to captures tray
        MediaItem item;
        item.filePath = path;
        item.type = MediaItem::Type::Video;
        item.timestamp = QDateTime::currentDateTime();
        item.thumbnail = m_preview->processedLastFrame();
        m_capturesTray->addItem(item);
    });

    m_grabTimer = new QTimer(this);
    m_grabTimer->setInterval(8); // ~120 Hz polling
    connect(m_grabTimer, &QTimer::timeout,
            m_capture, &CaptureDevice::grabFrame, Qt::QueuedConnection);

    buildUi();
    applyStyle();

    // Populate initial device list
    onDevicesChanged();

    // Load recent captures from pictures folder into roll
    m_capturesTray->scanDirectory(captureDir());
}

MainWindow::~MainWindow()
{
    closeDevice();
    m_captureThread.quit();
    m_captureThread.wait();
    delete m_capture;
}

void MainWindow::buildUi()
{
    // The central widget IS the .window root (no outer whitespace margins)
    m_centralRoot = new QWidget(this);
    m_centralRoot->setObjectName(QStringLiteral("windowContainer"));
    m_centralRoot->setMouseTracking(true);

    auto *windowLayout = new QVBoxLayout(m_centralRoot);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->setSpacing(0);

    // 1. Titlebar (height: 42px)
    m_titleBar = new QWidget(m_centralRoot);
    m_titleBar->setObjectName(QStringLiteral("titleBar"));
    m_titleBar->setFixedHeight(42);
    m_titleBar->installEventFilter(this);

    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(16, 0, 16, 0);
    titleLayout->setSpacing(0);

    // Window controls dots (Red, Yellow, Green)
    auto *windowControls = new QWidget(m_titleBar);
    windowControls->setObjectName(QStringLiteral("windowControls"));
    auto *controlsLayout = new QHBoxLayout(windowControls);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(8);

    m_closeDot = new QPushButton(windowControls);
    m_closeDot->setObjectName(QStringLiteral("dotRed"));
    m_closeDot->setFixedSize(12, 12);
    m_closeDot->setCursor(Qt::PointingHandCursor);
    m_closeDot->setToolTip(QStringLiteral("Close"));
    connect(m_closeDot, &QPushButton::clicked, this, &MainWindow::close);

    m_minDot = new QPushButton(windowControls);
    m_minDot->setObjectName(QStringLiteral("dotYellow"));
    m_minDot->setFixedSize(12, 12);
    m_minDot->setCursor(Qt::PointingHandCursor);
    m_minDot->setToolTip(QStringLiteral("Minimize"));
    connect(m_minDot, &QPushButton::clicked, this, &MainWindow::showMinimized);

    m_maxDot = new QPushButton(windowControls);
    m_maxDot->setObjectName(QStringLiteral("dotGreen"));
    m_maxDot->setFixedSize(12, 12);
    m_maxDot->setCursor(Qt::PointingHandCursor);
    m_maxDot->setToolTip(QStringLiteral("Maximize"));
    connect(m_maxDot, &QPushButton::clicked, this, [this] {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });

    controlsLayout->addWidget(m_closeDot);
    controlsLayout->addWidget(m_minDot);
    controlsLayout->addWidget(m_maxDot);
    titleLayout->addWidget(windowControls, 0, Qt::AlignLeft | Qt::AlignVCenter);

    // Centered Title: Camera Pro
    auto *titleLabel = new QLabel(QStringLiteral("C~Mi"), m_titleBar);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    titleLayout->addWidget(titleLabel, 1, Qt::AlignCenter);

    // Right Action: About & Settings
    auto *titleActions = new QWidget(m_titleBar);
    auto *actionsLayout = new QHBoxLayout(titleActions);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(6);

    m_headerAboutBtn = new QPushButton(QStringLiteral("About"), titleActions);
    m_headerAboutBtn->setIcon(icons::infoIcon(QColor(29, 29, 31), 16));
    m_headerAboutBtn->setIconSize(QSize(14, 14));
    m_headerAboutBtn->setObjectName(QStringLiteral("headerSettingsBtn"));
    m_headerAboutBtn->setCursor(Qt::PointingHandCursor);
    connect(m_headerAboutBtn, &QPushButton::clicked, this, [this] {
        m_aboutModal->showModal();
    });

    m_headerSettingsBtn = new QPushButton(QStringLiteral("Settings"), titleActions);
    m_headerSettingsBtn->setIcon(icons::gearIcon(QColor(29, 29, 31), 16));
    m_headerSettingsBtn->setIconSize(QSize(14, 14));
    m_headerSettingsBtn->setObjectName(QStringLiteral("headerSettingsBtn"));
    m_headerSettingsBtn->setCursor(Qt::PointingHandCursor);
    connect(m_headerSettingsBtn, &QPushButton::clicked, this, [this] {
        toggleSidebar(m_sidebar->isHidden());
    });
    actionsLayout->addWidget(m_headerAboutBtn);
    actionsLayout->addWidget(m_headerSettingsBtn);
    titleLayout->addWidget(titleActions, 0, Qt::AlignRight | Qt::AlignVCenter);

    windowLayout->addWidget(m_titleBar);

    // 2. Body Area (Stage + Captures Tray + Dock)
    auto *body = new QWidget(m_centralRoot);
    body->setObjectName(QStringLiteral("bodyArea"));
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // Stage (Viewfinder Stage)
    auto *stage = new QWidget(body);
    stage->setObjectName(QStringLiteral("stage"));
    auto *stageLayout = new QVBoxLayout(stage);
    stageLayout->setContentsMargins(16, 14, 16, 0);
    stageLayout->setSpacing(0);

    // Frame Container holding OpenGL Preview, HUD pill, Countdown overlay, Empty state
    auto *frameContainer = new QWidget(stage);
    frameContainer->setObjectName(QStringLiteral("frameContainer"));
    auto *frameLayout = new QGridLayout(frameContainer);
    frameLayout->setContentsMargins(0, 0, 0, 0);

    m_preview = new PreviewWidget(frameContainer);
    m_preview->setObjectName(QStringLiteral("previewWidget"));
    frameLayout->addWidget(m_preview, 0, 0);

    // Floating HUD Top Pill
    auto *topPill = new QWidget(frameContainer);
    topPill->setObjectName(QStringLiteral("topPill"));
    topPill->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    auto *pillLayout = new QHBoxLayout(topPill);
    pillLayout->setContentsMargins(14, 5, 14, 5);
    pillLayout->setSpacing(8);

    m_recDot = new QLabel(topPill);
    m_recDot->setFixedSize(8, 8);
    m_recDot->setStyleSheet(QStringLiteral("background: #ff3b30; border-radius: 4px;"));
    m_recDot->hide();
    pillLayout->addWidget(m_recDot);

    m_hudStatus = new QLabel(QStringLiteral("Standby"), topPill);
    m_hudStatus->setObjectName(QStringLiteral("hudStatusText"));
    pillLayout->addWidget(m_hudStatus);

    auto *topPillAligner = new QWidget(frameContainer);
    topPillAligner->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    auto *pillAlignLayout = new QVBoxLayout(topPillAligner);
    pillAlignLayout->setContentsMargins(0, 14, 0, 0);
    pillAlignLayout->addWidget(topPill, 0, Qt::AlignHCenter | Qt::AlignTop);
    pillAlignLayout->addStretch();
    frameLayout->addWidget(topPillAligner, 0, 0);

    // Big Countdown Label
    m_countdownLabel = new QLabel(QStringLiteral("3"), frameContainer);
    m_countdownLabel->setObjectName(QStringLiteral("countdownLabel"));
    m_countdownLabel->setAlignment(Qt::AlignCenter);
    m_countdownLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_countdownLabel->hide();
    frameLayout->addWidget(m_countdownLabel, 0, 0, Qt::AlignCenter);

    // Empty State Widget
    m_emptyState = new QWidget(frameContainer);
    m_emptyState->setObjectName(QStringLiteral("emptyState"));
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setContentsMargins(20, 20, 20, 20);
    emptyLayout->setSpacing(12);
    emptyLayout->setAlignment(Qt::AlignCenter);

    auto *glyph = new QLabel(m_emptyState);
    glyph->setPixmap(icons::cameraGlyph(QColor(255, 255, 255, 180), 54).pixmap(54, 54));
    glyph->setObjectName(QStringLiteral("emptyGlyph"));
    glyph->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(glyph);

    auto *msg = new QLabel(QStringLiteral("Enable camera permissions to preview and capture photos & video."), m_emptyState);
    msg->setObjectName(QStringLiteral("emptyMsg"));
    msg->setAlignment(Qt::AlignCenter);
    msg->setWordWrap(true);
    msg->setMaximumWidth(280);
    emptyLayout->addWidget(msg);

    auto *initBtn = new QPushButton(QStringLiteral("Turn On Camera"), m_emptyState);
    initBtn->setObjectName(QStringLiteral("initBtn"));
    initBtn->setCursor(Qt::PointingHandCursor);
    connect(initBtn, &QPushButton::clicked, this, &MainWindow::onDevicesChanged);
    emptyLayout->addWidget(initBtn, 0, Qt::AlignCenter);

    frameLayout->addWidget(m_emptyState, 0, 0);

    stageLayout->addWidget(frameContainer, 1);

    // Horizontal Captures Tray (Apple Photo Booth style)
    m_capturesTray = new CapturesTray(stage);
    connect(m_capturesTray, &CapturesTray::itemClicked, this, [this](const MediaItem &item) {
        m_previewModal->showItem(item);
    });
    connect(m_capturesTray, &CapturesTray::itemDeleteRequested, this, &MainWindow::onCaptureItemDeleted);
    connect(m_capturesTray, &CapturesTray::openFolderRequested, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(captureDir()));
    });
    stageLayout->addWidget(m_capturesTray);

    bodyLayout->addWidget(stage, 1);

    // 3. Bottom Dock Wrap
    auto *dockWrap = new QWidget(body);
    dockWrap->setObjectName(QStringLiteral("dockWrap"));
    auto *dockWrapLayout = new QVBoxLayout(dockWrap);
    dockWrapLayout->setContentsMargins(20, 6, 20, 14);
    dockWrapLayout->setSpacing(6);
    dockWrapLayout->setAlignment(Qt::AlignCenter);

    // Mode Selector (PHOTO | VIDEO)
    auto *modeSelector = new QWidget(dockWrap);
    modeSelector->setObjectName(QStringLiteral("modeSelector"));
    auto *modeLayout = new QHBoxLayout(modeSelector);
    modeLayout->setContentsMargins(0, 0, 0, 0);
    modeLayout->setSpacing(16);
    modeLayout->setAlignment(Qt::AlignCenter);

    m_modePhotoBtn = new QPushButton(QStringLiteral("PHOTO"), modeSelector);
    m_modePhotoBtn->setObjectName(QStringLiteral("modeBtn"));
    m_modePhotoBtn->setCheckable(true);
    m_modePhotoBtn->setChecked(true);
    m_modePhotoBtn->setCursor(Qt::PointingHandCursor);

    m_modeVideoBtn = new QPushButton(QStringLiteral("VIDEO"), modeSelector);
    m_modeVideoBtn->setObjectName(QStringLiteral("modeBtn"));
    m_modeVideoBtn->setCheckable(true);
    m_modeVideoBtn->setCursor(Qt::PointingHandCursor);

    modeLayout->addWidget(m_modePhotoBtn);
    modeLayout->addWidget(m_modeVideoBtn);
    dockWrapLayout->addWidget(modeSelector);

    connect(m_modePhotoBtn, &QPushButton::clicked, this, [this] {
        if (!m_recording) setAppMode(AppMode::Photo);
    });
    connect(m_modeVideoBtn, &QPushButton::clicked, this, [this] {
        if (!m_recording) setAppMode(AppMode::Video);
    });

    // Floating pill dock
    auto *dock = new QWidget(dockWrap);
    dock->setObjectName(QStringLiteral("dock"));
    auto *dockLayout = new QHBoxLayout(dock);
    dockLayout->setContentsMargins(18, 6, 18, 6);
    dockLayout->setSpacing(20);
    dockLayout->setAlignment(Qt::AlignCenter);

    m_switchCamBtn = new QPushButton(dock);
    m_switchCamBtn->setObjectName(QStringLiteral("dockIconBtn"));
    m_switchCamBtn->setIcon(icons::flipIcon(Qt::white, 20));
    m_switchCamBtn->setIconSize(QSize(18, 18));
    m_switchCamBtn->setToolTip(QStringLiteral("Switch camera"));
    m_switchCamBtn->setCursor(Qt::PointingHandCursor);
    connect(m_switchCamBtn, &QPushButton::clicked, this, [this] {
        if (m_deviceCombo && m_deviceCombo->count() > 1) {
            int next = (m_deviceCombo->currentIndex() + 1) % m_deviceCombo->count();
            m_deviceCombo->setCurrentIndex(next);
        }
    });

    m_shutterBtn = new ShutterButton(dock);
    connect(m_shutterBtn, &QAbstractButton::clicked, this, &MainWindow::onShutterClicked);

    m_dockSettingsBtn = new QPushButton(dock);
    m_dockSettingsBtn->setObjectName(QStringLiteral("dockIconBtn"));
    m_dockSettingsBtn->setIcon(icons::gearIcon(Qt::white, 20));
    m_dockSettingsBtn->setIconSize(QSize(18, 18));
    m_dockSettingsBtn->setToolTip(QStringLiteral("Settings"));
    m_dockSettingsBtn->setCursor(Qt::PointingHandCursor);
    connect(m_dockSettingsBtn, &QPushButton::clicked, this, [this] {
        toggleSidebar(m_sidebar->isHidden());
    });

    dockLayout->addWidget(m_switchCamBtn);
    dockLayout->addWidget(m_shutterBtn);
    dockLayout->addWidget(m_dockSettingsBtn);

    dockWrapLayout->addWidget(dock, 0, Qt::AlignCenter);
    bodyLayout->addWidget(dockWrap);

    windowLayout->addWidget(body, 1);

    // 4. Slide-Out Settings Sidebar & Backdrop
    m_sidebarBackdrop = new QWidget(m_centralRoot);
    m_sidebarBackdrop->setObjectName(QStringLiteral("sidebarBackdrop"));
    m_sidebarBackdrop->hide();
    m_sidebarBackdrop->installEventFilter(this);

    m_sidebar = new QWidget(m_centralRoot);
    m_sidebar->setObjectName(QStringLiteral("sidebar"));
    m_sidebar->hide();

    auto *sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(18, 16, 18, 16);
    sidebarLayout->setSpacing(16);

    // Sidebar Header
    auto *sbHeader = new QWidget(m_sidebar);
    auto *sbHeaderLayout = new QHBoxLayout(sbHeader);
    sbHeaderLayout->setContentsMargins(0, 0, 0, 8);
    auto *sbTitle = new QLabel(QStringLiteral("Camera Settings"), sbHeader);
    sbTitle->setObjectName(QStringLiteral("sidebarHeaderTitle"));
    auto *closeSbBtn = new QPushButton(sbHeader);
    closeSbBtn->setIcon(icons::closeIcon(QColor(134, 134, 139), 12));
    closeSbBtn->setIconSize(QSize(10, 10));
    closeSbBtn->setObjectName(QStringLiteral("closeSidebarBtn"));
    closeSbBtn->setCursor(Qt::PointingHandCursor);
    connect(closeSbBtn, &QPushButton::clicked, this, [this] { toggleSidebar(false); });
    sbHeaderLayout->addWidget(sbTitle);
    sbHeaderLayout->addStretch();
    sbHeaderLayout->addWidget(closeSbBtn);
    sidebarLayout->addWidget(sbHeader);

    auto *sidebarScroll = new QScrollArea(m_sidebar);
    sidebarScroll->setWidgetResizable(true);
    sidebarScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebarScroll->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { width: 4px; background: transparent; }"
        "QScrollBar::handle:vertical { background: rgba(0, 0, 0, 0.2); border-radius: 2px; }"
    ));

    auto *sidebarContent = new QWidget(sidebarScroll);
    sidebarContent->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *contentLay = new QVBoxLayout(sidebarContent);
    contentLay->setContentsMargins(0, 0, 0, 0);
    contentLay->setSpacing(16);

    // Source Camera Section
    auto *camSection = new QWidget(sidebarContent);
    auto *camSecLayout = new QVBoxLayout(camSection);
    camSecLayout->setContentsMargins(0, 0, 0, 0);
    camSecLayout->setSpacing(6);
    auto *camLbl = new QLabel(QStringLiteral("SOURCE CAMERA"), camSection);
    camLbl->setObjectName(QStringLiteral("fieldLabel"));
    camSecLayout->addWidget(camLbl);
    m_deviceCombo = new QComboBox(camSection);
    connect(m_deviceCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onDeviceSelected);
    camSecLayout->addWidget(m_deviceCombo);

    m_deviceStatus = new QLabel(QStringLiteral("[ Disconnected ]"), camSection);
    m_deviceStatus->setObjectName(QStringLiteral("deviceStatusPill"));
    camSecLayout->addWidget(m_deviceStatus);
    contentLay->addWidget(camSection);

    // Aspect Ratio Section
    auto *arSection = new QWidget(sidebarContent);
    auto *arLayout = new QVBoxLayout(arSection);
    arLayout->setContentsMargins(0, 0, 0, 0);
    arLayout->setSpacing(6);
    auto *arLbl = new QLabel(QStringLiteral("ASPECT RATIO"), arSection);
    arLbl->setObjectName(QStringLiteral("fieldLabel"));
    arLayout->addWidget(arLbl);
    m_arSegment = new SegmentedControl(arSection);
    m_arSegment->addSegment(QStringLiteral("Fit"), static_cast<int>(AspectRatioMode::Fit));
    m_arSegment->addSegment(QStringLiteral("16:9"), static_cast<int>(AspectRatioMode::Ratio16_9));
    m_arSegment->addSegment(QStringLiteral("4:3"), static_cast<int>(AspectRatioMode::Ratio4_3));
    m_arSegment->addSegment(QStringLiteral("1:1"), static_cast<int>(AspectRatioMode::Ratio1_1));
    connect(m_arSegment, &SegmentedControl::currentDataChanged, this, [this](const QVariant &d) {
        m_preview->setAspectRatioMode(static_cast<AspectRatioMode>(d.toInt()));
    });
    arLayout->addWidget(m_arSegment);
    contentLay->addWidget(arSection);

    // Quality Section
    auto *qSection = new QWidget(sidebarContent);
    auto *qLayout = new QVBoxLayout(qSection);
    qLayout->setContentsMargins(0, 0, 0, 0);
    qLayout->setSpacing(6);
    auto *qLbl = new QLabel(QStringLiteral("QUALITY"), qSection);
    qLbl->setObjectName(QStringLiteral("fieldLabel"));
    qLayout->addWidget(qLbl);
    m_qualitySegment = new SegmentedControl(qSection);
    m_qualitySegment->addSegment(QStringLiteral("480p"), QSize(640, 480));
    m_qualitySegment->addSegment(QStringLiteral("720p"), QSize(1280, 720));
    m_qualitySegment->addSegment(QStringLiteral("1080p"), QSize(1920, 1080));
    m_qualitySegment->setCurrentIndex(1);
    connect(m_qualitySegment, &SegmentedControl::currentDataChanged, this, [this](const QVariant &v) {
        m_preview->setQualityResolution(v.toSize());
    });
    qLayout->addWidget(m_qualitySegment);
    contentLay->addWidget(qSection);

    // Noise Reduction Section
    auto *nrSection = new QWidget(sidebarContent);
    auto *nrLayout = new QVBoxLayout(nrSection);
    nrLayout->setContentsMargins(0, 0, 0, 0);
    nrLayout->setSpacing(6);
    auto *nrLbl = new QLabel(QStringLiteral("NOISE REDUCTION"), nrSection);
    nrLbl->setObjectName(QStringLiteral("fieldLabel"));
    nrLayout->addWidget(nrLbl);
    m_denoiseSegment = new SegmentedControl(nrSection);
    m_denoiseSegment->addSegment(QStringLiteral("Off"), 0);
    m_denoiseSegment->addSegment(QStringLiteral("Low"), 1);
    m_denoiseSegment->addSegment(QStringLiteral("Med"), 2);
    m_denoiseSegment->addSegment(QStringLiteral("High"), 3);

    QSettings settings(QStringLiteral("c-mi"), QStringLiteral("c-mi"));
    int savedDenoise = settings.value(QStringLiteral("denoiseLevel"), 0).toInt();
    m_denoiseLevel = std::clamp(savedDenoise, 0, 3);
    m_denoiseSegment->setCurrentIndex(m_denoiseLevel);
    m_preview->setDenoiseLevel(m_denoiseLevel);

    connect(m_denoiseSegment, &SegmentedControl::currentDataChanged, this, [this](const QVariant &v) {
        m_denoiseLevel = v.toInt();
        m_preview->setDenoiseLevel(m_denoiseLevel);
        QSettings s(QStringLiteral("c-mi"), QStringLiteral("c-mi"));
        s.setValue(QStringLiteral("denoiseLevel"), m_denoiseLevel);
    });
    nrLayout->addWidget(m_denoiseSegment);
    contentLay->addWidget(nrSection);

    // Color Filters Section (3x3 Swatch Grid)
    auto *fxSection = new QWidget(sidebarContent);
    auto *fxLayout = new QVBoxLayout(fxSection);
    fxLayout->setContentsMargins(0, 0, 0, 0);
    fxLayout->setSpacing(6);
    auto *fxLbl = new QLabel(QStringLiteral("COLOR FILTERS"), fxSection);
    fxLbl->setObjectName(QStringLiteral("fieldLabel"));
    fxLayout->addWidget(fxLbl);

    auto *swatchGrid = new QWidget(fxSection);
    auto *gridLay = new QGridLayout(swatchGrid);
    gridLay->setContentsMargins(0, 0, 0, 0);
    gridLay->setSpacing(6);

    m_fxGroup = new QButtonGroup(this);
    m_fxGroup->setExclusive(true);

    struct SwatchDef {
        const char *name;
        ColorFilter filter;
        const char *grad;
    };
    const SwatchDef swatches[] = {
        {"None", ColorFilter::None, "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #c7c7cc, stop:1 #8e8e93)"},
        {"Mono", ColorFilter::Grayscale, "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #6e6e73, stop:1 #1d1d1f)"},
        {"Sepia", ColorFilter::Sepia, "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #c9a06a, stop:1 #8a5a2b)"},
        {"Cool", ColorFilter::Cool, "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #5ac8fa, stop:1 #0071e3)"},
        {"Warm", ColorFilter::Warm, "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ff9f0a, stop:1 #ff375f)"},
        {"Cyber", ColorFilter::Cyber, "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ff007f, stop:1 #00f0ff)"},
        {"Noir", ColorFilter::Noir, "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #000000, stop:1 #434343)"},
        {"Vintage", ColorFilter::Vintage, "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #d4a373, stop:1 #a98467)"},
        {"Invert", ColorFilter::Invert, "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #11998e, stop:1 #38ef7d)"}
    };

    for (int i = 0; i < 9; ++i) {
        auto *btn = new QPushButton(QString::fromLatin1(swatches[i].name), swatchGrid);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(32);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: %1;"
            "  border: 2px solid transparent;"
            "  border-radius: 8px;"
            "  color: #ffffff;"
            "  font-size: 10px;"
            "  font-weight: 700;"
            "}"
            "QPushButton:checked {"
            "  border: 2px solid #0071e3;"
            "}"
        ).arg(QString::fromLatin1(swatches[i].grad)));

        m_fxGroup->addButton(btn, static_cast<int>(swatches[i].filter));
        gridLay->addWidget(btn, i / 3, i % 3);
        if (i == 0) btn->setChecked(true);
    }

    connect(m_fxGroup, &QButtonGroup::idClicked, this, [this](int id) {
        m_preview->setFilter(static_cast<ColorFilter>(id));
    });

    fxLayout->addWidget(swatchGrid);
    contentLay->addWidget(fxSection);

    // Timer & Burst Section
    auto *tbSection = new QWidget(sidebarContent);
    auto *tbLayout = new QVBoxLayout(tbSection);
    tbLayout->setContentsMargins(0, 0, 0, 0);
    tbLayout->setSpacing(6);
    auto *tbLbl = new QLabel(QStringLiteral("TIMER & BURST"), tbSection);
    tbLbl->setObjectName(QStringLiteral("fieldLabel"));
    tbLayout->addWidget(tbLbl);

    m_timerSegment = new SegmentedControl(tbSection);
    m_timerSegment->addSegment(QStringLiteral("Off"), 0);
    m_timerSegment->addSegment(QStringLiteral("3s"), 3);
    m_timerSegment->addSegment(QStringLiteral("5s"), 5);
    m_timerSegment->addSegment(QStringLiteral("10s"), 10);
    connect(m_timerSegment, &SegmentedControl::currentDataChanged, this, [this](const QVariant &v) {
        m_timerDuration = v.toInt();
    });
    tbLayout->addWidget(m_timerSegment);

    m_burstSegment = new SegmentedControl(tbSection);
    m_burstSegment->addSegment(QStringLiteral("1x Shot"), 1);
    m_burstSegment->addSegment(QStringLiteral("3x Burst"), 3);
    m_burstSegment->addSegment(QStringLiteral("5x Burst"), 5);
    connect(m_burstSegment, &SegmentedControl::currentDataChanged, this, [this](const QVariant &v) {
        m_burstCount = v.toInt();
    });
    tbLayout->addWidget(m_burstSegment);
    contentLay->addWidget(tbSection);

    // Toggles Section
    auto *togglesSection = new QWidget(sidebarContent);
    auto *togglesLayout = new QVBoxLayout(togglesSection);
    togglesLayout->setContentsMargins(0, 0, 0, 0);
    togglesLayout->setSpacing(8);
    auto *togLbl = new QLabel(QStringLiteral("TOGGLES"), togglesSection);
    togLbl->setObjectName(QStringLiteral("fieldLabel"));
    togglesLayout->addWidget(togLbl);

    auto addToggleRow = [&](const QString &label, ToggleSwitch *&sw, bool defaultChecked, auto callback) {
        auto *row = new QWidget(togglesSection);
        auto *rowLay = new QHBoxLayout(row);
        rowLay->setContentsMargins(0, 0, 0, 0);
        auto *name = new QLabel(label, row);
        name->setStyleSheet(QStringLiteral("color: #1d1d1f; font-size: 12px; font-weight: 500;"));
        sw = new ToggleSwitch(row);
        sw->setChecked(defaultChecked);
        connect(sw, &QAbstractButton::toggled, this, callback);
        rowLay->addWidget(name);
        rowLay->addStretch();
        rowLay->addWidget(sw);
        togglesLayout->addWidget(row);
    };

    addToggleRow(QStringLiteral("Grid Guide"), m_gridToggle, false, [this](bool on) {
        m_preview->setShowGrid(on);
    });
    addToggleRow(QStringLiteral("Mirror Mode"), m_mirrorToggle, true, [this](bool on) {
        m_preview->setMirrored(on);
    });
    addToggleRow(QStringLiteral("Flash Effect"), m_flashToggle, true, [this](bool on) {
        m_flashEnabled = on;
    });
    addToggleRow(QStringLiteral("Shutter SFX"), m_soundToggle, true, [this](bool on) {
        m_soundEnabled = on;
    });
    addToggleRow(QStringLiteral("Microphone"), m_micToggle, true, [this](bool on) {
        m_micEnabled = on;
    });
    contentLay->addWidget(togglesSection);

    // V4L2 Hardware Sliders
    auto *hwSection = new QWidget(sidebarContent);
    auto *hwLayout = new QVBoxLayout(hwSection);
    hwLayout->setContentsMargins(0, 0, 0, 0);
    hwLayout->setSpacing(6);
    auto *hwLbl = new QLabel(QStringLiteral("DEVICE CONTROLS"), hwSection);
    hwLbl->setObjectName(QStringLiteral("fieldLabel"));
    hwLayout->addWidget(hwLbl);
    m_sliders = new ControlSliders(hwSection);
    hwLayout->addWidget(m_sliders);
    contentLay->addWidget(hwSection);

    // About App Section in Sidebar
    auto *aboutSection = new QWidget(sidebarContent);
    auto *aboutSecLayout = new QVBoxLayout(aboutSection);
    aboutSecLayout->setContentsMargins(0, 0, 0, 0);
    aboutSecLayout->setSpacing(6);
    auto *aboutLbl = new QLabel(QStringLiteral("ABOUT"), aboutSection);
    aboutLbl->setObjectName(QStringLiteral("fieldLabel"));
    aboutSecLayout->addWidget(aboutLbl);

    auto *aboutAppBtn = new QPushButton(QStringLiteral("About Camera Pro"), aboutSection);
    aboutAppBtn->setIcon(icons::infoIcon(QColor(29, 29, 31), 16));
    aboutAppBtn->setIconSize(QSize(14, 14));
    aboutAppBtn->setCursor(Qt::PointingHandCursor);
    aboutAppBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: rgba(0, 0, 0, 0.05);"
        "  border: 1px solid rgba(0, 0, 0, 0.08);"
        "  border-radius: 8px;"
        "  color: #1d1d1f;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  padding: 8px 12px;"
        "  text-align: left;"
        "}"
        "QPushButton:hover { background: rgba(0, 0, 0, 0.09); }"
    ));
    connect(aboutAppBtn, &QPushButton::clicked, this, [this] {
        toggleSidebar(false);
        m_aboutModal->showModal();
    });
    aboutSecLayout->addWidget(aboutAppBtn);
    contentLay->addWidget(aboutSection);

    sidebarScroll->setWidget(sidebarContent);
    sidebarLayout->addWidget(sidebarScroll, 1);

    // 5. Preview Modal Dialog
    m_previewModal = new PreviewModal(m_centralRoot);
    connect(m_previewModal, &PreviewModal::deleteRequested, this, &MainWindow::onCaptureItemDeleted);

    // 6. About Modal Dialog
    m_aboutModal = new AboutModal(m_centralRoot);

    setCentralWidget(m_centralRoot);
    resize(1120, 760);
    setMinimumSize(800, 560);
}

void MainWindow::applyStyle()
{
    const QString css = QStringLiteral(R"(
QMainWindow {
    background: transparent;
}
QWidget#windowContainer {
    background: rgba(255, 255, 255, 0.95);
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 18px;
}
QWidget#titleBar {
    background: rgba(255, 255, 255, 0.75);
    border-top-left-radius: 18px;
    border-top-right-radius: 18px;
    border-bottom: 1px solid rgba(0, 0, 0, 0.08);
}
QPushButton#dotRed {
    background: #ff5f57;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 6px;
    min-width: 12px;
    min-height: 12px;
    max-width: 12px;
    max-height: 12px;
}
QPushButton#dotRed:hover {
    background: #e0443e;
}
QPushButton#dotYellow {
    background: #febc2e;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 6px;
    min-width: 12px;
    min-height: 12px;
    max-width: 12px;
    max-height: 12px;
}
QPushButton#dotYellow:hover {
    background: #dea123;
}
QPushButton#dotGreen {
    background: #28c840;
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 6px;
    min-width: 12px;
    min-height: 12px;
    max-width: 12px;
    max-height: 12px;
}
QPushButton#dotGreen:hover {
    background: #1fa834;
}
QLabel#titleLabel {
    color: #1d1d1f;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.01em;
}
QPushButton#headerSettingsBtn {
    background: transparent;
    border: none;
    color: #1d1d1f;
    font-size: 13px;
    font-weight: 500;
    padding: 4px 10px;
    border-radius: 6px;
}
QPushButton#headerSettingsBtn:hover {
    background: rgba(0, 0, 0, 0.06);
}
QWidget#bodyArea {
    background: #0d0d0f;
    border-bottom-left-radius: 18px;
    border-bottom-right-radius: 18px;
}
QWidget#stage {
    background: #0d0d0f;
}
QWidget#frameContainer {
    background: #000000;
    border-radius: 14px;
}
QWidget#topPill {
    background: rgba(0, 0, 0, 0.55);
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: 100px;
}
QLabel#hudStatusText {
    color: #ffffff;
    font-size: 12px;
    font-weight: 500;
    letter-spacing: 0.01em;
}
QLabel#countdownLabel {
    color: #ffffff;
    font-size: 120px;
    font-weight: 800;
}
QWidget#emptyState {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #242426, stop:1 #141416);
    border-radius: 14px;
}
QLabel#emptyGlyph {
    color: rgba(255, 255, 255, 0.85);
    font-size: 42px;
}
QLabel#emptyMsg {
    color: rgba(255, 255, 255, 0.65);
    font-size: 13px;
}
QPushButton#initBtn {
    background: #ffffff;
    color: #1d1d1f;
    border: none;
    border-radius: 100px;
    padding: 8px 20px;
    font-size: 13px;
    font-weight: 600;
}
QPushButton#initBtn:hover {
    background: #f2f2f4;
}
QWidget#dockWrap {
    background: #0d0d0f;
    border-bottom-left-radius: 18px;
    border-bottom-right-radius: 18px;
}
QWidget#modeSelector {
    background: transparent;
}
QPushButton#modeBtn {
    background: transparent;
    border: none;
    color: rgba(255, 255, 255, 0.4);
    font-size: 12px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    padding: 2px 8px;
}
QPushButton#modeBtn:checked {
    color: #ffffff;
}
QWidget#dock {
    background: rgba(255, 255, 255, 0.12);
    border: 1px solid rgba(255, 255, 255, 0.18);
    border-radius: 100px;
}
QPushButton#dockIconBtn {
    width: 36px;
    height: 36px;
    min-width: 36px;
    min-height: 36px;
    max-width: 36px;
    max-height: 36px;
    border-radius: 18px;
    border: none;
    background: rgba(255, 255, 255, 0.12);
    color: #ffffff;
    font-size: 14px;
}
QPushButton#dockIconBtn:hover {
    background: rgba(255, 255, 255, 0.25);
}
QWidget#sidebarBackdrop {
    background: rgba(0, 0, 0, 0.35);
    border-radius: 18px;
}
QWidget#sidebar {
    background: rgba(255, 255, 255, 0.94);
    border-left: 1px solid rgba(0, 0, 0, 0.08);
    border-top-right-radius: 18px;
    border-bottom-right-radius: 18px;
}
QLabel#sidebarHeaderTitle {
    color: #1d1d1f;
    font-size: 14px;
    font-weight: 700;
}
QPushButton#closeSidebarBtn {
    background: rgba(0, 0, 0, 0.06);
    border: none;
    width: 24px;
    height: 24px;
    border-radius: 12px;
    color: #86868b;
    font-size: 11px;
}
QPushButton#closeSidebarBtn:hover {
    background: rgba(0, 0, 0, 0.12);
    color: #1d1d1f;
}
QLabel#fieldLabel {
    color: #86868b;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.05em;
}
QLabel#deviceStatusPill {
    color: #34c759;
    font-size: 11px;
    font-weight: 600;
}
QComboBox {
    background: rgba(0, 0, 0, 0.05);
    color: #1d1d1f;
    border: 1px solid rgba(0, 0, 0, 0.08);
    border-radius: 8px;
    font-size: 12px;
    font-weight: 500;
    padding: 6px 8px;
    min-height: 26px;
}
QComboBox QAbstractItemView {
    background: #ffffff;
    color: #1d1d1f;
    selection-background-color: #0071e3;
    selection-color: #ffffff;
}
QSlider::groove:horizontal {
    background: rgba(0, 0, 0, 0.12);
    height: 4px;
    border-radius: 2px;
}
QSlider::sub-page:horizontal {
    background: #0071e3;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: #ffffff;
    border: 2px solid #0071e3;
    width: 12px;
    height: 12px;
    margin: -4px 0;
    border-radius: 6px;
}
)");
    qApp->setStyleSheet(css);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    const int drawerW = 290;
    m_sidebarBackdrop->setGeometry(m_centralRoot->rect());
    m_previewModal->setGeometry(m_centralRoot->rect());
    if (m_aboutModal) {
        m_aboutModal->setGeometry(m_centralRoot->rect());
    }
    if (!m_sidebar->isHidden()) {
        m_sidebar->setGeometry(m_centralRoot->width() - drawerW, 0, drawerW, m_centralRoot->height());
    } else {
        m_sidebar->setGeometry(m_centralRoot->width(), 0, drawerW, m_centralRoot->height());
    }
}

MainWindow::ResizeEdge MainWindow::calculateResizeEdge(const QPoint &pos) const
{
    if (isMaximized()) return ResizeEdge::None;
    const int border = 6;
    const int w = width();
    const int h = height();

    const bool left = pos.x() <= border;
    const bool right = pos.x() >= w - border;
    const bool top = pos.y() <= border;
    const bool bottom = pos.y() >= h - border;

    if (top && left) return ResizeEdge::TopLeft;
    if (top && right) return ResizeEdge::TopRight;
    if (bottom && left) return ResizeEdge::BottomLeft;
    if (bottom && right) return ResizeEdge::BottomRight;
    if (left) return ResizeEdge::Left;
    if (right) return ResizeEdge::Right;
    if (top) return ResizeEdge::Top;
    if (bottom) return ResizeEdge::Bottom;

    return ResizeEdge::None;
}

void MainWindow::updateCursorForEdge(ResizeEdge edge)
{
    switch (edge) {
    case ResizeEdge::Left:
    case ResizeEdge::Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case ResizeEdge::Top:
    case ResizeEdge::Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case ResizeEdge::TopLeft:
    case ResizeEdge::BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case ResizeEdge::TopRight:
    case ResizeEdge::BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    default:
        unsetCursor();
        break;
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !isMaximized()) {
        ResizeEdge edge = calculateResizeEdge(event->pos());
        if (edge != ResizeEdge::None) {
            m_isResizing = true;
            m_currentResizeEdge = edge;
            m_resizeStartGeometry = geometry();
            m_resizeStartPos = event->globalPosition().toPoint();
            event->accept();
            return;
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isResizing) {
        QPoint delta = event->globalPosition().toPoint() - m_resizeStartPos;
        QRect geom = m_resizeStartGeometry;

        switch (m_currentResizeEdge) {
        case ResizeEdge::Left:
            geom.setLeft(geom.left() + delta.x());
            break;
        case ResizeEdge::Right:
            geom.setRight(geom.right() + delta.x());
            break;
        case ResizeEdge::Top:
            geom.setTop(geom.top() + delta.y());
            break;
        case ResizeEdge::Bottom:
            geom.setBottom(geom.bottom() + delta.y());
            break;
        case ResizeEdge::TopLeft:
            geom.setTopLeft(geom.topLeft() + delta);
            break;
        case ResizeEdge::TopRight:
            geom.setTopRight(geom.topRight() + delta);
            break;
        case ResizeEdge::BottomLeft:
            geom.setBottomLeft(geom.bottomLeft() + delta);
            break;
        case ResizeEdge::BottomRight:
            geom.setBottomRight(geom.bottomRight() + delta);
            break;
        default:
            break;
        }

        if (geom.width() >= minimumWidth() && geom.height() >= minimumHeight()) {
            setGeometry(geom);
        }
        event->accept();
        return;
    }

    ResizeEdge edge = calculateResizeEdge(event->pos());
    updateCursorForEdge(edge);
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isResizing) {
        m_isResizing = false;
        m_currentResizeEdge = ResizeEdge::None;
        unsetCursor();
        event->accept();
        return;
    }
    QMainWindow::mouseReleaseEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_sidebarBackdrop) {
        if (event->type() == QEvent::MouseButtonPress) {
            toggleSidebar(false);
            return true;
        }
    }

    if (watched == m_titleBar) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                QWidget *child = m_titleBar->childAt(me->pos());
                if (!child || qobject_cast<QLabel *>(child)) {
                    m_isTitleDragging = true;
                    m_dragStartPos = me->globalPosition().toPoint() - frameGeometry().topLeft();
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (m_isTitleDragging && (me->buttons() & Qt::LeftButton)) {
                if (isMaximized()) {
                    showNormal();
                }
                move(me->globalPosition().toPoint() - m_dragStartPos);
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            m_isTitleDragging = false;
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                QWidget *child = m_titleBar->childAt(me->pos());
                if (!child || qobject_cast<QLabel *>(child)) {
                    if (isMaximized()) {
                        showNormal();
                    } else {
                        showMaximized();
                    }
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::toggleSidebar(bool show)
{
    const int drawerW = 290;
    const QRect area = m_centralRoot->rect();
    m_sidebarBackdrop->setGeometry(area);

    if (show) {
        m_sidebarBackdrop->show();
        m_sidebarBackdrop->raise();
        m_sidebar->show();
        m_sidebar->raise();

        m_headerSettingsBtn->setStyleSheet(QStringLiteral("background: rgba(0, 0, 0, 0.12); color: #0071e3; font-weight: 600;"));
        m_dockSettingsBtn->setStyleSheet(QStringLiteral("background: #ffffff; color: #111111;"));

        auto *anim = new QPropertyAnimation(m_sidebar, "geometry");
        anim->setDuration(240);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->setStartValue(QRect(area.width(), 0, drawerW, area.height()));
        anim->setEndValue(QRect(area.width() - drawerW, 0, drawerW, area.height()));
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        m_headerSettingsBtn->setStyleSheet(QString());
        m_dockSettingsBtn->setStyleSheet(QString());

        auto *anim = new QPropertyAnimation(m_sidebar, "geometry");
        anim->setDuration(200);
        anim->setEasingCurve(QEasingCurve::InCubic);
        anim->setStartValue(m_sidebar->geometry());
        anim->setEndValue(QRect(area.width(), 0, drawerW, area.height()));
        connect(anim, &QAbstractAnimation::finished, this, [this] {
            m_sidebar->hide();
            m_sidebarBackdrop->hide();
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void MainWindow::setAppMode(AppMode mode)
{
    m_appMode = mode;
    m_modePhotoBtn->setChecked(mode == AppMode::Photo);
    m_modeVideoBtn->setChecked(mode == AppMode::Video);
    m_shutterBtn->setMode(mode == AppMode::Photo ? ShutterButton::Mode::Photo : ShutterButton::Mode::Video);

    if (m_capture && m_capture->isOpen()) {
        m_hudStatus->setText(mode == AppMode::Video ? QStringLiteral("Video Ready") : QStringLiteral("Ready"));
    }
}

void MainWindow::onDevicesChanged()
{
    const QString previous = m_deviceCombo->currentData().toString();

    m_deviceCombo->blockSignals(true);
    m_deviceCombo->clear();
    for (const DeviceInfo &d : m_devMgr->devices()) {
        m_deviceCombo->addItem(QStringLiteral("%1 - %2").arg(d.node, d.name), d.node);
    }
    m_deviceCombo->blockSignals(false);

    if (m_deviceCombo->count() == 0) {
        closeDevice();
        m_emptyState->show();
        m_deviceStatus->setText(QStringLiteral("[ Disconnected ]"));
        m_deviceStatus->setStyleSheet(QStringLiteral("color: #ff3b30; font-size: 11px; font-weight: 600;"));
        m_hudStatus->setText(QStringLiteral("No Signal"));
        return;
    }

    int idx = previous.isEmpty() ? 0 : m_deviceCombo->findData(previous);
    if (idx < 0) idx = 0;
    m_deviceCombo->setCurrentIndex(idx);
    onDeviceSelected(idx);
}

void MainWindow::onDeviceSelected(int index)
{
    if (index < 0) return;
    const QString node = m_deviceCombo->itemData(index).toString();
    if (node.isEmpty() || (m_capture->isOpen() && m_capture->node() == node))
        return;
    openDevice(node);
}

void MainWindow::openDevice(const QString &node)
{
    closeDevice();

    // CaptureDevice lives on m_captureThread and grabFrame() is delivered
    // there via queued connection; calling open/start directly from the GUI
    // thread would race with in-flight grabFrame() calls (segfault on rapid
    // resolution/device switches). Route through the same thread instead.
    bool opened = false;
    QMetaObject::invokeMethod(m_capture, "open", Qt::BlockingQueuedConnection,
                               Q_RETURN_ARG(bool, opened), Q_ARG(QString, node));
    if (!opened) {
        m_emptyState->show();
        m_deviceStatus->setText(QStringLiteral("[ Connection Error ]"));
        m_deviceStatus->setStyleSheet(QStringLiteral("color: #ff3b30; font-size: 11px; font-weight: 600;"));
        m_hudStatus->setText(QStringLiteral("Failed to open"));
        return;
    }

    m_controls->open(node);

    const QSize desired = m_qualitySegment ? m_qualitySegment->currentData().toSize() : QSize(1280, 720);

    bool started = false;
    QMetaObject::invokeMethod(m_capture, "start", Qt::BlockingQueuedConnection,
                               Q_RETURN_ARG(bool, started),
                               Q_ARG(QSize, desired), Q_ARG(uint32_t, 0));
    if (!started) {
        m_emptyState->show();
        m_deviceStatus->setText(QStringLiteral("[ Stream Error ]"));
        m_deviceStatus->setStyleSheet(QStringLiteral("color: #ff3b30; font-size: 11px; font-weight: 600;"));
        m_hudStatus->setText(QStringLiteral("Stream error"));
        m_controls->close();
        QMetaObject::invokeMethod(m_capture, "close", Qt::BlockingQueuedConnection);
        return;
    }

    m_grabTimer->start();
    m_emptyState->hide();
    m_deviceStatus->setText(QStringLiteral("[ Connected ]"));
    m_deviceStatus->setStyleSheet(QStringLiteral("color: #34c759; font-size: 11px; font-weight: 600;"));

    // Preset binding
    m_currentDeviceKey = QString(node).replace(QLatin1Char('/'), QLatin1Char('_'));
    m_sliders->bind(m_controls, m_currentDeviceKey);
    if (ControlPanel::presets(m_currentDeviceKey).contains(QStringLiteral("default")))
        m_controls->loadPreset(m_currentDeviceKey, QStringLiteral("default"));

    m_hudStatus->setText(m_appMode == AppMode::Video ? QStringLiteral("Video Ready") : QStringLiteral("Ready"));
    updateTrayState();
}

void MainWindow::closeDevice()
{
    if (m_recording)
        stopRecording();

    m_grabTimer->stop();
    m_preview->clearFrame();

    if (m_capture->isOpen()) {
        QMetaObject::invokeMethod(m_capture, "stop", Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(m_capture, "close", Qt::BlockingQueuedConnection);
    }
    m_controls->close();
    m_sliders->clear();
    m_currentDeviceKey.clear();
    m_emptyState->show();
    m_deviceStatus->setText(QStringLiteral("[ Disconnected ]"));
    m_deviceStatus->setStyleSheet(QStringLiteral("color: #86868b; font-size: 11px; font-weight: 600;"));
    updateTrayState();
}

void MainWindow::onFrameReady(const uchar *data, int bytes, QSize size, uint32_t pixFmt)
{
    m_preview->presentFrame(data, bytes, size, pixFmt);

    if (m_recording) {
        const QImage frame = m_preview->processedLastFrame();
        if (!frame.isNull())
            m_encoder->writeFrame(frame.constBits(), frame.sizeInBytes(), frame.size());
    }
}

void MainWindow::onShutterClicked()
{
    if (m_appMode == AppMode::Photo) {
        executePhotoCaptureSequence();
    } else {
        if (!m_recording)
            startRecording();
        else
            stopRecording();
    }
}

void MainWindow::executePhotoCaptureSequence()
{
    if (m_countdownTimer->isActive()) return;

    if (m_timerDuration > 0) {
        m_countdownRemaining = m_timerDuration;
        m_countdownLabel->setText(QString::number(m_countdownRemaining));
        m_countdownLabel->show();
        playBeepSfx(false);
        m_countdownTimer->start();
    } else {
        m_burstRemaining = m_burstCount;
        doSinglePhotoCapture();
    }
}

void MainWindow::doSinglePhotoCapture()
{
    const QImage frame = m_preview->processedLastFrame();
    if (frame.isNull()) {
        m_hudStatus->setText(QStringLiteral("No frame to capture"));
        return;
    }

    if (m_flashEnabled) {
        m_preview->triggerFlash();
    }

    playShutterSfx();

    QSettings s(QStringLiteral("c-mi"), QStringLiteral("c-mi"));
    QString dir = s.value(QStringLiteral("photoDir"), captureDir()).toString();
    QDir().mkpath(dir);

    const QString name = QDateTime::currentDateTime().toString(QStringLiteral("photo_yyyyMMdd_hhmmss_zzz"));
    const QString path = QDir(dir).filePath(name + QStringLiteral(".jpg"));

    QString err;
    if (PhotoEncoder::save(path, frame.constBits(), frame.size(), PhotoEncoder::Format::JPEG, &err)) {
        m_hudStatus->setText(QStringLiteral("Photo Captured"));
        QTimer::singleShot(1500, this, [this] {
            if (!m_recording)
                m_hudStatus->setText(m_appMode == AppMode::Video ? QStringLiteral("Video Ready") : QStringLiteral("Ready"));
        });

        // Add to captures tray
        MediaItem item;
        item.filePath = path;
        item.thumbnail = frame;
        item.type = MediaItem::Type::Photo;
        item.timestamp = QDateTime::currentDateTime();
        m_capturesTray->addItem(item);
    } else {
        m_hudStatus->setText(QStringLiteral("Photo failed"));
    }

    m_burstRemaining--;
    if (m_burstRemaining > 0) {
        QTimer::singleShot(220, this, &MainWindow::doSinglePhotoCapture);
    }
}

void MainWindow::startRecording()
{
    if (!m_capture->isStreaming()) {
        m_hudStatus->setText(QStringLiteral("No active stream"));
        return;
    }

    QSettings s(QStringLiteral("c-mi"), QStringLiteral("c-mi"));
    QString dir = s.value(QStringLiteral("videoDir"), captureDir()).toString();
    QDir().mkpath(dir);

    const QString name = QDateTime::currentDateTime().toString(QStringLiteral("video_yyyyMMdd_hhmmss"));
    const QString path = QDir(dir).filePath(name + QStringLiteral(".mp4"));

    const QSize sz = m_preview->processedLastFrame().size();
    if (m_encoder->start(path, sz.isValid() ? sz : m_capture->frameSize(), 30, m_micEnabled)) {
        m_recording = true;
        m_shutterBtn->setRecording(true);
        m_recDot->show();
        m_recordElapsed.restart();
        m_recordTimer->start();
        m_hudStatus->setText(QStringLiteral("REC 00:00"));
    }
    updateTrayState();
}

void MainWindow::stopRecording()
{
    if (!m_recording) return;
    m_encoder->stop();
    m_recording = false;
    m_shutterBtn->setRecording(false);
    m_recDot->hide();
    m_recordTimer->stop();
    playShutterSfx();
    updateTrayState();
}

void MainWindow::playShutterSfx()
{
    if (!m_soundEnabled) return;
    QApplication::beep();
}

void MainWindow::playBeepSfx(bool)
{
    if (!m_soundEnabled) return;
    QApplication::beep();
}

void MainWindow::onCaptureItemDeleted(const MediaItem &item)
{
    QFile::remove(item.filePath);
    m_capturesTray->removeItem(item.filePath);
}

void MainWindow::onCaptureError(const QString &message)
{
    m_hudStatus->setText(QStringLiteral("Error: %1").arg(message));
}

void MainWindow::updateTrayState()
{
    m_tray->setCameraInUse(m_capture->isOpen());
    m_tray->setRecording(m_recording);
}

QString MainWindow::captureDir() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (base.isEmpty())
        base = QDir::homePath();
    QString dir = QDir(base).filePath(QStringLiteral("c-mi"));
    QDir().mkpath(dir);
    return dir;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
    qApp->quit();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (m_aboutModal && !m_aboutModal->isHidden()) {
            m_aboutModal->hideModal();
        } else if (!m_previewModal->isHidden()) {
            m_previewModal->hideModal();
        } else if (!m_sidebar->isHidden()) {
            toggleSidebar(false);
        }
        return;
    }
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        onShutterClicked();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

} // namespace cmi
