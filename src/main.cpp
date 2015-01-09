/*
    Copyright 2008 Sascha Peilicke <sasch.pe@gmx.de>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy
    defined in Section 14 of version 3 of the license.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gui/mainwindow.h"

#include <KAboutData>

#include <KLocale>
#include <QApplication>
#include <KLocalizedString>
#include <QCommandLineParser>


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
    KAboutData aboutData("kigo", i18n("Kigo"), "0.5.6",
            i18n("KDE Go Board Game"), KAboutLicense::GPL_V2,
            i18n("Copyright (c) 2008-2010 Sascha Peilicke"));
    aboutData.addAuthor(i18n("Sascha Peilicke (saschpe)"), i18n("Original author"),
                        "sasch.pe@gmx.de", "http://saschpe.wordpress.com");
    aboutData.addCredit(i18n("Yuri Chornoivan"), i18n("Documentation editor"),
                        "yurchor@ukr.net");
    aboutData.addCredit(i18n("Arturo Silva"), i18n("Default theme designer"),
                        "jasilva28@gmail.com");
    aboutData.setHomepage("http://games.kde.org/kigo");

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QStringList() << "game" << i18nc("@info:shell", "Game to load (SGF file)") ) );
    parser.addOption(QCommandLineOption(QStringList() << "+[Url]" << i18nc("@info:shell", "Game to load (SGF file)") ) );

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (app.isSessionRestored()) {
        RESTORE(Kigo::MainWindow)
    } else {

        QString game;
        if (parser.isSet("game")) {
            game = parser.value("game");
        }
        if (parser.positionalArguments().count() == 1) {
            game = parser.positionalArguments().at(0);
        }

        Kigo::MainWindow *mainWin = new Kigo::MainWindow(game, NULL);

        mainWin->show();
    }
    return app.exec();
}
