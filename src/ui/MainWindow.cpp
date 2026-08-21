#include "MainWindow.h"

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
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QVBoxLayout>

#include <linux/videodev2.h>

namespace cmi {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("c~mi"));

    m_devMgr = new DeviceManager(this);
    m_capture = new CaptureDevice;
    m_capture->moveToThread(&m_captureThread);
    m_captureThread.start();

    m_controls = new ControlPanel(this);
    m_encoder = new VideoEncoder(this);
    m_tray = new TrayIcon(this);

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

    // Cross-thread wiring: capture thread -> GUI thread.
    connect(m_capture, &CaptureDevice::frameReady,
            this, &MainWindow::onFrameReady, Qt::QueuedConnection);
    connect(m_capture, &CaptureDevice::errorOccurred,
            this, &MainWindow::onCaptureError, Qt::QueuedConnection);
    connect(m_encoder, &VideoEncoder::errorOccurred,
            this, &MainWindow::onCaptureError);
    connect(m_encoder, &VideoEncoder::recordingStopped,
            this, [this](const QString &path) {
        m_statusLabel->setText(QStringLiteral("saved %1").arg(path));
    });

    m_grabTimer = new QTimer(this);
    m_grabTimer->setInterval(8); // ~120 Hz polling; DQBUF blocks only per-frame
    connect(m_grabTimer, &QTimer::timeout,
            m_capture, &CaptureDevice::grabFrame, Qt::QueuedConnection);

    buildUi();
    applyStyle();

    // Populate device list.
    onDevicesChanged();
}

MainWindow::~MainWindow()
{
    closeDevice();
    m_captureThread.quit();
    m_captureThread.wait();
    delete m_capture; // lives on the now-stopped thread; parent is none
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("centralRoot"));
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(14, 12, 14, 14);
    root->setSpacing(10);

    auto *titleBar = new QWidget(central);
    titleBar->setObjectName(QStringLiteral("titleBar"));
    auto *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(14, 8, 14, 8);

    auto *windowControls = new QWidget(titleBar);
    auto *controlsLayout = new QHBoxLayout(windowControls);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(8);
    for (const char *color : {"#ff5f57", "#febc2e", "#28c840"}) {
        auto *dot = new QLabel(titleBar);
        dot->setObjectName(QStringLiteral("windowControl"));
        dot->setStyleSheet(QStringLiteral("QLabel#windowControl { background: %1; border-radius: 6px; min-width: 12px; min-height: 12px; max-width: 12px; max-height: 12px; }")
                                .arg(QString::fromLatin1(color)));
        controlsLayout->addWidget(dot);
    }
    titleLayout->addWidget(windowControls);

    auto *titleLabel = new QLabel(QStringLiteral("Camera Pro"), titleBar);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    titleLayout->addWidget(titleLabel, 1);

    auto *navButtons = new QWidget(titleBar);
    auto *navLayout = new QHBoxLayout(navButtons);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(6);

    auto *homeBtn = new QPushButton(QStringLiteral("⌂"), titleBar);
    homeBtn->setObjectName(QStringLiteral("navButton"));
    auto *settingsBtn = new QPushButton(QStringLiteral("⚙"), titleBar);
    settingsBtn->setObjectName(QStringLiteral("navButton"));
    navLayout->addWidget(homeBtn);
    navLayout->addWidget(settingsBtn);
    titleLayout->addWidget(navButtons);
    root->addWidget(titleBar);

    auto *content = new QWidget(central);
    content->setObjectName(QStringLiteral("contentArea"));
    auto *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(12);

    auto *stage = new QWidget(content);
    stage->setObjectName(QStringLiteral("stage"));
    auto *stageLayout = new QVBoxLayout(stage);
    stageLayout->setContentsMargins(12, 12, 12, 12);
    stageLayout->setSpacing(10);

    m_onAirLabel = new QLabel(QStringLiteral("LIVE"), stage);
    m_onAirLabel->setObjectName(QStringLiteral("onAir"));
    m_onAirLabel->setAlignment(Qt::AlignCenter);
    m_onAirLabel->hide();
    stageLayout->addWidget(m_onAirLabel, 0, Qt::AlignHCenter);

    m_preview = new PreviewWidget(stage);
    m_preview->setObjectName(QStringLiteral("previewPanel"));
    stageLayout->addWidget(m_preview, 1);

    m_statusLabel = new QLabel(QStringLiteral("idle"), stage);
    m_statusLabel->setObjectName(QStringLiteral("status"));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    stageLayout->addWidget(m_statusLabel);

    auto *floatingActions = new QWidget(stage);
    floatingActions->setObjectName(QStringLiteral("floatingActions"));
    floatingActions->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    auto *floatingLayout = new QHBoxLayout(floatingActions);
    floatingLayout->setContentsMargins(0, 0, 0, 0);
    floatingLayout->setSpacing(10);
    floatingLayout->setAlignment(Qt::AlignCenter);

    auto *swapBtn = new QPushButton(QStringLiteral("⇄"), stage);
    swapBtn->setObjectName(QStringLiteral("floatingAction"));
    auto *gridBtn = new QPushButton(QStringLiteral("▦"), stage);
    gridBtn->setObjectName(QStringLiteral("floatingAction"));
    floatingLayout->addWidget(swapBtn);
    floatingLayout->addWidget(gridBtn);
    stageLayout->addWidget(floatingActions, 0, Qt::AlignBottom | Qt::AlignHCenter);

    auto *shutterBtn = new QPushButton(stage);
    shutterBtn->setObjectName(QStringLiteral("shutterButton"));
    shutterBtn->setText(QStringLiteral(""));
    shutterBtn->setCursor(Qt::PointingHandCursor);
    connect(shutterBtn, &QPushButton::clicked, this, &MainWindow::onPhoto);
    stageLayout->addWidget(shutterBtn, 0, Qt::AlignCenter);

    auto *drawerBackdrop = new QWidget(central);
    drawerBackdrop->setObjectName(QStringLiteral("drawerBackdrop"));
    drawerBackdrop->hide();
    auto *drawer = new QWidget(central);
    drawer->setObjectName(QStringLiteral("drawer"));
    drawer->hide();
    drawer->setAttribute(Qt::WA_StyledBackground, true);

    auto *drawerLayout = new QVBoxLayout(drawer);
    drawerLayout->setContentsMargins(16, 16, 16, 16);
    drawerLayout->setSpacing(10);

    auto *drawerHeader = new QLabel(QStringLiteral("Camera"), drawer);
    drawerHeader->setObjectName(QStringLiteral("sectionTitle"));
    drawerLayout->addWidget(drawerHeader);

    auto *deviceLabel = new QLabel(QStringLiteral("device"), drawer);
    deviceLabel->setObjectName(QStringLiteral("fieldLabel"));
    drawerLayout->addWidget(deviceLabel);
    m_deviceCombo = new QComboBox(drawer);
    connect(m_deviceCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::onDeviceSelected);
    drawerLayout->addWidget(m_deviceCombo);

    auto *fmtLabel = new QLabel(QStringLiteral("format"), drawer);
    fmtLabel->setObjectName(QStringLiteral("fieldLabel"));
    drawerLayout->addWidget(fmtLabel);
    m_formatCombo = new QComboBox(drawer);
    drawerLayout->addWidget(m_formatCombo);

    auto *resLabel = new QLabel(QStringLiteral("resolution"), drawer);
    resLabel->setObjectName(QStringLiteral("fieldLabel"));
    drawerLayout->addWidget(resLabel);
    m_resolutionCombo = new QComboBox(drawer);
    m_resolutionCombo->addItem(QStringLiteral("640x480"), QSize(640, 480));
    m_resolutionCombo->addItem(QStringLiteral("1280x720"), QSize(1280, 720));
    m_resolutionCombo->addItem(QStringLiteral("1920x1080"), QSize(1920, 1080));
    m_resolutionCombo->setCurrentIndex(1);
    connect(m_resolutionCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        if (m_capture && m_capture->isOpen() && m_capture->isStreaming())
            openDevice(m_capture->node());
    });
    drawerLayout->addWidget(m_resolutionCombo);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    m_photoBtn = new QPushButton(QStringLiteral("Photo"), drawer);
    m_photoBtn->setObjectName(QStringLiteral("actionButton"));
    m_recordBtn = new QPushButton(QStringLiteral("Record"), drawer);
    m_recordBtn->setObjectName(QStringLiteral("actionButton"));
    m_recordBtn->setCheckable(true);
    btnRow->addWidget(m_photoBtn);
    btnRow->addWidget(m_recordBtn);
    drawerLayout->addLayout(btnRow);

    connect(m_photoBtn, &QPushButton::clicked, this, &MainWindow::onPhoto);
    connect(m_recordBtn, &QPushButton::clicked, this, &MainWindow::onRecordToggled);

    m_sliders = new ControlSliders(drawer);
    drawerLayout->addWidget(m_sliders, 1);

    contentLayout->addWidget(stage, 1);
    root->addWidget(content, 1);

    auto *dock = new QWidget(central);
    dock->setObjectName(QStringLiteral("dock"));
    auto *dockLayout = new QHBoxLayout(dock);
    dockLayout->setContentsMargins(10, 6, 10, 6);
    dockLayout->setSpacing(10);

    auto *modeWrap = new QWidget(dock);
    auto *modeLayout = new QHBoxLayout(modeWrap);
    modeLayout->setContentsMargins(0, 0, 0, 0);
    modeLayout->setSpacing(8);

    auto *photoMode = new QPushButton(QStringLiteral("PHOTO"), dock);
    photoMode->setCheckable(true);
    photoMode->setChecked(true);
    photoMode->setObjectName(QStringLiteral("dockButton"));
    auto *videoMode = new QPushButton(QStringLiteral("VIDEO"), dock);
    videoMode->setCheckable(true);
    videoMode->setObjectName(QStringLiteral("dockButton"));
    modeLayout->addWidget(photoMode);
    modeLayout->addWidget(videoMode);
    dockLayout->addWidget(modeWrap);

    auto *spacer = new QWidget(dock);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    dockLayout->addWidget(spacer);

    auto *dockSettingsBtn = new QPushButton(QStringLiteral("Settings"), dock);
    dockSettingsBtn->setObjectName(QStringLiteral("dockButton"));
    dockLayout->addWidget(dockSettingsBtn);
    root->addWidget(dock);

    auto toggleDrawer = [drawerBackdrop, drawer, central, settingsBtn, dockSettingsBtn](bool open) {
        const int drawerWidth = 320;
        const QRect area = central->rect();
        const QPoint closedPos(area.right() + 8, 0);
        const QPoint openPos(area.right() - drawerWidth, 0);

        drawerBackdrop->setGeometry(area);
        drawer->setGeometry(openPos.x(), 0, drawerWidth, area.height());

        if (open) {
            drawerBackdrop->show();
            drawer->show();
            drawerBackdrop->raise();
            drawer->raise();
            auto *anim = new QPropertyAnimation(drawer, "pos");
            anim->setDuration(240);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->setStartValue(closedPos);
            anim->setEndValue(openPos);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
            settingsBtn->setChecked(true);
            dockSettingsBtn->setChecked(true);
        } else {
            auto *anim = new QPropertyAnimation(drawer, "pos");
            anim->setDuration(220);
            anim->setEasingCurve(QEasingCurve::InCubic);
            anim->setStartValue(drawer->pos());
            anim->setEndValue(closedPos);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
            QTimer::singleShot(220, drawerBackdrop, [drawerBackdrop, drawer]() {
                drawerBackdrop->hide();
                drawer->hide();
            });
            settingsBtn->setChecked(false);
            dockSettingsBtn->setChecked(false);
        }
    };

    connect(settingsBtn, &QPushButton::clicked, this, [toggleDrawer, drawer]() {
        toggleDrawer(drawer->isHidden());
    });
    connect(dockSettingsBtn, &QPushButton::clicked, this, [toggleDrawer, drawer]() {
        toggleDrawer(drawer->isHidden());
    });

    drawer->move(central->width() + 8, 0);
    drawer->hide();
    drawerBackdrop->hide();

    setCentralWidget(central);
    resize(1100, 720);
}

void MainWindow::applyStyle()
{
    const QString css = QStringLiteral(R"(
QMainWindow {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #eef0f4, stop:1 #dfe3ea);
    color: #1d1d1f;
}
QWidget#centralRoot {
    background: transparent;
}
QWidget#titleBar {
    background: rgba(255,255,255,0.7);
    border: 1px solid rgba(0,0,0,0.08);
    border-radius: 14px;
}
QLabel#titleLabel {
    color: #1d1d1f;
    font-size: 13px;
    font-weight: 700;
    letter-spacing: 0.02em;
}
QPushButton#navButton {
    background: rgba(0,0,0,0.04);
    border: none;
    border-radius: 8px;
    min-width: 28px;
    min-height: 28px;
    color: #1d1d1f;
}
QPushButton#navButton:hover {
    background: rgba(0,0,0,0.08);
}
QPushButton#navButton:checked {
    background: rgba(0,0,0,0.12);
}
QWidget#contentArea {
    background: transparent;
}
QWidget#stage {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #15171a, stop:1 #050608);
    border: 1px solid rgba(255,255,255,0.07);
    border-radius: 20px;
    box-shadow: inset 0 2px 14px rgba(255,255,255,0.04);
}
QWidget#floatingActions {
    background: transparent;
    margin-bottom: 16px;
}
QPushButton#floatingAction {
    background: rgba(255,255,255,0.12);
    border: 1px solid rgba(255,255,255,0.16);
    border-radius: 50%;
    color: #ffffff;
    min-width: 34px;
    min-height: 34px;
    max-width: 34px;
    max-height: 34px;
}
QPushButton#shutterButton {
    background: qradialgradient(cx:0.5, cy:0.5, radius:0.6, fx:0.5, fy:0.5, stop:0 #ffffff, stop:0.3 #ffffff, stop:0.35 #f4f4f4, stop:0.9 #dfe3ea, stop:1 #cfd5dc);
    border: 4px solid rgba(255,255,255,0.65);
    border-radius: 50%;
    min-width: 76px;
    min-height: 76px;
    max-width: 76px;
    max-height: 76px;
    margin-bottom: 16px;
    box-shadow: 0 10px 22px rgba(0,0,0,0.28);
}
QPushButton#shutterButton:hover {
    background: qradialgradient(cx:0.5, cy:0.5, radius:0.6, fx:0.5, fy:0.5, stop:0 #ffffff, stop:0.3 #ffffff, stop:0.35 #f2f2f2, stop:0.9 #d9dde3, stop:1 #c7ced6);
}
QWidget#drawer {
    background: rgba(255,255,255,0.78);
    border-left: 1px solid rgba(0,0,0,0.08);
    border-top-left-radius: 18px;
    border-bottom-left-radius: 18px;
    box-shadow: -10px 0 25px rgba(0,0,0,0.12);
}
QWidget#drawerBackdrop {
    background: rgba(0,0,0,0.18);
}
QLabel#sectionTitle {
    color: #1d1d1f;
    font-size: 13px;
    font-weight: 700;
    letter-spacing: 0.04em;
    text-transform: uppercase;
}
QLabel#fieldLabel {
    color: #6a6e74;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
}
QLabel#onAir {
    background: rgba(0,0,0,0.52);
    color: #ffffff;
    border: 1px solid rgba(255,255,255,0.12);
    border-radius: 999px;
    padding: 6px 12px;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.12em;
}
QLabel#status {
    color: rgba(255,255,255,0.76);
    font-size: 11px;
    padding-bottom: 2px;
}
QComboBox,
QPushButton,
QCheckBox,
QSlider {
    font-family: "SF Pro Display", "Segoe UI", sans-serif;
}
QComboBox {
    background: rgba(0,0,0,0.04);
    color: #1d1d1f;
    border: 1px solid rgba(0,0,0,0.08);
    border-radius: 10px;
    padding: 6px 8px;
    min-height: 30px;
}
QComboBox QAbstractItemView {
    background: #ffffff;
    color: #1d1d1f;
    selection-background-color: #0071e3;
    selection-color: #ffffff;
}
QPushButton {
    background: rgba(0,0,0,0.04);
    color: #1d1d1f;
    border: 1px solid rgba(0,0,0,0.08);
    border-radius: 10px;
    padding: 8px 12px;
    min-height: 32px;
}
QPushButton:hover {
    background: rgba(0,0,0,0.07);
}
QPushButton:checked,
QPushButton:pressed {
    background: #0071e3;
    color: #ffffff;
    border-color: rgba(0,0,0,0.02);
}
QPushButton#actionButton {
    min-width: 90px;
}
QPushButton#dockButton {
    background: rgba(255,255,255,0.45);
    border: 1px solid rgba(0,0,0,0.06);
    border-radius: 999px;
    min-width: 88px;
}
QWidget#dock {
    background: rgba(17,18,20,0.86);
    border: 1px solid rgba(255,255,255,0.08);
    border-radius: 18px;
}
QSlider::groove:horizontal {
    background: rgba(0,0,0,0.12);
    height: 4px;
    border-radius: 3px;
}
QSlider::sub-page:horizontal {
    background: #0071e3;
    border-radius: 3px;
}
QSlider::handle:horizontal {
    background: #ffffff;
    border: 2px solid #0071e3;
    width: 12px;
    height: 12px;
    margin: -5px 0;
    border-radius: 8px;
}
QSlider::add-page:horizontal {
    background: rgba(0,0,0,0.12);
}
QCheckBox {
    spacing: 8px;
}
QCheckBox::indicator {
    width: 14px;
    height: 14px;
    border-radius: 4px;
    border: 1px solid rgba(0,0,0,0.15);
    background: rgba(0,0,0,0.04);
}
QCheckBox::indicator:checked {
    background: #0071e3;
    border-color: #0071e3;
}
QSplitter::handle {
    background: transparent;
}
QToolTip {
    background: rgba(17,18,20,0.95);
    color: #ffffff;
    border: 1px solid rgba(255,255,255,0.12);
    border-radius: 8px;
}
    )" );
    qApp->setStyleSheet(css);
}

void MainWindow::onDevicesChanged()
{
    const QString previous = m_deviceCombo->currentData().toString();

    m_deviceCombo->blockSignals(true);
    m_deviceCombo->clear();
    for (const DeviceInfo &d : m_devMgr->devices())
        m_deviceCombo->addItem(QStringLiteral("%1 — %2").arg(d.node, d.name), d.node);
    m_deviceCombo->blockSignals(false);

    if (m_deviceCombo->count() == 0) {
        closeDevice();
        m_statusLabel->setText(QStringLiteral("no camera found"));
        return;
    }

    int idx = previous.isEmpty() ? 0 : m_deviceCombo->findData(previous);
    if (idx < 0)
        idx = 0;
    m_deviceCombo->setCurrentIndex(idx);
    onDeviceSelected(idx);
}

void MainWindow::onDeviceSelected(int index)
{
    if (index < 0)
        return;
    const QString node = m_deviceCombo->itemData(index).toString();
    if (node.isEmpty() || (m_capture->isOpen() && m_capture->node() == node))
        return;
    openDevice(node);
}

void MainWindow::openDevice(const QString &node)
{
    closeDevice();

    if (!m_capture->open(node)) {
        m_statusLabel->setText(QStringLiteral("failed to open %1").arg(node));
        return;
    }

    m_controls->open(node);

    // Formats
    m_formatCombo->blockSignals(true);
    m_formatCombo->clear();
    const auto formats = CaptureDevice::enumFormats(node);
    for (const auto &f : formats)
        m_formatCombo->addItem(f.description, static_cast<quint32>(f.pixFmt));
    m_formatCombo->blockSignals(false);

    const QSize desired = m_resolutionCombo && m_resolutionCombo->count() > 0
        ? m_resolutionCombo->currentData().toSize()
        : QSize(1280, 720);

    if (!m_capture->start(desired, 0)) {
        m_statusLabel->setText(QStringLiteral("failed to start stream"));
        m_controls->close();
        m_capture->close();
        return;
    }

    m_grabTimer->start();

    // Unique key for presets: use the device node path (stable enough).
    m_currentDeviceKey = QString(node).replace(QLatin1Char('/'), QLatin1Char('_'));
    m_sliders->bind(m_controls, m_currentDeviceKey);

    // Load "default" preset if one exists.
    if (ControlPanel::presets(m_currentDeviceKey).contains(QStringLiteral("default")))
        m_controls->loadPreset(m_currentDeviceKey, QStringLiteral("default"));

    m_onAirLabel->setVisible(true);
    m_statusLabel->setText(QStringLiteral("open: %1").arg(node));
    updateTrayState();
}

void MainWindow::closeDevice()
{
    if (m_recording)
        onRecordToggled();

    m_grabTimer->stop();
    m_preview->clearFrame();

    if (m_capture->isOpen()) {
        m_capture->stop();
        m_capture->close();
    }
    m_controls->close();
    m_sliders->clear();
    m_currentDeviceKey.clear();
    m_onAirLabel->setVisible(false);
    updateTrayState();
}

void MainWindow::onFrameReady(const uchar *data, int bytes, QSize size, uint32_t pixFmt)
{
    m_preview->presentFrame(data, bytes, size, pixFmt);

    if (m_recording) {
        const QImage frame = m_preview->lastFrame();
        if (!frame.isNull())
            m_encoder->writeFrame(frame.constBits(), frame.sizeInBytes(), frame.size());
    }
}

void MainWindow::onPhoto()
{
    const QImage frame = m_preview->lastFrame();
    if (frame.isNull()) {
        m_statusLabel->setText(QStringLiteral("no frame to capture"));
        return;
    }

    QSettings s(QStringLiteral("c-mi"), QStringLiteral("c-mi"));
    QString dir = s.value(QStringLiteral("photoDir"), captureDir()).toString();

    const QString name = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"));
    const QString path = QDir(dir).filePath(name + QStringLiteral(".jpg"));

    QString err;
    if (PhotoEncoder::save(path, frame.constBits(), frame.size(),
                           PhotoEncoder::Format::JPEG, &err)) {
        m_statusLabel->setText(QStringLiteral("photo: %1").arg(path));
    } else {
        m_statusLabel->setText(QStringLiteral("photo failed: %1").arg(err));
    }
}

void MainWindow::onRecordToggled()
{
    if (!m_recording) {
        if (!m_capture->isStreaming()) {
            m_statusLabel->setText(QStringLiteral("no active stream"));
            m_recordBtn->setChecked(false);
            return;
        }

        QSettings s(QStringLiteral("c-mi"), QStringLiteral("c-mi"));
        QString dir = s.value(QStringLiteral("videoDir"), captureDir()).toString();
        QDir().mkpath(dir);

        const QString name = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"));
        const QString path = QDir(dir).filePath(name + QStringLiteral(".mp4"));

        const QSize sz = m_capture->frameSize();
        if (m_encoder->start(path, sz, 30, true)) {
            m_recording = true;
            m_recordBtn->setText(QStringLiteral("[ stop ]"));
            m_statusLabel->setText(QStringLiteral("recording: %1").arg(path));
        } else {
            m_recordBtn->setChecked(false);
        }
    } else {
        m_encoder->stop();
        m_recording = false;
        m_recordBtn->setText(QStringLiteral("[ record ]"));
        m_recordBtn->setChecked(false);
    }
    updateTrayState();
}

void MainWindow::onCaptureError(const QString &message)
{
    m_statusLabel->setText(QStringLiteral("error: %1").arg(message));
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
    if (m_tray->isVisible()) {
        hide();
        event->ignore();
    } else {
        QMainWindow::closeEvent(event);
    }
}

} // namespace cmi
