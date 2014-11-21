/**************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Installer Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
**
** $QT_END_LICENSE$
**
**************************************************************************/

#include "console.h"
#include "constants.h"
#include "commandlineparser.h"
#include "installerbase.h"
#include "sdkapp.h"
#include "updatechecker.h"

#include <errors.h>
#include <kdselfrestarter.h>
#include <remoteserver.h>
#include <utils.h>

#include <QCommandLineParser>
#include <QDateTime>
#include <QHostAddress>
#include <QNetworkProxyFactory>

#include <iostream>

#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)
#define VERSION "IFW Version: \"" QUOTE(IFW_VERSION_STR) "\""
#define BUILDDATE "Build date: " QUOTE(__DATE__)
#define SHA "Installer Framework SHA1: \"" QUOTE(_GIT_SHA1_) "\""
static const char PLACEHOLDER[32] = "MY_InstallerCreateDateTime_MY";

int main(int argc, char *argv[])
{
    // increase maximum numbers of file descriptors
#if defined (Q_OS_OSX)
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);
    rl.rlim_cur = qMin((rlim_t) OPEN_MAX, rl.rlim_max);
    setrlimit(RLIMIT_NOFILE, &rl);
#endif

    qsrand(QDateTime::currentDateTime().toTime_t());

    // We need to start either a command line application or a GUI application. Since we
    // fail doing so at least on Linux while parsing the argument using a core application
    // object and later starting the GUI application, we now parse the arguments first.
    CommandLineParser parser;
    parser.parse(QInstaller::parseCommandLineArgs(argc, argv));

    QStringList mutually;
    if (parser.isSet(QLatin1String(CommandLineOptions::CheckUpdates)))
        mutually << QLatin1String(CommandLineOptions::CheckUpdates);
    if (parser.isSet(QLatin1String(CommandLineOptions::Updater)))
        mutually << QLatin1String(CommandLineOptions::Updater);
    if (parser.isSet(QLatin1String(CommandLineOptions::ManagePackages)))
        mutually << QLatin1String(CommandLineOptions::ManagePackages);

    const bool help = parser.isSet(QLatin1String(CommandLineOptions::HelpShort))
        || parser.isSet(QLatin1String(CommandLineOptions::HelpLong));
    if (help
            || parser.isSet(QLatin1String(CommandLineOptions::Version))
            || parser.isSet(QLatin1String(CommandLineOptions::FrameworkVersion))
            || mutually.count() > 1) {
        Console c;
        QCoreApplication app(argc, argv);

        if (parser.isSet(QLatin1String(CommandLineOptions::Version))) {
            std::cout << VERSION << std::endl << BUILDDATE << std::endl << SHA << std::endl;
            const QDateTime dateTime = QDateTime::fromString(QLatin1String(PLACEHOLDER),
                QLatin1String("yyyy-MM-dd - HH:mm:ss"));
            if (dateTime.isValid())
                std::cout << "Installer creation time: " << PLACEHOLDER << std::endl;
            return EXIT_SUCCESS;
        }

        if (parser.isSet(QLatin1String(CommandLineOptions::FrameworkVersion))) {
            std::cout << QUOTE(IFW_VERSION_STR) << std::endl;
            return EXIT_SUCCESS;
        }

        std::cout << qPrintable(parser.helpText()) << std::endl;
        if (mutually.count() > 1) {
            std::cerr << qPrintable(QString::fromLatin1("The following options are mutually "
                "exclusive: %1.").arg(mutually.join(QLatin1String(", ")))) << std::endl;
        }
        return help ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (parser.isSet(QLatin1String(CommandLineOptions::StartServer))) {
        const QString argument = parser.value(QLatin1String(CommandLineOptions::StartServer));
        const QString port = argument.section(QLatin1Char(','), 0, 0);
        const QString key = argument.section(QLatin1Char(','), 1, 1);

        QStringList missing;
        if (port.isEmpty())
            missing << QLatin1String("Port");
        if (key.isEmpty())
            missing << QLatin1String("Key");

        SDKApp<QCoreApplication> app(argc, argv);
        if (missing.count()) {
            Console c;
            std::cerr << qPrintable(QString::fromLatin1("Missing argument(s) for option "
                "'startserver': %2").arg(missing.join(QLatin1String(", ")))) << std::endl;
            std::cout << qPrintable(parser.helpText()) << std::endl;
            return EXIT_FAILURE;
        }

        QInstaller::RemoteServer *server = new QInstaller::RemoteServer;
        QObject::connect(server, SIGNAL(destroyed()), &app, SLOT(quit()));
        server->init(port.toInt(), QHostAddress::LocalHost, QInstaller::Protocol::Mode::Release);
        server->setAuthorizationKey(key);
        server->start();
        return app.exec();
    }

    try {
        QScopedPointer<Console> console;
        if (parser.isSet(QLatin1String(CommandLineOptions::VerboseShort))
            || parser.isSet(QLatin1String(CommandLineOptions::VerboseLong))) {
                console.reset(new Console);
                QInstaller::setVerbose(true);
        }

        // On Windows we need the console window from above, we are a GUI application.
        const QStringList unknownOptionNames = parser.unknownOptionNames();
        if (!unknownOptionNames.isEmpty()) {
            const QString options = unknownOptionNames.join(QLatin1String(", "));
            std::cerr << "Unknown option: " << qPrintable(options) << std::endl;
        }

        if (parser.isSet(QLatin1String(CommandLineOptions::Proxy))) {
            // Make sure we honor the system's proxy settings
#if defined(Q_OS_UNIX) && !defined(Q_OS_OSX)
            QUrl proxyUrl(QString::fromLatin1(qgetenv("http_proxy")));
            if (proxyUrl.isValid()) {
                QNetworkProxy proxy(QNetworkProxy::HttpProxy, proxyUrl.host(), proxyUrl.port(),
                    proxyUrl.userName(), proxyUrl.password());
                QNetworkProxy::setApplicationProxy(proxy);
            }
#else
            QNetworkProxyFactory::setUseSystemConfiguration(true);
#endif
        }

        if (parser.isSet(QLatin1String(CommandLineOptions::CheckUpdates)))
            return UpdateChecker(argc, argv).check();

        if (QInstaller::isVerbose())
            std::cout << VERSION << std::endl << BUILDDATE << std::endl << SHA << std::endl;

        const KDSelfRestarter restarter(argc, argv);
        return InstallerBase(argc, argv).run();

    } catch (const QInstaller::Error &e) {
        std::cerr << qPrintable(e.message()) << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception caught." << std::endl;
    }

    return EXIT_FAILURE;
}
