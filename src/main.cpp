/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha.

    This game engine is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the LICENSE file for more details.

    Mogara
*********************************************************************/

#include <QGuiApplication>
#include <QLocale>
#include <QTranslator>

#include <cmainwindow.h>
#include <cexceptionhandler.h>
#include <cnetwork.h>
#include <ctranslator.h>

int main(int argc, char *argv[])
{
    MCD_INIT_GUI;

    CExceptionHandler eh("./dmp");

    QGuiApplication app(argc, argv);

    app.setOrganizationName("Mogara");
    app.setOrganizationDomain("mogara.org");
    app.setApplicationName("QSanguosha");

    QString localeName = QLocale::system().name();
    QTranslator translator;
    if (!translator.load(localeName, QStringLiteral("translations"))) {
        localeName = "zh_CN";
        translator.load(localeName, QStringLiteral("translations"));
    }
    CTranslator ctranslator;
    if (ctranslator.load(localeName, QStringLiteral("translations")))
        app.installTranslator(&ctranslator);
    app.installTranslator(&translator);

    CMainWindow window;
    window.setIcon(QIcon(QSanguoshaSource"image/mogara/logo.png"));
    window.setSource(QUrl(QStringLiteral(QSanguoshaSource"script/main.qml")));
    window.show();

    cRegisterUrlScheme(window.title());

    return app.exec();
}
