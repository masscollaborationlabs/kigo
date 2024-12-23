/*
    SPDX-FileCopyrightText: 2008 Sascha Peilicke <sasch.pe@gmx.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "gui/mainwindow.h"

#include "kigo_version.h"

#include <KAboutData>
#include <KCrash>

#include <QApplication>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <KDBusService>
#define HAVE_KICONTHEME __has_include(<KIconTheme>)
#if HAVE_KICONTHEME
#include <KIconTheme>
#endif

#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#include <KStyleManager>
#endif

/**
 * This namespace collects all classes related to Kigo, the Go board game.
 */
namespace Kigo { /* This is only a Doxygen stub */ }

/**
 * The standard C/C++ program entry point. Application 'about' data
 * is created here and command line options are configured and checked
 * if the user invokes the application. If everything is set up, the
 * method displays the main window and jumps into the Qt event loop.
 */
int main(int argc, char *argv[])
{
#if HAVE_KICONTHEME
    KIconTheme::initTheme();
#endif
    QApplication app(argc, argv);
#if HAVE_STYLE_MANAGER
    KStyleManager::initStyle();
#else // !HAVE_STYLE_MANAGER
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    QApplication::setStyle(QStringLiteral("breeze"));
#endif // defined(Q_OS_MACOS) || defined(Q_OS_WIN)
#endif // HAVE_STYLE_MANAGER

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kigo"));

    KAboutData aboutData(QStringLiteral("kigo"), i18n("Kigo"), QStringLiteral(KIGO_VERSION_STRING),
            i18n("KDE Go Board Game"), KAboutLicense::GPL_V2,
            i18n("Copyright (c) 2008-2010 Sascha Peilicke"));
    aboutData.addAuthor(i18n("Sascha Peilicke (saschpe)"), i18n("Original author"),
                        QStringLiteral("sasch.pe@gmx.de"), QStringLiteral("https://saschpe.wordpress.com"));
    aboutData.addCredit(i18n("Yuri Chornoivan"), i18n("Documentation editor"),
                        QStringLiteral("yurchor@ukr.net"));
    aboutData.addCredit(i18n("Arturo Silva"), i18n("Default theme designer"),
                        QStringLiteral("jasilva28@gmail.com"));
    aboutData.setHomepage(QStringLiteral("https://apps.kde.org/kigo"));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.kigo"));

    KAboutData::setApplicationData(aboutData);
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kigo")));

    KCrash::initialize();

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("game"), i18nc("@info:shell", "Game to load (SGF file)"), i18nc("@info:shell", "gamefile")));
    parser.addPositionalArgument(i18nc("@info:shell", "[gamefile]"), i18nc("@info:shell", "Game to load (SGF file)"));
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService service;

    if (app.isSessionRestored()) {
        kRestoreMainWindows<Kigo::MainWindow>();
    } else {

        QString game;
        if (parser.isSet(QStringLiteral("game"))) {
            game = parser.value(QStringLiteral("game"));
        }
        if (parser.positionalArguments().count() == 1) {
            game = parser.positionalArguments().at(0);
        }

        auto mainWin = new Kigo::MainWindow(game, nullptr);

        mainWin->show();
    }
    return app.exec();
}
