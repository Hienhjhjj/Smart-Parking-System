#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

#include "bleclient.h"
#include "database.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(
        const QString &username = QString(),
        const QString &role = QString("admin"),
        int userId = -1,
        QWidget *parent = nullptr
        );

    ~MainWindow();

private slots:
    void changePassword();
    void logout();
    void exitApplication();

private:
    Ui::MainWindow *ui;

    BLEClient *bleClient;
    Database *database;


    // =====================================================
    // TÀI KHOẢN ĐĂNG NHẬP
    // =====================================================

    QString currentUsername;
    QString currentRole;

    // ID người dùng tương ứng với tài khoản đăng nhập.
    // Admin thường có giá trị -1.
    int currentUserId;

    void applyPermissions();


    // =====================================================
    // TRẠNG THÁI BÃI XE
    // =====================================================

    int vehiclesInParking;
    int todayAccessCount;
    bool vehicleWaiting;


    // =====================================================
    // NGƯỜI DÙNG
    // =====================================================

    void loadUsers();
    void refreshUserCount();


    // =====================================================
    // NHẬT KÝ
    // =====================================================

    void loadAccessLogs();

    void addRecentLog(
        const QString &time,
        const QString &name,
        const QString &fingerprintId,
        const QString &result,
        const QString &barrier
        );

    void addHistoryLog(
        const QString &time,
        const QString &name,
        const QString &userCode,
        const QString &fingerprintId,
        const QString &result,
        const QString &barrier,
        const QString &direction
        );
};

#endif // MAINWINDOW_H
