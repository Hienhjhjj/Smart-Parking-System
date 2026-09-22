#define RUN_ALL_TESTS_COMBINED
#include <QApplication>
#include <QtTest>
#include <iostream>

#include "test_database.h"
#include "test_login.h"
#include "test_user_dialog.h"
#include "test_mainwindow.h"

int main(int argc, char *argv[])
{
    // Enable offscreen rendering by default if not set
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication app(argc, argv);

    int status = 0;

    std::cout << "\n======================================================\n";
    std::cout << "           RUNNING ALL QT TEST SUITES                \n";
    std::cout << "======================================================\n\n";

    // 1. Test Database
    {
        std::cout << "\n>>> [1/4] Running TestDatabase...\n";
        TestDatabase testDb;
        status |= QTest::qExec(&testDb, argc, argv);
    }

    // 2. Test Login
    {
        std::cout << "\n>>> [2/4] Running TestLogin...\n";
        TestLogin testLogin;
        status |= QTest::qExec(&testLogin, argc, argv);
    }

    // 3. Test User Dialog
    {
        std::cout << "\n>>> [3/4] Running TestUserDialog...\n";
        TestUserDialog testUserDialog;
        status |= QTest::qExec(&testUserDialog, argc, argv);
    }

    // 4. Test MainWindow
    {
        std::cout << "\n>>> [4/4] Running TestMainWindow...\n";
        TestMainWindow testMainWindow;
        status |= QTest::qExec(&testMainWindow, argc, argv);
    }

    std::cout << "\n======================================================\n";
    if (status == 0)
    {
        std::cout << "  ALL TEST SUITES COMPLETED SUCCESSFULLY (PASS)  \n";
    }
    else
    {
        std::cout << "  SOME TEST SUITES REPORTED FAILURES (FAIL)       \n";
    }
    std::cout << "======================================================\n\n";

    return status;
}
