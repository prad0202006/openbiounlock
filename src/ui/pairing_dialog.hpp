#pragma once
#include <QDialog>
#include <QString>
class QLabel;
class QLineEdit;
namespace openbiounlock {
class PairingDialog final : public QDialog {
    Q_OBJECT
public:
    explicit PairingDialog(const QString &pcId, quint16 port, QWidget *parent = nullptr);
private:
    void renderCode(const QString &payload);
    QLabel *codeView_ = nullptr;
    QLineEdit *payloadView_ = nullptr;
};
}