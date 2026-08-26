#include "TrayIcon.h"

#include <QAction>
#include <QMenu>
#include <QPainter>
#include <QPixmap>

namespace cmi {

TrayIcon::TrayIcon(QObject *parent)
    : QSystemTrayIcon(parent)
{
    auto *menu = new QMenu;
    auto *showAction = menu->addAction(QStringLiteral("Show c~mi"));
    auto *quitAction = menu->addAction(QStringLiteral("Quit"));
    setContextMenu(menu);

    connect(showAction, &QAction::triggered, this, &TrayIcon::showWindowRequested);
    connect(quitAction, &QAction::triggered, this, &TrayIcon::quitRequested);

    updateIcon();
    show();
}

void TrayIcon::setCameraInUse(bool inUse)
{
    m_inUse = inUse;
    updateIcon();
}

void TrayIcon::setRecording(bool recording)
{
    m_recording = recording;
    updateIcon();
}

void TrayIcon::updateIcon()
{
    // Draw a minimal 22x22 indicator: idle = dim outline, in-use = amber dot,
    // recording = orange-brown filled dot with ring (matches app accent).
    QPixmap pm(22, 22);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    QColor accent(0xC8, 0x6A, 0x2E); // orange-brown
    if (m_recording) {
        p.setPen(QPen(accent, 2));
        p.setBrush(accent);
        p.drawEllipse(3, 3, 16, 16);
        setToolTip(QStringLiteral("c~mi - recording"));
    } else if (m_inUse) {
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        p.drawEllipse(6, 6, 10, 10);
        setToolTip(QStringLiteral("c~mi - camera in use"));
    } else {
        QColor dim(0x88, 0x88, 0x88);
        p.setPen(QPen(dim, 2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(4, 4, 14, 14);
        setToolTip(QStringLiteral("c~mi"));
    }
    p.end();
    setIcon(QIcon(pm));
}

} // namespace cmi
