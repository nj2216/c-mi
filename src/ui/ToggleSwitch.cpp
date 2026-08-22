#include "ToggleSwitch.h"

#include <QPainter>
#include <QPainterPath>

namespace cmi {

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setChecked(false);
    setCursor(Qt::PointingHandCursor);

    m_anim = new QPropertyAnimation(this, "knobOffset", this);
    m_anim->setDuration(160);
    m_anim->setEasingCurve(QEasingCurve::InOutQuad);
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(38, 22);
}

QSize ToggleSwitch::minimumSizeHint() const
{
    return QSize(38, 22);
}

void ToggleSwitch::setKnobOffset(qreal offset)
{
    m_offset = offset;
    update();
}

void ToggleSwitch::nextCheckState()
{
    QAbstractButton::nextCheckState();
    m_anim->stop();
    m_anim->setStartValue(m_offset);
    m_anim->setEndValue(isChecked() ? 1.0 : 0.0);
    m_anim->start();
}

void ToggleSwitch::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal w = width();
    const qreal h = height();
    const qreal radius = h / 2.0;

    // Background track
    QColor offBg(0, 0, 0, 38);
    QColor onBg(52, 199, 89); // Apple green #34c759

    int r = static_cast<int>(offBg.red() + (onBg.red() - offBg.red()) * m_offset);
    int g = static_cast<int>(offBg.green() + (onBg.green() - offBg.green()) * m_offset);
    int b = static_cast<int>(offBg.blue() + (onBg.blue() - offBg.blue()) * m_offset);
    int a = static_cast<int>(offBg.alpha() + (onBg.alpha() - offBg.alpha()) * m_offset);
    QColor trackColor(r, g, b, a);

    p.setPen(Qt::NoPen);
    p.setBrush(trackColor);
    p.drawRoundedRect(QRectF(0, 0, w, h), radius, radius);

    // Knob
    const qreal knobPadding = 2.5;
    const qreal knobDiameter = h - knobPadding * 2.0;
    const qreal minX = knobPadding;
    const qreal maxX = w - knobPadding - knobDiameter;
    const qreal knobX = minX + (maxX - minX) * m_offset;

    // Knob shadow
    p.setBrush(QColor(0, 0, 0, 40));
    p.drawEllipse(QRectF(knobX, knobPadding + 0.8, knobDiameter, knobDiameter));

    // Knob body
    p.setBrush(Qt::white);
    p.drawEllipse(QRectF(knobX, knobPadding, knobDiameter, knobDiameter));
}

} // namespace cmi
