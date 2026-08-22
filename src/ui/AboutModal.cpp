#include "AboutModal.h"
#include "SvgIcons.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QPixmap>

namespace cmi {

AboutModal::AboutModal(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("aboutModal"));
    hide();
    setFocusPolicy(Qt::StrongFocus);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(40, 30, 40, 30);
    rootLayout->setAlignment(Qt::AlignCenter);

    m_dialogBox = new QWidget(this);
    m_dialogBox->setObjectName(QStringLiteral("aboutDialogBox"));
    m_dialogBox->setFixedWidth(460);
    m_dialogBox->setStyleSheet(QStringLiteral(
        "QWidget#aboutDialogBox {"
        "  background: #161618;"
        "  border: 1px solid rgba(255, 255, 255, 0.16);"
        "  border-radius: 20px;"
        "}"
    ));

    auto *boxLayout = new QVBoxLayout(m_dialogBox);
    boxLayout->setContentsMargins(28, 28, 28, 24);
    boxLayout->setSpacing(14);
    boxLayout->setAlignment(Qt::AlignCenter);

    // Icon Header
    auto *iconLabel = new QLabel(m_dialogBox);
    iconLabel->setAlignment(Qt::AlignCenter);
    QPixmap iconPix(QStringLiteral(":/icons/c-mi.svg"));
    if (iconPix.isNull()) {
        iconPix = QPixmap(QStringLiteral(":/icons/splash.png"));
    }
    if (!iconPix.isNull()) {
        iconLabel->setPixmap(iconPix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        iconLabel->setPixmap(icons::cameraGlyph(Qt::white, 56).pixmap(56, 56));
    }
    boxLayout->addWidget(iconLabel, 0, Qt::AlignCenter);

    // App Name & Tagline
    auto *title = new QLabel(QStringLiteral("Camera Pro"), m_dialogBox);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 18px; font-weight: 700; letter-spacing: 0.02em;"));
    boxLayout->addWidget(title, 0, Qt::AlignCenter);

    auto *tagline = new QLabel(QStringLiteral("c~mi — See It. Make It."), m_dialogBox);
    tagline->setAlignment(Qt::AlignCenter);
    tagline->setStyleSheet(QStringLiteral("color: #0071e3; font-size: 12px; font-weight: 600;"));
    boxLayout->addWidget(tagline, 0, Qt::AlignCenter);

    auto *versionPill = new QLabel(QStringLiteral("Version 0.2.0"), m_dialogBox);
    versionPill->setAlignment(Qt::AlignCenter);
    versionPill->setStyleSheet(QStringLiteral(
        "color: rgba(255, 255, 255, 0.7);"
        "background: rgba(255, 255, 255, 0.08);"
        "border: 1px solid rgba(255, 255, 255, 0.12);"
        "border-radius: 10px;"
        "padding: 3px 10px;"
        "font-size: 11px;"
        "font-weight: 500;"
    ));
    boxLayout->addWidget(versionPill, 0, Qt::AlignCenter);

    // Info Description Card
    auto *infoCard = new QWidget(m_dialogBox);
    infoCard->setStyleSheet(QStringLiteral(
        "background: rgba(255, 255, 255, 0.04);"
        "border: 1px solid rgba(255, 255, 255, 0.08);"
        "border-radius: 12px;"
        "padding: 8px;"
    ));
    auto *cardLayout = new QVBoxLayout(infoCard);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(6);

    auto addDetailRow = [&](const QString &label, const QString &val) {
        auto *row = new QHBoxLayout;
        auto *l = new QLabel(label, infoCard);
        l->setStyleSheet(QStringLiteral("color: #8e8e93; font-size: 11px; font-weight: 600; text-transform: uppercase;"));
        auto *v = new QLabel(val, infoCard);
        v->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 11.5px; font-weight: 500;"));
        v->setAlignment(Qt::AlignRight);
        row->addWidget(l);
        row->addStretch();
        row->addWidget(v);
        cardLayout->addLayout(row);
    };

    addDetailRow(QStringLiteral("Framework"), QStringLiteral("C++17 / Qt6 Widgets"));
    addDetailRow(QStringLiteral("Video Capture"), QStringLiteral("Raw V4L2 ioctl & mmap ring"));
    addDetailRow(QStringLiteral("Encoding"), QStringLiteral("FFmpeg H.264 & AAC"));
    addDetailRow(QStringLiteral("Audio Input"), QStringLiteral("PulseAudio / PipeWire"));
    addDetailRow(QStringLiteral("Renderer"), QStringLiteral("OpenGL Hardware Shaders"));
    addDetailRow(QStringLiteral("License"), QStringLiteral("GNU GPL v3.0"));

    boxLayout->addWidget(infoCard);

    // Action Buttons
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->setAlignment(Qt::AlignCenter);

    m_githubBtn = new QPushButton(QStringLiteral("GitHub"), m_dialogBox);
    m_githubBtn->setCursor(Qt::PointingHandCursor);
    m_githubBtn->setFixedHeight(32);
    m_githubBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: rgba(255, 255, 255, 0.12);"
        "  border: 1px solid rgba(255, 255, 255, 0.18);"
        "  border-radius: 16px;"
        "  color: #ffffff;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  padding: 0 18px;"
        "}"
        "QPushButton:hover { background: rgba(255, 255, 255, 0.22); }"
    ));

    m_closeBtn = new QPushButton(QStringLiteral("Close"), m_dialogBox);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setFixedHeight(32);
    m_closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #0071e3;"
        "  border: none;"
        "  border-radius: 16px;"
        "  color: #ffffff;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  padding: 0 22px;"
        "}"
        "QPushButton:hover { background: #0077ed; }"
    ));

    btnRow->addWidget(m_githubBtn);
    btnRow->addWidget(m_closeBtn);
    boxLayout->addLayout(btnRow);

    rootLayout->addWidget(m_dialogBox);

    connect(m_githubBtn, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/nj2216/c-mi")));
    });

    connect(m_closeBtn, &QPushButton::clicked, this, &AboutModal::hideModal);
}

void AboutModal::showModal()
{
    raise();
    show();
    setFocus();
}

void AboutModal::hideModal()
{
    hide();
}

void AboutModal::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 190));
}

void AboutModal::mousePressEvent(QMouseEvent *event)
{
    if (!m_dialogBox->geometry().contains(event->pos())) {
        hideModal();
    }
    QWidget::mousePressEvent(event);
}

void AboutModal::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hideModal();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace cmi
