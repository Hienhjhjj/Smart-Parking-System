#ifndef TEST_LOGIN_H
#define TEST_LOGIN_H

#include <QObject>
#include <QtTest>
#include "../login.h"

class TestLogin : public QObject
{
    Q_OBJECT

public:
    explicit TestLogin(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // UI Initial State & Toggles
    void testInitialUIState();
    void testShowPasswordToggle();

    // Validation (Invalid Inputs)
    void testEmptyUsernameAndPassword();
    void testEmptyUsernameWithPassword();
    void testEmptyPasswordWithUsername();

    // Authentication (Invalid & Valid)
    void testInvalidCredentials();
    void testValidAdminLogin();
    void testValidStaffLogin();

    // Key Return Trigger
    void testReturnPressedTriggersLogin();

private:
    Login *m_login;
    void dismissActiveMessageBox();
};

#endif // TEST_LOGIN_H
