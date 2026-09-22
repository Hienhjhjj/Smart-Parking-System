#ifndef TEST_DATABASE_H
#define TEST_DATABASE_H

#include <QObject>
#include <QtTest>
#include "../database.h"

class TestDatabase : public QObject
{
    Q_OBJECT

public:
    explicit TestDatabase(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Database Initialization
    void testInitialize();

    // Authentication (Valid & Invalid)
    void testAuthenticateValidAdmin();
    void testAuthenticateValidStaff();
    void testAuthenticateInvalidPassword();
    void testAuthenticateInvalidUser();
    void testAuthenticateEmpty();

    // Password Change (Valid & Invalid)
    void testChangePasswordValid();
    void testChangePasswordInvalidOldPassword();
    void testChangePasswordInvalidUsername();

    // User Management (Valid & Invalid)
    void testAddUserValid();
    void testAddUserWithoutFingerprint();
    void testAddUserDuplicateUserCode();
    void testAddUserDuplicateFingerprint();
    void testUpdateUserValid();
    void testUpdateUserDuplicateCode();
    void testDeleteUserValid();
    void testUserCount();
    void testGetUsers();
    void testSearchUsersValid();
    void testSearchUsersNotFound();
    void testGetUserByIdValidAndInvalid();
    void testGetUserByFingerprintIdValidAndInvalid();

    // Access Logs (Valid, Invalid/Filtering & Mapping)
    void testAddAccessLog();
    void testGetAccessLogsFilteringAndMapping();

private:
    Database *m_db;
    void clearTestTables();
};

#endif // TEST_DATABASE_H
