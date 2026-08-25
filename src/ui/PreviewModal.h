#ifndef CMI_UI_PREVIEWMODAL_H
#define CMI_UI_PREVIEWMODAL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <vector>
#include "CapturesTray.h"

namespace cmi {

class PreviewModal : public QWidget {
    Q_OBJECT
public:
    explicit PreviewModal(QWidget *parent = nullptr);

    void setItems(const std::vector<MediaItem> &items, int currentIndex = 0);
    void showItem(const MediaItem &item);
    void showNext();
    void showPrevious();
    void hideModal();

signals:
    void deleteRequested(const MediaItem &item);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    MediaItem m_item;
    void updateCurrentDisplay();

    std::vector<MediaItem> m_items;
    int m_currentIndex = -1;

    QWidget *m_dialogBox = nullptr;
    QPushButton *m_prevBtn = nullptr;
    QPushButton *m_nextBtn = nullptr;
    QLabel *m_imageLabel = nullptr;
    QLabel *m_infoLabel = nullptr;
    QPushButton *m_openBtn = nullptr;
    QPushButton *m_exportBtn = nullptr;
    QPushButton *m_deleteBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
};

} // namespace cmi

#endif // CMI_UI_PREVIEWMODAL_H
