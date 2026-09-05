#include "main_window.hpp"
#include "pairing_dialog.hpp"
#include "../core/server.hpp"
#include <QAction>
#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>

namespace openbiounlock {
MainWindow::MainWindow(Server *server, QWidget *parent) : QMainWindow(parent), server_(server) {
    setWindowTitle(QStringLiteral("OpenBioUnlock")); resize(960, 620);
    auto *pair = menuBar()->addAction(QStringLiteral("Pair device")); connect(pair, &QAction::triggered, this, &MainWindow::showPairing);
    auto *central = new QWidget(this); auto *root = new QVBoxLayout(central); auto *header = new QHBoxLayout; auto *title = new QLabel(QStringLiteral("OpenBioUnlock workstation dashboard"), central); QFont titleFont = title->font(); titleFont.setPointSize(16); titleFont.setBold(true); title->setFont(titleFont); header->addWidget(title); statusLabel_ = new QLabel(QStringLiteral("Starting"), central); statusLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter); header->addWidget(statusLabel_, 1); root->addLayout(header);
    auto *splitter = new QSplitter(Qt::Horizontal, central); auto *devicesBox = new QGroupBox(QStringLiteral("Connected devices"), splitter); auto *devicesLayout = new QVBoxLayout(devicesBox); deviceList_ = new QListWidget(devicesBox); devicesLayout->addWidget(deviceList_); auto *pairButton = new QPushButton(QStringLiteral("Pair mobile device"), devicesBox); connect(pairButton, &QPushButton::clicked, this, &MainWindow::showPairing); devicesLayout->addWidget(pairButton);
    auto *logsBox = new QGroupBox(QStringLiteral("Live security log"), splitter); auto *logsLayout = new QVBoxLayout(logsBox); logList_ = new QListWidget(logsBox); logList_->setUniformItemSizes(true); logsLayout->addWidget(logList_); splitter->addWidget(devicesBox); splitter->addWidget(logsBox); splitter->setStretchFactor(1, 1); root->addWidget(splitter, 1); setCentralWidget(central); statusBar()->showMessage(QStringLiteral("Initializing server"));
    connect(server_, &Server::logMessage, this, &MainWindow::appendLog); connect(server_, &Server::statusChanged, this, &MainWindow::setStatus); connect(server_, &Server::deviceChanged, this, &MainWindow::setDevice);
}
void MainWindow::appendLog(const QString &message) { logList_->insertItem(0, QStringLiteral("[%1] %2").arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), message)); while (logList_->count() > 500) delete logList_->takeItem(logList_->count() - 1); }
void MainWindow::setStatus(const QString &status) { statusLabel_->setText(status); statusBar()->showMessage(status); }
void MainWindow::setDevice(const QString &deviceId, bool connected) { for (int index = 0; index < deviceList_->count(); ++index) if (deviceList_->item(index)->data(Qt::UserRole).toString() == deviceId) { if (!connected) delete deviceList_->takeItem(index); return; } if (connected) { auto *item = new QListWidgetItem(QStringLiteral("%1  |  paired").arg(deviceId), deviceList_); item->setData(Qt::UserRole, deviceId); } }
void MainWindow::showPairing() { PairingDialog dialog(QStringLiteral("local-workstation"), server_->port(), this); dialog.exec(); }
}