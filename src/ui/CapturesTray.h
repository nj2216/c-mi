#ifndef CMI_UI_CAPTURESTRAY_H
#define CMI_UI_CAPTURESTRAY_H

#include <QWidget>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QImage>
#include <vector>

namespace cmi {

struct MediaItem {
    enum class Type { Photo, Video };
    QString filePath;
    QImage thumbnail;
    Type type = Type::Photo;
    QDateTime timestamp;
};

class ThumbnailCard : public QWidget {
    Q_OBJECT
public:
    explicit ThumbnailCard(const MediaItem &item, QWidget *parent = nullptr);

    const MediaItem &item() const { return m_item; }

signals:
    void clicked(const MediaItem &item);
    void deleteRequested(const MediaItem &item);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    MediaItem m_item;
    QPushButton *m_deleteBtn = nullptr;
    bool m_hovered = false;
};

class CapturesTray : public QWidget {
    Q_OBJECT
public:
    explicit CapturesTray(QWidget *parent = nullptr);

    void addItem(const MediaItem &item);
    void removeItem(const QString &filePath);
    void clear();
    void scanDirectory(const QString &dir);
    int count() const { return static_cast<int>(m_items.size()); }

signals:
    void itemClicked(const MediaItem &item);
    void itemDeleteRequested(const MediaItem &item);
    void openFolderRequested();

private:
    void updateMeta();

    QLabel *m_titleLabel = nullptr;
    QLabel *m_countLabel = nullptr;
    QPushButton *m_folderBtn = nullptr;
    QWidget *m_cardContainer = nullptr;
    QHBoxLayout *m_cardLayout = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QScrollArea *m_scrollArea = nullptr;

    std::vector<MediaItem> m_items;
};

} // namespace cmi

#endif // CMI_UI_CAPTURESTRAY_H
