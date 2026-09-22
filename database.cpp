#include "database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QStringList>


// =====================================================
// CONSTRUCTOR
// =====================================================

Database::Database(QObject *parent)
    : QObject(parent)
{
}


// =====================================================
// KHỞI TẠO DATABASE
// =====================================================

bool Database::initialize()
{
    // =================================================
    // KẾT NỐI SQLITE
    // =================================================

    if (QSqlDatabase::contains("ParkingSystemDB"))
    {
        db = QSqlDatabase::database(
            "ParkingSystemDB"
            );
    }
    else
    {
        db = QSqlDatabase::addDatabase(
            "QSQLITE",
            "ParkingSystemDB"
            );

        db.setDatabaseName(
            "parking.db"
            );
    }


    // =================================================
    // MỞ DATABASE
    // =================================================

    if (!db.open())
    {
        qDebug()
            << "[SQLite] Open error:"
            << db.lastError().text();

        return false;
    }


    QSqlQuery query(db);


    // =================================================
    // TẠO BẢNG ACCOUNTS
    // =================================================

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS accounts ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT NOT NULL UNIQUE,"
            "password TEXT NOT NULL,"
            "role TEXT NOT NULL,"
            "status TEXT DEFAULT 'Hoạt động',"
            "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
            ")"
            ))
    {
        qDebug()
            << "[SQLite] Create accounts error:"
            << query.lastError().text();

        return false;
    }


    // =================================================
    // LIÊN KẾT TÀI KHOẢN VỚI NGƯỜI DÙNG
    // =================================================
    // Database cũ có thể chưa có cột user_id.
    // Bổ sung cột này mà không làm mất dữ liệu hiện có.

    if (!query.exec(
            "PRAGMA table_info(accounts)"
            ))
    {
        qDebug()
            << "[SQLite] Check accounts columns error:"
            << query.lastError().text();

        return false;
    }

    bool hasUserIdColumn = false;

    while (query.next())
    {
        if (query.value(1).toString() == "user_id")
        {
            hasUserIdColumn = true;
            break;
        }
    }

    if (!hasUserIdColumn)
    {
        if (!query.exec(
                "ALTER TABLE accounts "
                "ADD COLUMN user_id INTEGER"
                ))
        {
            qDebug()
                << "[SQLite] Add accounts.user_id error:"
                << query.lastError().text();

            return false;
        }

        qDebug()
            << "[SQLite] Added accounts.user_id";
    }


    // Tạo các tài khoản mặc định nếu chưa tồn tại.
    //
    // admin  -> tài khoản quản trị
    // user1  -> DTC01
    // user2  -> DTC2
    // user3  -> DTC03
    // user4  -> DTC04
    // user5  -> DTC05
    //
    // Mật khẩu ban đầu: 123456
    //
    // Database cũ có thể có "nhanvien" hoặc "user".
    // Nếu user1 chưa tồn tại, đổi tên tài khoản cũ thành user1
    // để không làm mất tài khoản hiện tại.

    if (!query.exec(
            "INSERT OR IGNORE INTO accounts "
            "(username, password, role, status) "
            "VALUES ('admin', '123456', 'admin', 'Hoạt động')"
            ))
    {
        qDebug()
            << "[SQLite] Create admin account error:"
            << query.lastError().text();

        return false;
    }

    if (!query.exec(
            "UPDATE accounts SET username = 'user1' "
            "WHERE username = 'nhanvien' "
            "AND NOT EXISTS (SELECT 1 FROM accounts WHERE username = 'user1')"
            ))
    {
        qDebug()
            << "[SQLite] Rename nhanvien -> user1 error:"
            << query.lastError().text();

        return false;
    }

    if (!query.exec(
            "UPDATE accounts SET username = 'user1' "
            "WHERE username = 'user' "
            "AND NOT EXISTS (SELECT 1 FROM accounts WHERE username = 'user1')"
            ))
    {
        qDebug()
            << "[SQLite] Rename user -> user1 error:"
            << query.lastError().text();

        return false;
    }

    const QStringList accountSql = {
                                    "INSERT OR IGNORE INTO accounts "
                                    "(username, password, role, status) "
                                    "VALUES ('user1', '123456', 'staff', 'Hoạt động')",

                                    "INSERT OR IGNORE INTO accounts "
                                    "(username, password, role, status) "
                                    "VALUES ('user2', '123456', 'staff', 'Hoạt động')",

                                    "INSERT OR IGNORE INTO accounts "
                                    "(username, password, role, status) "
                                    "VALUES ('user3', '123456', 'staff', 'Hoạt động')",

                                    "INSERT OR IGNORE INTO accounts "
                                    "(username, password, role, status) "
                                    "VALUES ('user4', '123456', 'staff', 'Hoạt động')",

                                    "INSERT OR IGNORE INTO accounts "
                                    "(username, password, role, status) "
                                    "VALUES ('user5', '123456', 'staff', 'Hoạt động')"
    };

    for (const QString &sql : accountSql)
    {
        if (!query.exec(sql))
        {
            qDebug()
                << "[SQLite] Create staff account error:"
                << query.lastError().text();

            return false;
        }
    }


    // KHÔI PHỤC MẬT KHẨU MẶC ĐỊNH MỘT LẦN
    // =================================================
    // Do phiên bản trước có thể đã ghi sai mật khẩu khi đổi
    // mật khẩu, thực hiện khôi phục admin/user về 123456
    // đúng một lần. Sau khi đánh dấu hoàn tất, mật khẩu mới
    // do người dùng đổi sẽ được giữ nguyên ở các lần chạy sau.

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS account_migration ("
            "key TEXT PRIMARY KEY"
            ")"
            ))
    {
        qDebug()
            << "[SQLite] Create account migration error:"
            << query.lastError().text();

        return false;
    }

    if (!query.exec(
            "SELECT key FROM account_migration "
            "WHERE key = 'password_reset_v2'"
            ))
    {
        qDebug()
            << "[SQLite] Check password migration error:"
            << query.lastError().text();

        return false;
    }

    if (!query.next())
    {
        QSqlQuery resetQuery(db);

        if (!resetQuery.exec(
                "UPDATE accounts SET password = '123456', status = 'Hoạt động' "
                "WHERE username IN ('admin', 'user1')"
                ))
        {
            qDebug()
                << "[SQLite] Reset default passwords error:"
                << resetQuery.lastError().text();

            return false;
        }

        if (!query.exec(
                "INSERT INTO account_migration (key) "
                "VALUES ('password_reset_v2')"
                ))
        {
            qDebug()
                << "[SQLite] Save password migration error:"
                << query.lastError().text();

            return false;
        }

        qDebug()
            << "[SQLite] Default passwords restored:"
            << "admin / 123456"
            << "user1 / 123456";
    }


    // =================================================
    // TẠO BẢNG USERS
    // =================================================

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "user_code TEXT NOT NULL UNIQUE,"
            "name TEXT NOT NULL,"
            "department TEXT,"
            "fingerprint_id INTEGER UNIQUE,"
            "status TEXT DEFAULT 'Hoạt động',"
            "note TEXT,"
            "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
            ")"
            ))
    {
        qDebug()
            << "[SQLite] Create users error:"
            << query.lastError().text();

        return false;
    }


    // =================================================
    // =================================================
    // LIÊN KẾT TÀI KHOẢN VỚI NGƯỜI DÙNG
    // =================================================
    //
    // user1 -> DTC01
    // user2 -> DTC2
    // user3 -> DTC03
    // user4 -> DTC04
    // user5 -> DTC05
    //
    // Tìm ID thực tế bằng user_code, không hard-code ID SQLite.

    struct AccountUserMapping
    {
        const char *username;
        const char *userCode;
    };

    const AccountUserMapping mappings[] = {
        {"user1", "DTC01"},
        {"user2", "DTC02"},
        {"user3", "DTC03"},
        {"user4", "DTC04"},
        {"user5", "DTC05"}
    };

    for (const AccountUserMapping &mapping : mappings)
    {
        QSqlQuery linkQuery(db);

        linkQuery.prepare(
            "UPDATE accounts "
            "SET user_id = ("
            "    SELECT id FROM users WHERE user_code = ?"
            ") "
            "WHERE username = ? "
            "AND EXISTS ("
            "    SELECT 1 FROM users WHERE user_code = ?"
            ")"
            );

        linkQuery.addBindValue(
            QString::fromUtf8(mapping.userCode)
            );

        linkQuery.addBindValue(
            QString::fromUtf8(mapping.username)
            );

        linkQuery.addBindValue(
            QString::fromUtf8(mapping.userCode)
            );

        if (!linkQuery.exec())
        {
            qDebug()
                << "[SQLite] Link account -> user error:"
                << mapping.username
                << linkQuery.lastError().text();

            return false;
        }
    }


    // TẠO BẢNG ACCESS LOGS
    // =================================================

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS access_logs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "user_code TEXT,"
            "name TEXT,"
            "fingerprint_id INTEGER,"
            "result TEXT,"
            "barrier TEXT,"
            "direction TEXT,"
            "access_time TEXT"
            ")"
            ))
    {
        qDebug()
            << "[SQLite] Create access_logs error:"
            << query.lastError().text();

        return false;
    }


    // =================================================
    // SỬA DỮ LIỆU NHẬT KÝ CŨ THEO ID VÂN TAY
    // =================================================
    //
    // Một số bản ghi access_logs được tạo trước khi cơ chế
    // liên kết tài khoản/người dùng hoàn chỉnh. Khi đó user_code
    // và name có thể không khớp với fingerprint_id.
    //
    // Chỉ thực hiện một lần. Với các log có fingerprint_id hợp lệ,
    // lấy lại user_code và name từ bảng users theo fingerprint_id.

    if (!query.exec(
            "SELECT key FROM account_migration "
            "WHERE key = 'access_log_repair_v1'"
            ))
    {
        qDebug()
            << "[SQLite] Check access log repair error:"
            << query.lastError().text();

        return false;
    }

    if (!query.next())
    {
        QSqlQuery repairQuery(db);

        if (!repairQuery.exec(
                "UPDATE access_logs "
                "SET user_code = ("
                "    SELECT user_code FROM users "
                "    WHERE users.fingerprint_id = access_logs.fingerprint_id"
                "), "
                "name = ("
                "    SELECT name FROM users "
                "    WHERE users.fingerprint_id = access_logs.fingerprint_id"
                ") "
                "WHERE fingerprint_id > 0 "
                "AND EXISTS ("
                "    SELECT 1 FROM users "
                "    WHERE users.fingerprint_id = access_logs.fingerprint_id"
                ")"
                ))
        {
            qDebug()
                << "[SQLite] Repair access logs error:"
                << repairQuery.lastError().text();

            return false;
        }

        const int repairedRows =
            repairQuery.numRowsAffected();

        if (!query.exec(
                "INSERT INTO account_migration (key) "
                "VALUES ('access_log_repair_v1')"
                ))
        {
            qDebug()
                << "[SQLite] Save access log repair error:"
                << query.lastError().text();

            return false;
        }

        qDebug()
            << "[SQLite] Repaired access logs:"
            << repairedRows;
    }


    qDebug()
        << "[SQLite] Database ready.";

    return true;
}


// =====================================================
// XÁC THỰC TÀI KHOẢN
// =====================================================

bool Database::authenticateAccount(
    const QString &username,
    const QString &password,
    QString &role,
    int &userId
    )
{
    QSqlQuery query(db);

    query.prepare(
        "SELECT role, "
        "COALESCE(user_id, -1) "
        "FROM accounts "
        "WHERE username = ? "
        "AND password = ? "
        "AND status = 'Hoạt động'"
        );

    query.addBindValue(username);
    query.addBindValue(password);

    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Authenticate error:"
            << query.lastError().text();

        return false;
    }

    if (!query.next())
    {
        return false;
    }

    role = query.value(0).toString();
    userId = query.value(1).toInt();

    return true;
}

// =====================================================
// ĐỔI MẬT KHẨU
// =====================================================

bool Database::changePassword(
    const QString &username,
    const QString &oldPassword,
    const QString &newPassword
    )
{
    QSqlQuery query(db);

    query.prepare(
        "UPDATE accounts "
        "SET password = ? "
        "WHERE username = ? "
        "AND password = ? "
        "AND status = 'Hoạt động'"
        );

    query.addBindValue(newPassword);
    query.addBindValue(username);
    query.addBindValue(oldPassword);

    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Change password error:"
            << query.lastError().text();

        return false;
    }

    return query.numRowsAffected() == 1;
}


// =====================================================
// ĐẾM NGƯờI DÙNG
// =====================================================

int Database::userCount()
{
    QSqlQuery query(db);

    if (!query.exec(
            "SELECT COUNT(*) "
            "FROM users"
            ))
    {
        qDebug()
            << "[SQLite] Count error:"
            << query.lastError().text();

        return 0;
    }


    if (query.next())
    {
        return query.value(0).toInt();
    }


    return 0;
}


// =====================================================
// THÊM NGƯờI DÙNG
// =====================================================

bool Database::addUser(
    const QString &userCode,
    const QString &name,
    const QString &department,
    int fingerprintId,
    const QString &status,
    const QString &note
    )
{
    QSqlQuery query(db);

    query.prepare(
        "INSERT INTO users "
        "(user_code, name, department, "
        "fingerprint_id, status, note) "
        "VALUES (?, ?, ?, ?, ?, ?)"
        );


    query.addBindValue(userCode);
    query.addBindValue(name);
    query.addBindValue(department);


    if (fingerprintId >= 0)
    {
        query.addBindValue(fingerprintId);
    }
    else
    {
        query.addBindValue(QVariant());
    }


    query.addBindValue(status);
    query.addBindValue(note);


    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Add user error:"
            << query.lastError().text();

        return false;
    }


    return true;
}


// =====================================================
// SỬA NGƯờI DÙNG
// =====================================================

bool Database::updateUser(
    int id,
    const QString &userCode,
    const QString &name,
    const QString &department,
    int fingerprintId,
    const QString &status,
    const QString &note
    )
{
    QSqlQuery query(db);

    query.prepare(
        "UPDATE users SET "
        "user_code = ?, "
        "name = ?, "
        "department = ?, "
        "fingerprint_id = ?, "
        "status = ?, "
        "note = ? "
        "WHERE id = ?"
        );


    query.addBindValue(userCode);
    query.addBindValue(name);
    query.addBindValue(department);


    if (fingerprintId >= 0)
    {
        query.addBindValue(fingerprintId);
    }
    else
    {
        query.addBindValue(QVariant());
    }


    query.addBindValue(status);
    query.addBindValue(note);
    query.addBindValue(id);


    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Update user error:"
            << query.lastError().text();

        return false;
    }


    return true;
}


// =====================================================
// XÓA NGƯờI DÙNG
// =====================================================

bool Database::deleteUser(int id)
{
    QSqlQuery query(db);

    query.prepare(
        "DELETE FROM users "
        "WHERE id = ?"
        );


    query.addBindValue(id);


    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Delete user error:"
            << query.lastError().text();

        return false;
    }


    return true;
}


// =====================================================
// LẤY TẤT CẢ NGƯờI DÙNG
// =====================================================

QVariantList Database::getUsers()
{
    QVariantList users;

    QSqlQuery query(db);


    if (!query.exec(
            "SELECT "
            "id, "
            "user_code, "
            "name, "
            "department, "
            "fingerprint_id, "
            "status, "
            "note "
            "FROM users "
            "ORDER BY id DESC"
            ))
    {
        qDebug()
            << "[SQLite] Get users error:"
            << query.lastError().text();

        return users;
    }


    while (query.next())
    {
        QVariantMap user;


        user["id"] =
            query.value(0);

        user["user_code"] =
            query.value(1);

        user["name"] =
            query.value(2);

        user["department"] =
            query.value(3);

        user["fingerprint_id"] =
            query.value(4);

        user["status"] =
            query.value(5);

        user["note"] =
            query.value(6);


        users.append(user);
    }


    return users;
}


// =====================================================
// TÌM KIẾM NGƯờI DÙNG
// =====================================================

QVariantList Database::searchUsers(
    const QString &keyword
    )
{
    QVariantList users;

    QSqlQuery query(db);


    query.prepare(
        "SELECT "
        "id, "
        "user_code, "
        "name, "
        "department, "
        "fingerprint_id, "
        "status, "
        "note "
        "FROM users "
        "WHERE user_code LIKE ? "
        "OR name LIKE ? "
        "ORDER BY id DESC"
        );


    QString pattern =
        "%" + keyword + "%";


    query.addBindValue(pattern);
    query.addBindValue(pattern);


    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Search error:"
            << query.lastError().text();

        return users;
    }


    while (query.next())
    {
        QVariantMap user;


        user["id"] =
            query.value(0);

        user["user_code"] =
            query.value(1);

        user["name"] =
            query.value(2);

        user["department"] =
            query.value(3);

        user["fingerprint_id"] =
            query.value(4);

        user["status"] =
            query.value(5);

        user["note"] =
            query.value(6);


        users.append(user);
    }


    return users;
}


// =====================================================
// LẤY USER THEO ID DATABASE
// =====================================================

QVariantList Database::getUserById(
    int id
    )
{
    QVariantList users;

    QSqlQuery query(db);


    query.prepare(
        "SELECT "
        "id, "
        "user_code, "
        "name, "
        "department, "
        "fingerprint_id, "
        "status, "
        "note "
        "FROM users "
        "WHERE id = ?"
        );


    query.addBindValue(id);


    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Get user by id error:"
            << query.lastError().text();

        return users;
    }


    if (query.next())
    {
        QVariantMap user;


        user["id"] =
            query.value(0);

        user["user_code"] =
            query.value(1);

        user["name"] =
            query.value(2);

        user["department"] =
            query.value(3);

        user["fingerprint_id"] =
            query.value(4);

        user["status"] =
            query.value(5);

        user["note"] =
            query.value(6);


        users.append(user);
    }


    return users;
}


// =====================================================
// LẤY USER THEO ID VÂN TAY
// =====================================================

QVariantList Database::getUserByFingerprintId(
    int fingerprintId
    )
{
    QVariantList users;

    QSqlQuery query(db);


    query.prepare(
        "SELECT "
        "id, "
        "user_code, "
        "name, "
        "department, "
        "fingerprint_id, "
        "status, "
        "note "
        "FROM users "
        "WHERE fingerprint_id = ?"
        );


    query.addBindValue(fingerprintId);


    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Get user by fingerprint error:"
            << query.lastError().text();

        return users;
    }


    if (query.next())
    {
        QVariantMap user;


        user["id"] =
            query.value(0);

        user["user_code"] =
            query.value(1);

        user["name"] =
            query.value(2);

        user["department"] =
            query.value(3);

        user["fingerprint_id"] =
            query.value(4);

        user["status"] =
            query.value(5);

        user["note"] =
            query.value(6);


        users.append(user);
    }


    return users;
}


// =====================================================
// THÊM NHẬT KÝ RA VÀO
// =====================================================

bool Database::addAccessLog(
    const QString &userCode,
    const QString &name,
    int fingerprintId,
    const QString &result,
    const QString &barrier,
    const QString &direction
    )
{
    QSqlQuery query(db);


    query.prepare(
        "INSERT INTO access_logs "
        "(user_code, name, fingerprint_id, "
        "result, barrier, direction, access_time) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"
        );


    query.addBindValue(userCode);
    query.addBindValue(name);
    query.addBindValue(fingerprintId);
    query.addBindValue(result);
    query.addBindValue(barrier);
    query.addBindValue(direction);


    query.addBindValue(
        QDateTime::currentDateTime().toString(
            "dd/MM/yyyy hh:mm:ss"
            )
        );


    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Add access log error:"
            << query.lastError().text();

        return false;
    }


    qDebug()
        << "[SQLite] Access log added:"
        << userCode
        << name
        << fingerprintId
        << direction;


    return true;
}


// =====================================================
// LẤY NHẬT KÝ RA VÀO
// =====================================================

QVariantList Database::getAccessLogs(
    int limit
    )
{
    QVariantList logs;

    QSqlQuery query(db);


    query.prepare(
        "SELECT "
        "access_time, "
        "user_code, "
        "name, "
        "fingerprint_id, "
        "CASE "
        "WHEN result = 'Từ chối' THEN 'Xác thực thất bại' "
        "ELSE result END, "
        "CASE "
        "WHEN barrier = 'Đang mở' THEN 'Mở' "
        "WHEN barrier = 'Đang đóng' THEN 'Đóng' "
        "ELSE barrier END, "
        "direction "
        "FROM access_logs "
        "WHERE result IN ('Xác thực thành công', 'Xác thực thất bại', 'Từ chối') "
        "AND direction <> 'Xe ra' "
        "ORDER BY id DESC "
        "LIMIT ?"
        );


    query.addBindValue(limit);


    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Get access logs error:"
            << query.lastError().text();

        return logs;
    }


    while (query.next())
    {
        QVariantMap log;


        log["time"] =
            query.value(0);

        log["user_code"] =
            query.value(1);

        log["name"] =
            query.value(2);

        log["fingerprint_id"] =
            query.value(3);

        log["result"] =
            query.value(4);

        log["barrier"] =
            query.value(5);

        log["direction"] =
            query.value(6);


        logs.append(log);
    }


    return logs;
}

// =====================================================
// LẤY NHẬT KÝ CỦA MỘT NGƯỜI DÙNG
// =====================================================

QVariantList Database::getAccessLogsForUser(
    const QString &userCode,
    int limit
    )
{
    QVariantList logs;

    QSqlQuery query(db);

    query.prepare(
        "SELECT "
        "access_time, "
        "user_code, "
        "name, "
        "fingerprint_id, "
        "CASE "
        "WHEN result = 'Từ chối' THEN 'Xác thực thất bại' "
        "ELSE result END, "
        "CASE "
        "WHEN barrier = 'Đang mở' THEN 'Mở' "
        "WHEN barrier = 'Đang đóng' THEN 'Đóng' "
        "ELSE barrier END, "
        "direction "
        "FROM access_logs "
        "WHERE user_code = ? "
        "AND result IN "
        "('Xác thực thành công', 'Xác thực thất bại', 'Từ chối') "
        "AND direction <> 'Xe ra' "
        "ORDER BY id DESC "
        "LIMIT ?"
        );

    query.addBindValue(userCode);
    query.addBindValue(limit);

    if (!query.exec())
    {
        qDebug()
            << "[SQLite] Get user access logs error:"
            << query.lastError().text();

        return logs;
    }

    while (query.next())
    {
        QVariantMap log;

        log["time"] =
            query.value(0);

        log["user_code"] =
            query.value(1);

        log["name"] =
            query.value(2);

        log["fingerprint_id"] =
            query.value(3);

        log["result"] =
            query.value(4);

        log["barrier"] =
            query.value(5);

        log["direction"] =
            query.value(6);

        logs.append(log);
    }

    return logs;
}
