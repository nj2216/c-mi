#ifndef CMI_UI_TOGGLESWITCH_H
#define CMI_UI_TOGGLESWITCH_H

#include <QAbstractButton>
#include <QPropertyAnimation>

namespace cmi {

class ToggleSwitch : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(qreal knobOffset READ knobOffset WRITE setKnobOffset)

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    qreal knobOffset() const { return m_offset; }
    void setKnobOffset(qreal offset);

protected:
    void paintEvent(QPaintEvent *event) override;
    void nextCheckState() override;

private:
    qreal m_offset = 0.0;
    QPropertyAnimation *m_anim = nullptr;
};

} // namespace cmi

#endif // CMI_UI_TOGGLESWITCH_H
