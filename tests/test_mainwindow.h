#ifndef TEST_MAINWINDOW_H
#define TEST_MAINWINDOW_H

#include <QObject>
#include <QtTest>
#include "../mainwindow.h"
#include "../database.h"

class TestMainWindow : public QObject
{
    Q_OBJECT

public:
    explicit TestMainWindow(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Initial State
    void testInitialWidgetsAndLabels();
    void testTableHeadersAndColumns();

    // Permissions (Admin vs Staff)
    void testAdminPermissions();
    void testStaffPermissions();

    // System Menu
    void testSystemMenuActions();

    // History Date Filter Validation (Valid & Invalid Range)
    void testHistoryFilterInvalidDateRange();
    void testHistoryFilterValidRange();

    // Export Empty Validation
    void testExportEmptyHistoryValidation();

    // User Table Loading
    void testLoadUsersPopulatesTable();

private:
    MainWindow *m_window;
    Database *m_db;
    void dismissActiveMessageBox();
};

#endif // TEST_MAINWINDOW_H
