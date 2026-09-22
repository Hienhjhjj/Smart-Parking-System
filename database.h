#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVariantList>

class Database : public QObject
{
    Q_OBJECT

public:

    explicit Database(QObject *parent = nullptr);

    // =====================================================
    // DATABASE
    // =====================================================

    bool initialize();


    // =====================================================
    // TÀI KHOẢN / PHÂN QUYỀN
    // =====================================================

    bool authenticateAccount(
        const QString &username,
        const QString &password,
        QString &role,
        int &userId
        );

    bool changePassword(
        const QString &username,
        const QString &oldPassword,
        const QString &newPassword
        );


    // =====================================================
    // NGƯỜI DÙNG
    // =====================================================

    int userCount();

    bool addUser(
        const QString &userCode,
        const QString &name,
        const QString &department,
        int fingerprintId,
        const QString &status,
        const QString &note
        );

    bool updateUser(
        int id,
        const QString &userCode,
        const QString &name,
        const QString &department,
        int fingerprintId,
        const QString &status,
        const QString &note
        );

    bool deleteUser(int id);

    QVariantList getUsers();

    QVariantList searchUsers(
        const QString &keyword
        );

    QVariantList getUserById(
        int id
        );

    QVariantList getUserByFingerprintId(
        int fingerprintId
        );


    // =====================================================
    // NHẬT KÝ RA VÀO
    // =====================================================

    bool addAccessLog(
        const QString &userCode,
        const QString &name,
        int fingerprintId,
        const QString &result,
        const QString &barrier,
        const QString &direction
        );

    QVariantList getAccessLogs(
        int limit = 100
        );

    QVariantList getAccessLogsForUser(
        const QString &userCode,
        int limit = 100
        );


private:

    QSqlDatabase db;
};

#endif // DATABASE_H
