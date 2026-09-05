#include "pairing_dialog.hpp"
#include <QClipboard>
#include <QGuiApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <openssl/sha.h>
#include <qrcodegen.hpp>

namespace openbiounlock {
PairingDialog::PairingDialog(const QString &pcId, quint16 port, QWidget *parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("Pair mobile device")); setModal(true); resize(520, 620);
    const QString payload = QStringLiteral("openbiounlock://pair?pc=%1&port=%2").arg(pcId).arg(port);
    auto *layout = new QVBoxLayout(this); auto *title = new QLabel(QStringLiteral("Scan this pairing code with the OpenBioUnlock mobile app."), this); title->setWordWrap(true); layout->addWidget(title);
    codeView_ = new QLabel(this); codeView_->setAlignment(Qt::AlignCenter); codeView_->setMinimumSize(420, 420); layout->addWidget(codeView_, 1);
    payloadView_ = new QLineEdit(payload, this); payloadView_->setReadOnly(true); layout->addWidget(payloadView_);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this); auto *copy = buttons->addButton(QStringLiteral("Copy payload"), QDialogButtonBox::ActionRole); connect(copy, &QPushButton::clicked, this, [payload] { if (QClipboard *clipboard = QGuiApplication::clipboard()) clipboard->setText(payload); }); connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject); layout->addWidget(buttons);
    renderCode(payload);
}
void PairingDialog::renderCode(const QString &payload) {
    const qrcodegen::QrCode code = qrcodegen::QrCode::encodeText(payload.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM); const int border = 4; const int modules = code.getSize() + border * 2; QImage image(modules, modules, QImage::Format_RGB32); image.fill(Qt::white); QPainter painter(&image); painter.setPen(Qt::NoPen); painter.setBrush(Qt::black); for (int y = 0; y < code.getSize(); ++y) for (int x = 0; x < code.getSize(); ++x) if (code.getModule(x, y)) painter.drawRect(x + border, y + border, 1, 1); painter.end(); codeView_->setPixmap(QPixmap::fromImage(image).scaled(420, 420, Qt::KeepAspectRatio, Qt::FastTransformation));
}
}