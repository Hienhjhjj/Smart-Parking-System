#include "test_database.h"
#include <QSqlQuery>
#include <QSqlError>

TestDatabase::TestDatabase(QObject *parent)
    : QObject(parent)
    , m_db(nullptr)
{
}

void TestDatabase::initTestCase()
{
    m_db = new Database(this);
    QVERIFY2(m_db->initialize(), "Database initialization failed in initTestCase");
}

void TestDatabase::cleanupTestCase()
{
    delete m_db;
    m_db = nullptr;
}

void TestDatabase::clearTestTables()
{
    QSqlDatabase db = QSqlDatabase::database("ParkingSystemDB");
    if (db.isOpen())
    {
        QSqlQuery query(db);
        query.exec("DELETE FROM users");
        query.exec("DELETE FROM access_logs");
    }
}

void TestDatabase::init()
{
    clearTestTables();
}

void TestDatabase::cleanup()
{
    clearTestTables();
}

void TestDatabase::testInitialize()
{
    // Calling initialize() again on an already configured DB should succeed
    bool result = m_db->initialize();
    QVERIFY(result);

    // Verify default accounts exist
    QString role;
    QVERIFY(m_db->authenticateAccount("admin", "123456", role));
    QCOMPARE(role, QString("admin"));

    QVERIFY(m_db->authenticateAccount("user", "123456", role));
    QCOMPARE(role, QString("staff"));
}

void TestDatabase::testAuthenticateValidAdmin()
{
    QString role;
    bool success = m_db->authenticateAccount("admin", "123456", role);
    QVERIFY(success);
    QCOMPARE(role, QString("admin"));
}

void TestDatabase::testAuthenticateValidStaff()
{
    QString role;
    bool success = m_db->authenticateAccount("user", "123456", role);
    QVERIFY(success);
    QCOMPARE(role, QString("staff"));
}

void TestDatabase::testAuthenticateInvalidPassword()
{
    QString role;
    bool success = m_db->authenticateAccount("admin", "wrong_password_999", role);
    QVERIFY(!success);
    QVERIFY(role.isEmpty());
}

void TestDatabase::testAuthenticateInvalidUser()
{
    QString role;
    bool success = m_db->authenticateAccount("non_existing_user_xyz", "123456", role);
    QVERIFY(!success);
    QVERIFY(role.isEmpty());
}

void TestDatabase::testAuthenticateEmpty()
{
    QString role;
    bool success = m_db->authenticateAccount("", "", role);
    QVERIFY(!success);
    QVERIFY(role.isEmpty());
}

void TestDatabase::testChangePasswordValid()
{
    // Change user password to new password
    bool changed = m_db->changePassword("user", "123456", "new_secure_pwd_123");
    QVERIFY(changed);

    // Verify authentication with new password succeeds
    QString role;
    bool authNew = m_db->authenticateAccount("user", "new_secure_pwd_123", role);
    QVERIFY(authNew);
    QCOMPARE(role, QString("staff"));

    // Verify authentication with old password now fails
    bool authOld = m_db->authenticateAccount("user", "123456", role);
    QVERIFY(!authOld);

    // Restore original password
    bool restored = m_db->changePassword("user", "new_secure_pwd_123", "123456");
    QVERIFY(restored);
}

void TestDatabase::testChangePasswordInvalidOldPassword()
{
    // Try to change password with incorrect old password
    bool changed = m_db->changePassword("admin", "incorrect_old_pwd", "new_pwd");
    QVERIFY(!changed);

    // Original password should remain intact
    QString role;
    bool auth = m_db->authenticateAccount("admin", "123456", role);
    QVERIFY(auth);
}

void TestDatabase::testChangePasswordInvalidUsername()
{
    bool changed = m_db->changePassword("unknown_user_123", "123456", "new_pwd");
    QVERIFY(!changed);
}

void TestDatabase::testAddUserValid()
{
    bool added = m_db->addUser("NV001", "Nguyen Van A", "Phong IT", 101, "Hoạt động", "Nhan vien chinh thuc");
    QVERIFY(added);
    QCOMPARE(m_db->userCount(), 1);

    QVariantList users = m_db->getUserByFingerprintId(101);
    QCOMPARE(users.size(), 1);
    QVariantMap user = users.first().toMap();
    QCOMPARE(user["user_code"].toString(), QString("NV001"));
    QCOMPARE(user["name"].toString(), QString("Nguyen Van A"));
    QCOMPARE(user["department"].toString(), QString("Phong IT"));
    QCOMPARE(user["fingerprint_id"].toInt(), 101);
    QCOMPARE(user["status"].toString(), QString("Hoạt động"));
    QCOMPARE(user["note"].toString(), QString("Nhan vien chinh thuc"));
}

void TestDatabase::testAddUserWithoutFingerprint()
{
    // FingerprintId < 0 means no fingerprint enrolled yet
    bool added = m_db->addUser("NV002", "Tran Thi B", "Phong Ke Toan", -1, "Hoạt động", "Chua co van tay");
    QVERIFY(added);
    QCOMPARE(m_db->userCount(), 1);

    QVariantList users = m_db->searchUsers("NV002");
    QCOMPARE(users.size(), 1);
    QVariantMap user = users.first().toMap();
    QCOMPARE(user["user_code"].toString(), QString("NV002"));
    QVERIFY(user["fingerprint_id"].isNull() || user["fingerprint_id"].toString().isEmpty());
}

void TestDatabase::testAddUserDuplicateUserCode()
{
    bool added1 = m_db->addUser("NV_DUP", "User One", "IT", 201, "Hoạt động", "");
    QVERIFY(added1);

    // Duplicate user_code should fail due to UNIQUE constraint
    bool added2 = m_db->addUser("NV_DUP", "User Two", "HR", 202, "Hoạt động", "");
    QVERIFY(!added2);
}

void TestDatabase::testAddUserDuplicateFingerprint()
{
    bool added1 = m_db->addUser("NV_FP1", "User One", "IT", 301, "Hoạt động", "");
    QVERIFY(added1);

    // Duplicate fingerprint_id should fail due to UNIQUE constraint
    bool added2 = m_db->addUser("NV_FP2", "User Two", "HR", 301, "Hoạt động", "");
    QVERIFY(!added2);
}

void TestDatabase::testUpdateUserValid()
{
    m_db->addUser("NV003", "Le Van C", "Marketing", 401, "Hoạt động", "Ghi chu cu");
    QVariantList users = m_db->searchUsers("NV003");
    QCOMPARE(users.size(), 1);
    int id = users.first().toMap()["id"].toInt();

    bool updated = m_db->updateUser(id, "NV003_UPDATED", "Le Van C (Truong Phong)", "Ban Giam Doc", 402, "Không hoạt động", "Da chuyen phong ban");
    QVERIFY(updated);

    QVariantList updatedList = m_db->getUserById(id);
    QCOMPARE(updatedList.size(), 1);
    QVariantMap user = updatedList.first().toMap();
    QCOMPARE(user["user_code"].toString(), QString("NV003_UPDATED"));
    QCOMPARE(user["name"].toString(), QString("Le Van C (Truong Phong)"));
    QCOMPARE(user["department"].toString(), QString("Ban Giam Doc"));
    QCOMPARE(user["fingerprint_id"].toInt(), 402);
    QCOMPARE(user["status"].toString(), QString("Không hoạt động"));
    QCOMPARE(user["note"].toString(), QString("Da chuyen phong ban"));
}

void TestDatabase::testUpdateUserDuplicateCode()
{
    m_db->addUser("NV_A", "User A", "IT", 501, "Hoạt động", "");
    m_db->addUser("NV_B", "User B", "HR", 502, "Hoạt động", "");

    QVariantList usersB = m_db->searchUsers("NV_B");
    int idB = usersB.first().toMap()["id"].toInt();

    // Trying to update User B's code to "NV_A" which already exists -> should fail
    bool updated = m_db->updateUser(idB, "NV_A", "User B", "HR", 502, "Hoạt động", "");
    QVERIFY(!updated);
}

void TestDatabase::testDeleteUserValid()
{
    m_db->addUser("NV_DEL", "User To Delete", "Test", 601, "Hoạt động", "");
    QCOMPARE(m_db->userCount(), 1);

    QVariantList users = m_db->searchUsers("NV_DEL");
    int id = users.first().toMap()["id"].toInt();

    bool deleted = m_db->deleteUser(id);
    QVERIFY(deleted);
    QCOMPARE(m_db->userCount(), 0);

    QVariantList checkList = m_db->getUserById(id);
    QVERIFY(checkList.isEmpty());
}

void TestDatabase::testUserCount()
{
    QCOMPARE(m_db->userCount(), 0);

    m_db->addUser("U1", "User 1", "D1", 701, "Hoạt động", "");
    m_db->addUser("U2", "User 2", "D2", 702, "Hoạt động", "");
    m_db->addUser("U3", "User 3", "D3", 703, "Hoạt động", "");

    QCOMPARE(m_db->userCount(), 3);
}

void TestDatabase::testGetUsers()
{
    m_db->addUser("U_ALPHA", "Alpha Name", "Dept A", 801, "Hoạt động", "Note A");
    m_db->addUser("U_BETA", "Beta Name", "Dept B", 802, "Hoạt động", "Note B");

    QVariantList list = m_db->getUsers();
    QCOMPARE(list.size(), 2);
    // Ordered by id DESC
    QCOMPARE(list.at(0).toMap()["user_code"].toString(), QString("U_BETA"));
    QCOMPARE(list.at(1).toMap()["user_code"].toString(), QString("U_ALPHA"));
}

void TestDatabase::testSearchUsersValid()
{
    m_db->addUser("SEARCH_01", "Hoang Van Nam", "Kho Van", 901, "Hoạt động", "");
    m_db->addUser("SEARCH_02", "Pham Thi Lan", "Ke Toan", 902, "Hoạt động", "");

    // Search by name
    QVariantList resByName = m_db->searchUsers("Lan");
    QCOMPARE(resByName.size(), 1);
    QCOMPARE(resByName.first().toMap()["name"].toString(), QString("Pham Thi Lan"));

    // Search by code
    QVariantList resByCode = m_db->searchUsers("SEARCH_01");
    QCOMPARE(resByCode.size(), 1);
    QCOMPARE(resByCode.first().toMap()["user_code"].toString(), QString("SEARCH_01"));
}

void TestDatabase::testSearchUsersNotFound()
{
    m_db->addUser("SEARCH_01", "Hoang Van Nam", "Kho Van", 901, "Hoạt động", "");
    QVariantList res = m_db->searchUsers("NOT_EXISTING_KEYWORD");
    QVERIFY(res.isEmpty());
}

void TestDatabase::testGetUserByIdValidAndInvalid()
{
    m_db->addUser("GET_ID", "Nguyen Test", "IT", 950, "Hoạt động", "");
    QVariantList list = m_db->searchUsers("GET_ID");
    int id = list.first().toMap()["id"].toInt();

    // Valid ID
    QVariantList validResult = m_db->getUserById(id);
    QCOMPARE(validResult.size(), 1);
    QCOMPARE(validResult.first().toMap()["name"].toString(), QString("Nguyen Test"));

    // Invalid ID
    QVariantList invalidResult = m_db->getUserById(9999999);
    QVERIFY(invalidResult.isEmpty());
}

void TestDatabase::testGetUserByFingerprintIdValidAndInvalid()
{
    m_db->addUser("GET_FP", "Tran Test", "IT", 980, "Hoạt động", "");

    // Valid fingerprint
    QVariantList validResult = m_db->getUserByFingerprintId(980);
    QCOMPARE(validResult.size(), 1);
    QCOMPARE(validResult.first().toMap()["user_code"].toString(), QString("GET_FP"));

    // Invalid fingerprint
    QVariantList invalidResult = m_db->getUserByFingerprintId(123456);
    QVERIFY(invalidResult.isEmpty());
}

void TestDatabase::testAddAccessLog()
{
    bool added = m_db->addAccessLog("NV001", "Nguyen Van A", 101, "Xác thực thành công", "Đang mở", "Xe vào");
    QVERIFY(added);
}

void TestDatabase::testGetAccessLogsFilteringAndMapping()
{
    // Insert various types of logs
    // 1. Success, Xe vào, barrier Đang mở -> should be included, mapped to Mở
    m_db->addAccessLog("NV001", "Nguyen Van A", 101, "Xác thực thành công", "Đang mở", "Xe vào");

    // 2. Reject "Từ chối", Xe vào, barrier Đang đóng -> should be mapped to "Xác thực thất bại" and "Đóng"
    m_db->addAccessLog("NV002", "Tran Thi B", 102, "Từ chối", "Đang đóng", "Xe vào");

    // 3. Direction "Xe ra" -> query filters out direction <> 'Xe ra'
    m_db->addAccessLog("NV003", "Le Van C", 103, "Xác thực thành công", "Đang mở", "Xe ra");

    // 4. Result not in ('Xác thực thành công', 'Xác thực thất bại', 'Từ chối') e.g. "Chờ xác thực"
    m_db->addAccessLog("NV004", "Pham Van D", 104, "Chờ xác thực", "Đang đóng", "Xe vào");

    QVariantList logs = m_db->getAccessLogs(100);
    // Should contain exactly 2 logs (NV002 and NV001, ordered by id DESC)
    QCOMPARE(logs.size(), 2);

    QVariantMap log1 = logs.at(0).toMap(); // Latest is NV002
    QCOMPARE(log1["user_code"].toString(), QString("NV002"));
    QCOMPARE(log1["result"].toString(), QString("Xác thực thất bại")); // Mapped from 'Từ chối'
    QCOMPARE(log1["barrier"].toString(), QString("Đóng")); // Mapped from 'Đang đóng'
    QCOMPARE(log1["direction"].toString(), QString("Xe vào"));

    QVariantMap log2 = logs.at(1).toMap(); // Earlier is NV001
    QCOMPARE(log2["user_code"].toString(), QString("NV001"));
    QCOMPARE(log2["result"].toString(), QString("Xác thực thành công"));
    QCOMPARE(log2["barrier"].toString(), QString("Mở")); // Mapped from 'Đang mở'
    QCOMPARE(log2["direction"].toString(), QString("Xe vào"));
}

#ifndef RUN_ALL_TESTS_STANDALONE
#if !defined(RUN_ALL_TESTS_COMBINED)
QTEST_MAIN(TestDatabase)
#endif
#endif
