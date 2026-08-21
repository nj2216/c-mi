#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QFileInfo>
#include <QSettings>

#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
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

    // Monospace font throughout; fall back gracefully if the preferred font
    // is not installed on the system or bundled with the application.
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QStringList preferred = {QStringLiteral("JetBrains Mono"),
                                   QStringLiteral("DejaVu Sans Mono"),
                                   QStringLiteral("Liberation Mono")};
    for (const QString &family : preferred) {
        if (QFontDatabase::hasFamily(family)) {
            mono = QFont(family);
            break;
        }
    }
    mono.setStyleHint(QFont::Monospace);
    QApplication::setFont(mono);

    cmi::MainWindow w;
    w.show();
    return QApplication::exec();
}
