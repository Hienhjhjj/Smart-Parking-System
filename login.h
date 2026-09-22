#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QString>

#include "database.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Login;
}
QT_END_NAMESPACE

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

private slots:
    void on_btnLogin_clicked();
    void on_chkShowPassword_toggled(bool checked);

private:
    Ui::Login *ui;
    Database *database;
};

#endif // LOGIN_H
