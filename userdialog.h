#ifndef USERDIALOG_H
#define USERDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class UserDialog;
}
QT_END_NAMESPACE

class UserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserDialog(QWidget *parent = nullptr);
    ~UserDialog();

    QString userCode() const;
    QString userName() const;
    QString department() const;
    int fingerprintId() const;
    QString status() const;
    QString note() const;

    void setUserCode(const QString &value);
    void setUserName(const QString &value);
    void setDepartment(const QString &value);
    void setFingerprintId(int value);
    void setStatus(const QString &value);
    void setNote(const QString &value);

private:
    Ui::UserDialog *ui;
};

#endif // USERDIALOG_H
