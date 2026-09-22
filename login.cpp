#include "login.h"
#include "./ui_login.h"

#include "mainwindow.h"

#include <QMessageBox>


Login::Login(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Login)
    , database(new Database(this))
{
    ui->setupUi(this);

    // Nhấn Enter trong ô mật khẩu cũng đăng nhập
    connect(
        ui->txtPassword,
        &QLineEdit::returnPressed,
        this,
        &Login::on_btnLogin_clicked
        );


    // Khởi tạo SQLite để lấy tài khoản đăng nhập
    if (!database->initialize())
    {
        QMessageBox::critical(
            this,
            "SQLite",
            "Không thể khởi tạo cơ sở dữ liệu tài khoản."
            );
    }
}


Login::~Login()
{
    delete ui;
}


void Login::on_btnLogin_clicked()
{
    QString username =
        ui->txtUsername->text().trimmed();

    QString password =
        ui->txtPassword->text();


    // =====================================================
    // KIỂM TRA DỮ LIỆU NHẬP
    // =====================================================

    if (
        username.isEmpty()
        ||
        password.isEmpty()
        )
    {
        QMessageBox::warning(
            this,
            "Đăng nhập",
            "Vui lòng nhập đầy đủ tên đăng nhập và mật khẩu."
            );

        return;
    }


    QString role;

    // ID của người dùng trong bảng users.
    // Admin không gắn với một người dùng cụ thể
    // nên giá trị thường là -1.
    int userId = -1;


    // =====================================================
    // KIỂM TRA TÀI KHOẢN TRONG SQLITE
    // =====================================================

    if (
        database->authenticateAccount(
            username,
            password,
            role,
            userId
            )
        )
    {
        // =================================================
        // ĐĂNG NHẬP THÀNH CÔNG
        // =================================================

        MainWindow *mainWindow =
            new MainWindow(
                username,
                role,
                userId
                );


        mainWindow->setAttribute(
            Qt::WA_DeleteOnClose
            );


        mainWindow->show();


        // Đóng cửa sổ Login
        this->close();
    }
    else
    {
        // =================================================
        // ĐĂNG NHẬP THẤT BẠI
        // =================================================

        QMessageBox::warning(
            this,
            "Đăng nhập thất bại",
            "Tên đăng nhập hoặc mật khẩu không đúng!"
            );


        ui->txtPassword->clear();

        ui->txtPassword->setFocus();
    }
}


void Login::on_chkShowPassword_toggled(bool checked)
{
    if (checked)
    {
        ui->txtPassword->setEchoMode(
            QLineEdit::Normal
            );
    }
    else
    {
        ui->txtPassword->setEchoMode(
            QLineEdit::Password
            );
    }
}
