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
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    auto *splitter = new QSplitter(Qt::Horizontal, central);

    // Left: preview + on-air indicator
    auto *leftPane = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    m_onAirLabel = new QLabel(QStringLiteral("● CAMERA IN USE"), leftPane);
    m_onAirLabel->setObjectName(QStringLiteral("onAir"));
    m_onAirLabel->setAlignment(Qt::AlignCenter);
    m_onAirLabel->setVisible(false);
    leftLayout->addWidget(m_onAirLabel);

    m_preview = new PreviewWidget(leftPane);
    leftLayout->addWidget(m_preview, 1);
    splitter->addWidget(leftPane);

    // Right: control panel
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("panel"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(8, 8, 8, 8);
    panelLayout->setSpacing(8);

    auto *devLabel = new QLabel(QStringLiteral("device"), panel);
    m_deviceCombo = new QComboBox(panel);
    connect(m_deviceCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::onDeviceSelected);
    panelLayout->addWidget(devLabel);
    panelLayout->addWidget(m_deviceCombo);

    auto *fmtLabel = new QLabel(QStringLiteral("format"), panel);
    m_formatCombo = new QComboBox(panel);
    panelLayout->addWidget(fmtLabel);
    panelLayout->addWidget(m_formatCombo);

    auto *btnRow = new QHBoxLayout;
    m_photoBtn = new QPushButton(QStringLiteral("[ photo ]"), panel);
    m_recordBtn = new QPushButton(QStringLiteral("[ record ]"), panel);
    m_recordBtn->setCheckable(true);
    btnRow->addWidget(m_photoBtn);
    btnRow->addWidget(m_recordBtn);
    panelLayout->addLayout(btnRow);

    connect(m_photoBtn, &QPushButton::clicked, this, &MainWindow::onPhoto);
    connect(m_recordBtn, &QPushButton::clicked, this, &MainWindow::onRecordToggled);

    m_sliders = new ControlSliders(panel);
    panelLayout->addWidget(m_sliders, 1);

    m_statusLabel = new QLabel(QStringLiteral("idle"), panel);
    m_statusLabel->setObjectName(QStringLiteral("status"));
    m_statusLabel->setWordWrap(true);
    panelLayout->addWidget(m_statusLabel);

    splitter->addWidget(panel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({700, 300});

    root->addWidget(splitter);
    setCentralWidget(central);
    resize(1024, 600);
}

void MainWindow::applyStyle()
{
    // Dark, high-contrast, terminal/bitmap aesthetic. Monospace throughout,
    // warm off-white on near-black, orange-brown accent for active states.
    const QString css = QStringLiteral(R"(
* {
    font-size: 12px;
}
QMainWindow, QWidget {
    background: #0d0d0d;
    color: #e8e0d0;
}
QWidget#panel {
    background: #111111;
    border: 1px solid #2a2a2a;
}
QLabel#onAir {
    background: #c86a2e;
    color: #0d0d0d;
    font-weight: bold;
    padding: 4px;
    letter-spacing: 2px;
}
QLabel#muted, QLabel#status {
    color: #8f867a;
}
QPushButton {
    background: #1a1a1a;
    color: #e8e0d0;
    border: 1px solid #3a3a3a;
    padding: 6px 10px;
}
QPushButton:hover {
    border-color: #c86a2e;
}
QPushButton:checked {
    background: #c86a2e;
    color: #0d0d0d;
    border-color: #c86a2e;
}
QComboBox, QLineEdit {
    background: #1a1a1a;
    color: #e8e0d0;
    border: 1px solid #3a3a3a;
    padding: 4px;
}
QComboBox QAbstractItemView {
    background: #1a1a1a;
    color: #e8e0d0;
    selection-background-color: #c86a2e;
    selection-color: #0d0d0d;
}
QSlider::groove:horizontal {
    height: 4px;
    background: #2a2a2a;
}
QSlider::handle:horizontal {
    width: 12px;
    margin: -6px 0;
    background: #c86a2e;
}
QSlider::sub-page:horizontal {
    background: #c86a2e;
}
QCheckBox {
    spacing: 6px;
}
QCheckBox::indicator {
    width: 14px;
    height: 14px;
    background: #1a1a1a;
    border: 1px solid #3a3a3a;
}
QCheckBox::indicator:checked {
    background: #c86a2e;
    border-color: #c86a2e;
}
QSplitter::handle {
    background: #2a2a2a;
}
QToolTip {
    background: #1a1a1a;
    color: #e8e0d0;
    border: 1px solid #3a3a3a;
}
    )").replace(QStringLiteral("* {"),
                QStringLiteral("* {\n    font-family: \"%1\";").arg(qApp->font().family()));
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

    // Start streaming at 640x480, preferred format (MJPEG > YUYV > H264).
    if (!m_capture->start({640, 480}, 0)) {
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
