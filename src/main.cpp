#include "core/server.hpp"
#include "ui/main_window.hpp"
#include <QApplication>
#include <QMenu>
#include <QSystemTrayIcon>

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("OpenBioUnlock"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setQuitOnLastWindowClosed(false);
    openbiounlock::Server server;
    if (!server.start(43295)) return 1;
    openbiounlock::MainWindow window(&server);
    QSystemTrayIcon tray(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon), &application);
    QMenu trayMenu;
    QAction *openAction = trayMenu.addAction(QStringLiteral("Open dashboard"));
    QAction *quitAction = trayMenu.addAction(QStringLiteral("Quit"));
    tray.setContextMenu(&trayMenu);
    QObject::connect(openAction, &QAction::triggered, &window, [&window] { window.show(); window.raise(); window.activateWindow(); });
    QObject::connect(quitAction, &QAction::triggered, &application, &QCoreApplication::quit);
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &server, &openbiounlock::Server::stop);
    tray.show();
    window.show();
    return application.exec();
}