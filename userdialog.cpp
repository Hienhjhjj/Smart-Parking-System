#include "userdialog.h"
#include "./ui_userdialog.h"

UserDialog::UserDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UserDialog)
{
    ui->setupUi(this);

    setWindowTitle("Thông tin người dùng");

    ui->cmbStatus->addItem("Hoạt động");
    ui->cmbStatus->addItem("Không hoạt động");

    ui->spinFingerprint->setMinimum(-1);
    ui->spinFingerprint->setMaximum(10000);
    ui->spinFingerprint->setValue(-1);
}

UserDialog::~UserDialog()
{
    delete ui;
}


// =====================================================
// GET
// =====================================================

QString UserDialog::userCode() const
{
    return ui->txtUserCode->text().trimmed();
}

QString UserDialog::userName() const
{
    return ui->txtUserName->text().trimmed();
}

QString UserDialog::department() const
{
    return ui->txtDepartment->text().trimmed();
}

int UserDialog::fingerprintId() const
{
    return ui->spinFingerprint->value();
}

QString UserDialog::status() const
{
    return ui->cmbStatus->currentText();
}

QString UserDialog::note() const
{
    return ui->txtNote->text().trimmed();
}


// =====================================================
// SET
// =====================================================

void UserDialog::setUserCode(const QString &value)
{
    ui->txtUserCode->setText(value);
}

void UserDialog::setUserName(const QString &value)
{
    ui->txtUserName->setText(value);
}

void UserDialog::setDepartment(const QString &value)
{
    ui->txtDepartment->setText(value);
}

void UserDialog::setFingerprintId(int value)
{
    ui->spinFingerprint->setValue(value);
}

void UserDialog::setStatus(const QString &value)
{
    int index =
        ui->cmbStatus->findText(value);

    if (index >= 0)
    {
        ui->cmbStatus->setCurrentIndex(index);
    }
}

void UserDialog::setNote(const QString &value)
{
    ui->txtNote->setText(value);
}
