#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QFileInfo>
#include <QSettings>

extern "C" {
#include <libavutil/log.h>
}

#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    // FFmpeg logs warnings (e.g. malformed MJPEG APPn markers from some
    // webcams, deprecated pixel-format notices) on every frame; these are
    // benign noise for a live preview, so only surface real errors.
    av_log_set_level(AV_LOG_ERROR);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("c-mi"));
    QApplication::setOrganizationName(QStringLiteral("c-mi"));
    QApplication::setDesktopFileName(QStringLiteral("c-mi"));
    QApplication::setQuitOnLastWindowClosed(false); // keep running for tray

    // QSettings INI backend only (portable, no dconf/GSettings).
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // Load the packaged fallback before selecting the application font. This
    // keeps text rendering independent of the target machine's font setup.
    const QStringList bundledFonts = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/../lib/c-mi/DejaVuSansMono.ttf"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../share/fonts/c-mi/DejaVuSansMono.ttf")
    };
    for (const QString &path : bundledFonts) {
        if (QFileInfo::exists(path))
            QFontDatabase::addApplicationFont(path);
    }

    // Prefer modern clean sans-serif UI typography; fallback gracefully
    QFont appFont = QApplication::font();
    const QStringList preferred = {
        QStringLiteral("SF Pro Display"),
        QStringLiteral("SF Pro Text"),
        QStringLiteral("Inter"),
        QStringLiteral("Ubuntu"),
        QStringLiteral("DejaVu Sans"),
        QStringLiteral("Liberation Sans"),
        QStringLiteral("Cantarell"),
        QStringLiteral("Segoe UI"),
        QStringLiteral("Noto Sans")
    };
    for (const QString &family : preferred) {
        if (QFontDatabase::hasFamily(family)) {
            appFont = QFont(family);
            break;
        }
    }
    appFont.setStyleHint(QFont::SansSerif);
    appFont.setPixelSize(13);
    QApplication::setFont(appFont);

    cmi::MainWindow w;
    w.show();
    return QApplication::exec();
}
