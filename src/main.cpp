#include <QApplication>
#include <QFont>
#include <QFontDatabase>
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

    // Monospace font throughout; fall back gracefully if JetBrains Mono is
    // not installed on the system.
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
