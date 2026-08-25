#ifndef CMI_UI_SVGICONS_H
#define CMI_UI_SVGICONS_H

#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace cmi {
namespace icons {

inline QIcon gearIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(color, size * 0.12, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    const qreal center = size / 2.0;
    const qreal outerR = size * 0.38;
    const qreal innerR = size * 0.18;

    p.drawEllipse(QPointF(center, center), innerR, innerR);

    // Draw 6 gear spokes
    for (int i = 0; i < 6; ++i) {
        qreal angle = i * (M_PI / 3.0);
        qreal x1 = center + std::cos(angle) * (innerR * 1.1);
        qreal y1 = center + std::sin(angle) * (innerR * 1.1);
        qreal x2 = center + std::cos(angle) * outerR;
        qreal y2 = center + std::sin(angle) * outerR;
        p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }
    p.end();
    return QIcon(pm);
}

inline QIcon flipIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(color, size * 0.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    const qreal center = size / 2.0;
    const qreal r = size * 0.32;

    QRectF arcRect(center - r, center - r, r * 2, r * 2);
    p.drawArc(arcRect, 45 * 16, 260 * 16);

    // Arrowhead at top
    qreal ax = center + r * std::cos(M_PI / 4.0);
    qreal ay = center - r * std::sin(M_PI / 4.0);
    QPolygonF arrow;
    arrow << QPointF(ax - 2, ay - 4) << QPointF(ax + 3, ay) << QPointF(ax - 2, ay + 4);
    p.setBrush(color);
    p.drawPolygon(arrow);

    p.end();
    return QIcon(pm);
}

inline QIcon closeIcon(const QColor &color = QColor(134, 134, 139), int size = 16)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const qreal pad = size * 0.28;
    p.drawLine(QPointF(pad, pad), QPointF(size - pad, size - pad));
    p.drawLine(QPointF(size - pad, pad), QPointF(pad, size - pad));
    p.end();
    return QIcon(pm);
}

inline QIcon cameraGlyph(const QColor &color = QColor(255, 255, 255, 220), int size = 64)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(color, size * 0.055, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    const qreal pad = size * 0.18;
    QRectF bodyRect(pad, pad * 1.3, size - pad * 2, (size - pad * 2) * 0.72);
    p.drawRoundedRect(bodyRect, 8, 8);

    // Lens circle
    const qreal center = size / 2.0;
    const qreal lensR = size * 0.16;
    p.drawEllipse(QPointF(center, bodyRect.center().y()), lensR, lensR);

    // Flash dot / bump
    p.drawLine(QPointF(center - 10, pad * 1.3), QPointF(center - 6, pad * 0.9));
    p.drawLine(QPointF(center - 6, pad * 0.9), QPointF(center + 6, pad * 0.9));
    p.drawLine(QPointF(center + 6, pad * 0.9), QPointF(center + 10, pad * 1.3));

    p.end();
    return QIcon(pm);
}

inline QIcon videoBadgeIcon(const QColor &color = Qt::white, int size = 16)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(Qt::NoPen);
    p.setBrush(color);

    QPolygonF poly;
    poly << QPointF(size * 0.3, size * 0.2)
         << QPointF(size * 0.8, size * 0.5)
         << QPointF(size * 0.3, size * 0.8);
    p.drawPolygon(poly);
    p.end();
    return QIcon(pm);
}

inline QIcon infoIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(color, size * 0.09, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    const qreal center = size / 2.0;
    const qreal r = size * 0.40;
    p.drawEllipse(QPointF(center, center), r, r);

    // Draw 'i' dot and line
    p.setBrush(color);
    p.drawEllipse(QPointF(center, center - size * 0.18), size * 0.05, size * 0.05);
    p.drawLine(QPointF(center, center - size * 0.05), QPointF(center, center + size * 0.22));
    p.end();
    return QIcon(pm);
}

inline QIcon aspectRatioIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, size * 0.09, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QRectF r(size * 0.15, size * 0.25, size * 0.70, size * 0.50);
    p.drawRoundedRect(r, 3, 3);
    p.setPen(QPen(color, size * 0.08, Qt::DotLine, Qt::RoundCap));
    p.drawLine(QPointF(size * 0.15, size * 0.5), QPointF(size * 0.85, size * 0.5));
    p.end();
    return QIcon(pm);
}

inline QIcon qualityBadgeIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, size * 0.09, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QRectF r(size * 0.12, size * 0.20, size * 0.76, size * 0.60);
    p.drawRoundedRect(r, 4, 4);
    p.setPen(color);
    QFont f = p.font();
    f.setPixelSize(static_cast<int>(size * 0.36));
    f.setBold(true);
    p.setFont(f);
    p.drawText(r, Qt::AlignCenter, QStringLiteral("HD"));
    p.end();
    return QIcon(pm);
}

inline QIcon filtersIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, size * 0.08, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    const qreal r = size * 0.22;
    const qreal cx = size / 2.0;
    const qreal cy = size / 2.0;
    p.drawEllipse(QPointF(cx, cy - r * 0.6), r, r);
    p.drawEllipse(QPointF(cx - r * 0.6, cy + r * 0.5), r, r);
    p.drawEllipse(QPointF(cx + r * 0.6, cy + r * 0.5), r, r);
    p.end();
    return QIcon(pm);
}

inline QIcon timerIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, size * 0.09, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    const qreal cx = size / 2.0;
    const qreal cy = size * 0.54;
    const qreal r = size * 0.36;
    p.drawEllipse(QPointF(cx, cy), r, r);
    p.drawLine(QPointF(cx, size * 0.18), QPointF(cx, size * 0.06));
    p.drawLine(QPointF(cx - size * 0.12, size * 0.06), QPointF(cx + size * 0.12, size * 0.06));
    p.drawLine(QPointF(cx, cy), QPointF(cx, cy - r * 0.65));
    p.drawLine(QPointF(cx, cy), QPointF(cx + r * 0.5, cy));
    p.end();
    return QIcon(pm);
}

inline QIcon noiseReductionIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    auto drawSparkle = [&](qreal cx, qreal cy, qreal rad) {
        QPainterPath path;
        path.moveTo(cx, cy - rad);
        path.quadTo(cx, cy, cx + rad, cy);
        path.quadTo(cx, cy, cx, cy + rad);
        path.quadTo(cx, cy, cx - rad, cy);
        path.quadTo(cx, cy, cx, cy - rad);
        p.drawPath(path);
    };
    drawSparkle(size * 0.50, size * 0.44, size * 0.36);
    drawSparkle(size * 0.78, size * 0.22, size * 0.16);
    drawSparkle(size * 0.22, size * 0.74, size * 0.14);
    p.end();
    return QIcon(pm);
}

inline QIcon togglesIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, size * 0.09, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawLine(QPointF(size * 0.15, size * 0.30), QPointF(size * 0.85, size * 0.30));
    p.drawEllipse(QPointF(size * 0.38, size * 0.30), size * 0.12, size * 0.12);
    p.drawLine(QPointF(size * 0.15, size * 0.70), QPointF(size * 0.85, size * 0.70));
    p.drawEllipse(QPointF(size * 0.64, size * 0.70), size * 0.12, size * 0.12);
    p.end();
    return QIcon(pm);
}

inline QIcon chevronLeftIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, size * 0.11, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(size * 0.62, size * 0.22);
    path.lineTo(size * 0.35, size * 0.50);
    path.lineTo(size * 0.62, size * 0.78);
    p.drawPath(path);
    p.end();
    return QIcon(pm);
}

inline QIcon chevronRightIcon(const QColor &color = Qt::white, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, size * 0.11, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(size * 0.38, size * 0.22);
    path.lineTo(size * 0.65, size * 0.50);
    path.lineTo(size * 0.38, size * 0.78);
    p.drawPath(path);
    p.end();
    return QIcon(pm);
}

} // namespace icons
} // namespace cmi

#endif // CMI_UI_SVGICONS_H
