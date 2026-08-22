#ifndef CMI_UI_ABOUTMODAL_H
#define CMI_UI_ABOUTMODAL_H

#include <QWidget>

class QLabel;
class QPushButton;

namespace cmi {

class AboutModal : public QWidget {
    Q_OBJECT
public:
    explicit AboutModal(QWidget *parent = nullptr);

    void showModal();
    void hideModal();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QWidget *m_dialogBox = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QPushButton *m_githubBtn = nullptr;
};

} // namespace cmi

#endif // CMI_UI_ABOUTMODAL_H
