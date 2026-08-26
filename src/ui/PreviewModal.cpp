#include "PreviewModal.h"
#include "SvgIcons.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

namespace cmi {

PreviewModal::PreviewModal(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("previewModal"));
    hide();
    setFocusPolicy(Qt::StrongFocus);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(40, 30, 40, 30);
    rootLayout->setAlignment(Qt::AlignCenter);

    m_dialogBox = new QWidget(this);
    m_dialogBox->setObjectName(QStringLiteral("dialogBox"));
    m_dialogBox->setMaximumSize(960, 680);
    m_dialogBox->setStyleSheet(QStringLiteral(
        "QWidget#dialogBox {"
        "  background: #111113;"
        "  border: 1px solid rgba(255, 255, 255, 0.15);"
        "  border-radius: 16px;"
        "}"
    ));

    auto *boxLayout = new QVBoxLayout(m_dialogBox);
    boxLayout->setContentsMargins(20, 20, 20, 16);
    boxLayout->setSpacing(14);

    // Center image area with Left and Right nav buttons
    auto *imageRow = new QWidget(m_dialogBox);
    auto *imageRowLayout = new QHBoxLayout(imageRow);
    imageRowLayout->setContentsMargins(0, 0, 0, 0);
    imageRowLayout->setSpacing(12);
    imageRowLayout->setAlignment(Qt::AlignCenter);

    const QString navBtnStyle = QStringLiteral(
        "QPushButton#navBtn {"
        "  background: rgba(255, 255, 255, 0.12);"
        "  border: 1px solid rgba(255, 255, 255, 0.18);"
        "  border-radius: 20px;"
        "}"
        "QPushButton#navBtn:hover {"
        "  background: rgba(255, 255, 255, 0.28);"
        "}"
    );

    m_prevBtn = new QPushButton(imageRow);
    m_prevBtn->setObjectName(QStringLiteral("navBtn"));
    m_prevBtn->setIcon(icons::chevronLeftIcon(Qt::white, 24));
    m_prevBtn->setIconSize(QSize(20, 20));
    m_prevBtn->setCursor(Qt::PointingHandCursor);
    m_prevBtn->setToolTip(QStringLiteral("Previous (Left Arrow)"));
    m_prevBtn->setFixedSize(40, 40);
    m_prevBtn->setStyleSheet(navBtnStyle);

    m_nextBtn = new QPushButton(imageRow);
    m_nextBtn->setObjectName(QStringLiteral("navBtn"));
    m_nextBtn->setIcon(icons::chevronRightIcon(Qt::white, 24));
    m_nextBtn->setIconSize(QSize(20, 20));
    m_nextBtn->setCursor(Qt::PointingHandCursor);
    m_nextBtn->setToolTip(QStringLiteral("Next (Right Arrow)"));
    m_nextBtn->setFixedSize(40, 40);
    m_nextBtn->setStyleSheet(navBtnStyle);

    m_imageLabel = new QLabel(imageRow);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_imageLabel->setMinimumSize(320, 240);
    m_imageLabel->setStyleSheet(QStringLiteral("background: #050506; border-radius: 10px;"));

    imageRowLayout->addWidget(m_prevBtn, 0, Qt::AlignVCenter);
    imageRowLayout->addWidget(m_imageLabel, 1);
    imageRowLayout->addWidget(m_nextBtn, 0, Qt::AlignVCenter);
    boxLayout->addWidget(imageRow, 1);

    m_infoLabel = new QLabel(m_dialogBox);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setStyleSheet(QStringLiteral("color: #8e8e93; font-size: 11px; font-weight: 500;"));
    boxLayout->addWidget(m_infoLabel);

    auto *bar = new QWidget(m_dialogBox);
    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(0, 0, 0, 0);
    barLayout->setSpacing(10);
    barLayout->setAlignment(Qt::AlignCenter);

    m_openBtn = new QPushButton(QStringLiteral("Open"), bar);
    m_exportBtn = new QPushButton(QStringLiteral("Export"), bar);
    m_deleteBtn = new QPushButton(QStringLiteral("Delete"), bar);
    m_closeBtn = new QPushButton(QStringLiteral("Close"), bar);

    for (auto *b : {m_openBtn, m_exportBtn, m_deleteBtn, m_closeBtn}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(32);
    }

    m_openBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: rgba(255, 255, 255, 0.15);"
        "  border: 1px solid rgba(255, 255, 255, 0.2);"
        "  border-radius: 16px;"
        "  color: #ffffff;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  padding: 0 16px;"
        "}"
        "QPushButton:hover { background: rgba(255, 255, 255, 0.28); }"
    ));

    m_exportBtn->setStyleSheet(m_openBtn->styleSheet());

    m_deleteBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: rgba(255, 59, 48, 0.8);"
        "  border: none;"
        "  border-radius: 16px;"
        "  color: #ffffff;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  padding: 0 16px;"
        "}"
        "QPushButton:hover { background: rgba(255, 59, 48, 1.0); }"
    ));

    m_closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: rgba(255, 255, 255, 0.25);"
        "  border: none;"
        "  border-radius: 16px;"
        "  color: #ffffff;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  padding: 0 16px;"
        "}"
        "QPushButton:hover { background: rgba(255, 255, 255, 0.38); }"
    ));

    barLayout->addWidget(m_openBtn);
    barLayout->addWidget(m_exportBtn);
    barLayout->addWidget(m_deleteBtn);
    barLayout->addWidget(m_closeBtn);
    boxLayout->addWidget(bar);

    rootLayout->addWidget(m_dialogBox);

    connect(m_prevBtn, &QPushButton::clicked, this, &PreviewModal::showPrevious);
    connect(m_nextBtn, &QPushButton::clicked, this, &PreviewModal::showNext);

    connect(m_openBtn, &QPushButton::clicked, this, [this] {
        if (m_currentIndex >= 0 && m_currentIndex < static_cast<int>(m_items.size())) {
            const auto &item = m_items[m_currentIndex];
            if (!item.filePath.isEmpty()) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(item.filePath));
            }
        }
    });

    connect(m_exportBtn, &QPushButton::clicked, this, [this] {
        if (m_currentIndex < 0 || m_currentIndex >= static_cast<int>(m_items.size())) return;
        const auto &item = m_items[m_currentIndex];
        if (item.filePath.isEmpty()) return;
        QFileInfo fi(item.filePath);
        QString dest = QFileDialog::getSaveFileName(this, QStringLiteral("Export File"), fi.fileName());
        if (!dest.isEmpty()) {
            QFile::copy(item.filePath, dest);
        }
    });

    connect(m_deleteBtn, &QPushButton::clicked, this, [this] {
        if (m_currentIndex >= 0 && m_currentIndex < static_cast<int>(m_items.size())) {
            MediaItem itemToDelete = m_items[m_currentIndex];
            emit deleteRequested(itemToDelete);
            m_items.erase(m_items.begin() + m_currentIndex);
            if (m_items.empty()) {
                hideModal();
            } else {
                if (m_currentIndex >= static_cast<int>(m_items.size())) {
                    m_currentIndex = static_cast<int>(m_items.size()) - 1;
                }
                updateCurrentDisplay();
            }
        }
    });

    connect(m_closeBtn, &QPushButton::clicked, this, &PreviewModal::hideModal);
}

void PreviewModal::setItems(const std::vector<MediaItem> &items, int currentIndex)
{
    m_items = items;
    if (m_items.empty()) {
        hideModal();
        return;
    }
    m_currentIndex = std::clamp(currentIndex, 0, static_cast<int>(m_items.size()) - 1);
    updateCurrentDisplay();
    raise();
    show();
    setFocus();
}

void PreviewModal::showItem(const MediaItem &item)
{
    setItems({item}, 0);
}

void PreviewModal::showPrevious()
{
    if (m_items.empty()) return;
    if (m_currentIndex > 0) {
        m_currentIndex--;
    } else {
        m_currentIndex = static_cast<int>(m_items.size()) - 1;
    }
    updateCurrentDisplay();
}

void PreviewModal::showNext()
{
    if (m_items.empty()) return;
    if (m_currentIndex + 1 < static_cast<int>(m_items.size())) {
        m_currentIndex++;
    } else {
        m_currentIndex = 0;
    }
    updateCurrentDisplay();
}

void PreviewModal::updateCurrentDisplay()
{
    if (m_currentIndex < 0 || m_currentIndex >= static_cast<int>(m_items.size())) {
        return;
    }

    const MediaItem &item = m_items[m_currentIndex];
    QFileInfo fi(item.filePath);

    if (m_items.size() > 1) {
        m_infoLabel->setText(QStringLiteral("%1 (%2)  •  %3 of %4")
            .arg(fi.fileName())
            .arg(item.type == MediaItem::Type::Video ? QStringLiteral("Video") : QStringLiteral("Photo"))
            .arg(m_currentIndex + 1)
            .arg(m_items.size()));
        m_prevBtn->show();
        m_nextBtn->show();
    } else {
        m_infoLabel->setText(QStringLiteral("%1 (%2)")
            .arg(fi.fileName())
            .arg(item.type == MediaItem::Type::Video ? QStringLiteral("Video") : QStringLiteral("Photo")));
        m_prevBtn->hide();
        m_nextBtn->hide();
    }

    if (item.type == MediaItem::Type::Photo) {
        QPixmap pix(item.filePath);
        if (!pix.isNull()) {
            m_imageLabel->setPixmap(pix.scaled(720, 480, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            m_imageLabel->setText(QStringLiteral("Unable to load image"));
        }
        m_imageLabel->setStyleSheet(QStringLiteral("background: #050506; border-radius: 10px;"));
    } else {
        m_imageLabel->setText(QStringLiteral("Video File: %1\nClick 'Open' to play with system default player.")
            .arg(fi.fileName()));
        m_imageLabel->setStyleSheet(QStringLiteral("background: #050506; border-radius: 10px; color: #ffffff; font-size: 14px; font-weight: 600; text-align: center;"));
    }
}

void PreviewModal::hideModal()
{
    hide();
    m_imageLabel->clear();
}

void PreviewModal::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 190));
}

void PreviewModal::mousePressEvent(QMouseEvent *event)
{
    if (!m_dialogBox->geometry().contains(event->pos())) {
        hideModal();
    }
    QWidget::mousePressEvent(event);
}

void PreviewModal::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hideModal();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace cmi
