#pragma once
#include <QMainWindow>
class QListWidget;
class QLabel;
namespace openbiounlock {
class Server;
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(Server *server, QWidget *parent = nullptr);
private Q_SLOTS:
    void appendLog(const QString &message);
    void setStatus(const QString &status);
    void setDevice(const QString &deviceId, bool connected);
    void showPairing();
private:
    Server *server_;
    QLabel *statusLabel_ = nullptr;
    QListWidget *deviceList_ = nullptr;
    QListWidget *logList_ = nullptr;
};
}