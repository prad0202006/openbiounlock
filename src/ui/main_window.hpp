#pragma once
#include <QMainWindow>
class QListWidget;
class QLabel;
class QSpinBox;
namespace openbiounlock {
class Server;
class BleDiscovery;
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(Server *server, QWidget *parent = nullptr);
private Q_SLOTS:
    void appendLog(const QString &message);
    void setStatus(const QString &status);
    void setDevice(const QString &deviceId, bool connected);
    void showPairing();
    void proximityChanged(int value);
private:
    Server *server_;
    BleDiscovery *bleDiscovery_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QListWidget *deviceList_ = nullptr;
    QListWidget *logList_ = nullptr;
    QSpinBox *nearThreshold_ = nullptr;
    QSpinBox *farThreshold_ = nullptr;
};
}