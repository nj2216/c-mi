#include "CapturesTray.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QDir>
#include <QFileInfo>
#include <QScrollBar>

namespace cmi {

ThumbnailCard::ThumbnailCard(const MediaItem &item, QWidget *parent)
    : QWidget(parent), m_item(item)
{
    setFixedSize(56, 56);
    setCursor(Qt::PointingHandCursor);

    m_deleteBtn = new QPushButton(QStringLiteral("✕"), this);
    m_deleteBtn->setGeometry(39, -2, 17, 17);
    m_deleteBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #ffffff;"
        "  color: #111111;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 9px;"
        "  font-weight: 800;"
        "  padding: 0px;"
        "}"
        "QPushButton:hover {"
        "  background: #ff3b30;"
        "  color: #ffffff;"
        "}"
    ));
    m_deleteBtn->hide();

    connect(m_deleteBtn, &QPushButton::clicked, this, [this] {
        emit deleteRequested(m_item);
    });
}

void ThumbnailCard::enterEvent(QEnterEvent *)
{
    m_hovered = true;
    m_deleteBtn->show();
    update();
}

void ThumbnailCard::leaveEvent(QEvent *)
{
    m_hovered = false;
    m_deleteBtn->hide();
    update();
}

void ThumbnailCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_item);
    }
    QWidget::mousePressEvent(event);
}

void ThumbnailCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QRectF rect(2, 2, 52, 52);
    QPainterPath path;
    path.addRoundedRect(rect, 10, 10);
    p.setClipPath(path);

    if (!m_item.thumbnail.isNull()) {
        QPixmap pix = QPixmap::fromImage(m_item.thumbnail.scaled(52, 52, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        int offsetX = (pix.width() - 52) / 2;
        int offsetY = (pix.height() - 52) / 2;
        p.drawPixmap(2, 2, pix, offsetX, offsetY, 52, 52);
    } else {
        p.fillRect(rect, QColor(0, 0, 0));
    }

    // Video badge
    if (m_item.type == MediaItem::Type::Video) {
        p.setClipping(false);
        QRectF badgeRect(5, 36, 28, 14);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 190));
        p.drawRoundedRect(badgeRect, 4, 4);

        p.setPen(Qt::white);
        QFont f = p.font();
        f.setPixelSize(8);
        f.setBold(true);
        p.setFont(f);
        p.drawText(badgeRect, Qt::AlignCenter, QStringLiteral("▶ VID"));
    }

    // Border
    p.setClipping(false);
    p.setBrush(Qt::NoBrush);
    if (m_hovered) {
        p.setPen(QPen(Qt::white, 2.0));
    } else {
        p.setPen(QPen(QColor(255, 255, 255, 50), 1.5));
    }
    p.drawRoundedRect(rect, 10, 10);
}

// ---------------- CapturesTray ----------------

CapturesTray::CapturesTray(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("capturesTray"));
    setFixedHeight(78);

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(12);

    // Left metadata column
    auto *metaWrap = new QWidget(this);
    auto *metaLayout = new QVBoxLayout(metaWrap);
    metaLayout->setContentsMargins(0, 2, 10, 2);
    metaLayout->setSpacing(2);

    m_titleLabel = new QLabel(QStringLiteral("ROLL"), metaWrap);
    m_titleLabel->setStyleSheet(QStringLiteral("color: #8e8e93; font-size: 11px; font-weight: 700; letter-spacing: 0.04em;"));
    metaLayout->addWidget(m_titleLabel);

    m_countLabel = new QLabel(QStringLiteral("0 items"), metaWrap);
    m_countLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 12px; font-weight: 600;"));
    metaLayout->addWidget(m_countLabel);

    m_folderBtn = new QPushButton(QStringLiteral("Save All"), metaWrap);
    m_folderBtn->setToolTip(QStringLiteral("Open captured files folder"));
    m_folderBtn->setCursor(Qt::PointingHandCursor);
    m_folderBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: #0071e3;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "  text-align: left;"
        "  padding: 0px;"
        "  margin-top: 2px;"
        "}"
        "QPushButton:hover {"
        "  text-decoration: underline;"
        "}"
    ));
    connect(m_folderBtn, &QPushButton::clicked, this, &CapturesTray::openFolderRequested);
    metaLayout->addWidget(m_folderBtn);

    metaWrap->setStyleSheet(QStringLiteral("border-right: 1px solid rgba(255, 255, 255, 0.12);"));
    mainLayout->addWidget(metaWrap, 0, Qt::AlignVCenter);

    // Scroll area for cards
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:horizontal { height: 4px; background: transparent; margin: 0; }"
        "QScrollBar::handle:horizontal { background: rgba(255, 255, 255, 0.2); border-radius: 2px; min-width: 20px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
    ));

    m_cardContainer = new QWidget(m_scrollArea);
    m_cardContainer->setStyleSheet(QStringLiteral("background: transparent;"));
    m_cardLayout = new QHBoxLayout(m_cardContainer);
    m_cardLayout->setContentsMargins(4, 0, 4, 0);
    m_cardLayout->setSpacing(10);
    m_cardLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_emptyLabel = new QLabel(QStringLiteral("Your captured photos and videos will appear here."), m_cardContainer);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 0.4); font-size: 11.5px; font-style: italic;"));
    m_cardLayout->addWidget(m_emptyLabel);
    m_cardLayout->addWidget(m_emptyLabel);

    m_scrollArea->setWidget(m_cardContainer);
    mainLayout->addWidget(m_scrollArea, 1);

    setStyleSheet(QStringLiteral(
        "QWidget#capturesTray {"
        "  background: rgba(255, 255, 255, 0.06);"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "  border-radius: 14px;"
        "}"
    ));
}

void CapturesTray::updateMeta()
{
    int c = static_cast<int>(m_items.size());
    m_countLabel->setText(c == 1 ? QStringLiteral("1 item") : QStringLiteral("%1 items").arg(c));
    m_emptyLabel->setVisible(c == 0);
}

void CapturesTray::addItem(const MediaItem &item)
{
    m_items.insert(m_items.begin(), item);
    updateMeta();

    auto *card = new ThumbnailCard(item, m_cardContainer);
    connect(card, &ThumbnailCard::clicked, this, &CapturesTray::itemClicked);
    connect(card, &ThumbnailCard::deleteRequested, this, &CapturesTray::itemDeleteRequested);

    m_cardLayout->insertWidget(0, card);

    // Scroll to left
    QScrollBar *sb = m_scrollArea->horizontalScrollBar();
    if (sb) sb->setValue(0);
}

void CapturesTray::removeItem(const QString &filePath)
{
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        if (it->filePath == filePath) {
            m_items.erase(it);
            break;
        }
    }

    for (int i = 0; i < m_cardLayout->count(); ++i) {
        if (auto *card = qobject_cast<ThumbnailCard *>(m_cardLayout->itemAt(i)->widget())) {
            if (card->item().filePath == filePath) {
                m_cardLayout->removeWidget(card);
                card->deleteLater();
                break;
            }
        }
    }

    updateMeta();
}

void CapturesTray::clear()
{
    m_items.clear();
    for (int i = m_cardLayout->count() - 1; i >= 0; --i) {
        if (auto *card = qobject_cast<ThumbnailCard *>(m_cardLayout->itemAt(i)->widget())) {
            m_cardLayout->removeWidget(card);
            card->deleteLater();
        }
    }
    updateMeta();
}

void CapturesTray::scanDirectory(const QString &dir)
{
    QDir d(dir);
    if (!d.exists()) return;

    QFileInfoList entries = d.entryInfoList({QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"),
                                             QStringLiteral("*.png"), QStringLiteral("*.mp4"),
                                             QStringLiteral("*.webm"), QStringLiteral("*.mkv")},
                                            QDir::Files, QDir::Time);

    for (int i = entries.size() - 1; i >= 0; --i) {
        const QFileInfo &fi = entries.at(i);
        bool isVid = fi.suffix().compare(QStringLiteral("mp4"), Qt::CaseInsensitive) == 0 ||
                     fi.suffix().compare(QStringLiteral("webm"), Qt::CaseInsensitive) == 0 ||
                     fi.suffix().compare(QStringLiteral("mkv"), Qt::CaseInsensitive) == 0;

        MediaItem item;
        item.filePath = fi.absoluteFilePath();
        item.type = isVid ? MediaItem::Type::Video : MediaItem::Type::Photo;
        item.timestamp = fi.lastModified();

        if (!isVid) {
            item.thumbnail = QImage(fi.absoluteFilePath());
        } else {
            // Placeholder thumbnail for video
            QImage vidThumb(120, 120, QImage::Format_RGB32);
            vidThumb.fill(QColor(24, 24, 27));
            item.thumbnail = vidThumb;
        }

        addItem(item);
    }
}

} // namespace cmi
