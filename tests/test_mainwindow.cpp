#include "test_mainwindow.h"
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QDateEdit>
#include <QAction>
#include <QMenuBar>
#include <QMessageBox>
#include <QTimer>
#include <QApplication>
#include <QSqlQuery>

TestMainWindow::TestMainWindow(QObject *parent)
    : QObject(parent)
    , m_window(nullptr)
    , m_db(nullptr)
{
}

void TestMainWindow::initTestCase()
{
    m_db = new Database(this);
    m_db->initialize();
}

void TestMainWindow::cleanupTestCase()
{
    delete m_db;
    m_db = nullptr;
}

void TestMainWindow::dismissActiveMessageBox()
{
    QTimer::singleShot(100, []() {
        QWidgetList topLevels = QApplication::topLevelWidgets();
        for (QWidget *w : topLevels)
        {
            if (QMessageBox *box = qobject_cast<QMessageBox*>(w))
            {
                box->accept();
            }
        }
    });
}

void TestMainWindow::init()
{
    // Clean tables before creating MainWindow
    QSqlDatabase db = QSqlDatabase::database("ParkingSystemDB");
    if (db.isOpen())
    {
        QSqlQuery query(db);
        query.exec("DELETE FROM users");
        query.exec("DELETE FROM access_logs");
    }
}

void TestMainWindow::cleanup()
{
    if (m_window)
    {
        m_window->close();
        delete m_window;
        m_window = nullptr;
    }
}

void TestMainWindow::testInitialWidgetsAndLabels()
{
    m_window = new MainWindow("admin", "admin");
    m_window->show();

    QLabel *lbESP = m_window->findChild<QLabel*>("lbESP");
    QLabel *lbSensor = m_window->findChild<QLabel*>("lbSensor");
    QLabel *lbFinger = m_window->findChild<QLabel*>("lbFinger");
    QLabel *lbServo = m_window->findChild<QLabel*>("lbServo");
    QLabel *lbOnline = m_window->findChild<QLabel*>("lbOnline");
    QLabel *lbToday = m_window->findChild<QLabel*>("lbToday");
    QLabel *lbFingerID = m_window->findChild<QLabel*>("lbFingerID");
    QLabel *lbResult = m_window->findChild<QLabel*>("lbResult");
    QLabel *lbBarrier = m_window->findChild<QLabel*>("lbBarrier");
    QLabel *lbName = m_window->findChild<QLabel*>("lbName");
    QLabel *lbID = m_window->findChild<QLabel*>("lbID");

    QVERIFY(lbESP != nullptr);
    QVERIFY(lbSensor != nullptr);
    QVERIFY(lbFinger != nullptr);
    QVERIFY(lbServo != nullptr);
    QVERIFY(lbOnline != nullptr);
    QVERIFY(lbToday != nullptr);
    QVERIFY(lbFingerID != nullptr);
    QVERIFY(lbResult != nullptr);
    QVERIFY(lbBarrier != nullptr);
    QVERIFY(lbName != nullptr);
    QVERIFY(lbID != nullptr);

    QCOMPARE(lbESP->text(), QString("Offline"));
    QCOMPARE(lbSensor->text(), QString("Sẵn sàng"));
    QCOMPARE(lbFinger->text(), QString("Sẵn sàng"));
    QCOMPARE(lbServo->text(), QString("Đang đóng"));
    QCOMPARE(lbOnline->text(), QString("0 / 4"));
    QCOMPARE(lbToday->text(), QString("0"));
    QCOMPARE(lbFingerID->text(), QString("---"));
    QCOMPARE(lbResult->text(), QString("Chưa xác thực"));
    QCOMPARE(lbBarrier->text(), QString("Đang đóng"));
    QCOMPARE(lbName->text(), QString("---"));
    QCOMPARE(lbID->text(), QString("---"));
}

void TestMainWindow::testTableHeadersAndColumns()
{
    m_window = new MainWindow("admin", "admin");
    m_window->show();

    QTableWidget *tblRecent = m_window->findChild<QTableWidget*>("tblRecentLog");
    QTableWidget *tblHistory = m_window->findChild<QTableWidget*>("tblHistory");
    QTableWidget *tblUsers = m_window->findChild<QTableWidget*>("tblUsers");

    QVERIFY(tblRecent != nullptr);
    QVERIFY(tblHistory != nullptr);
    QVERIFY(tblUsers != nullptr);

    QCOMPARE(tblRecent->columnCount(), 5);
    QCOMPARE(tblHistory->columnCount(), 7);
    QCOMPARE(tblUsers->columnCount(), 6);
}

void TestMainWindow::testAdminPermissions()
{
    m_window = new MainWindow("admin", "admin");
    m_window->show();

    QPushButton *btnAdd = m_window->findChild<QPushButton*>("btnAddUser");
    QPushButton *btnEdit = m_window->findChild<QPushButton*>("btnEditUser");
    QPushButton *btnDel = m_window->findChild<QPushButton*>("btnDeleteUser");
    QPushButton *btnEnroll = m_window->findChild<QPushButton*>("btnEnroll");
    QTabWidget *tabWidget = m_window->findChild<QTabWidget*>("tabWidget");

    QVERIFY(btnAdd != nullptr);
    QVERIFY(btnEdit != nullptr);
    QVERIFY(btnDel != nullptr);
    QVERIFY(btnEnroll != nullptr);
    QVERIFY(tabWidget != nullptr);

    QCOMPARE(btnAdd->isEnabled(), true);
    QCOMPARE(btnEdit->isEnabled(), true);
    QCOMPARE(btnDel->isEnabled(), true);
    QCOMPARE(btnEnroll->isEnabled(), true);

    if (tabWidget->count() > 3)
    {
        QCOMPARE(tabWidget->isTabVisible(3), true);
    }
}

void TestMainWindow::testStaffPermissions()
{
    m_window = new MainWindow("user", "staff");
    m_window->show();

    QPushButton *btnAdd = m_window->findChild<QPushButton*>("btnAddUser");
    QPushButton *btnEdit = m_window->findChild<QPushButton*>("btnEditUser");
    QPushButton *btnDel = m_window->findChild<QPushButton*>("btnDeleteUser");
    QPushButton *btnEnroll = m_window->findChild<QPushButton*>("btnEnroll");
    QTabWidget *tabWidget = m_window->findChild<QTabWidget*>("tabWidget");

    QVERIFY(btnAdd != nullptr);
    QVERIFY(btnEdit != nullptr);
    QVERIFY(btnDel != nullptr);
    QVERIFY(btnEnroll != nullptr);
    QVERIFY(tabWidget != nullptr);

    // Staff must not be able to add/edit/delete users or enroll fingerprints
    QCOMPARE(btnAdd->isEnabled(), false);
    QCOMPARE(btnEdit->isEnabled(), false);
    QCOMPARE(btnDel->isEnabled(), false);
    QCOMPARE(btnEnroll->isEnabled(), false);

    // Settings tab must be hidden for staff
    if (tabWidget->count() > 3)
    {
        QCOMPARE(tabWidget->isTabVisible(3), false);
    }
}

void TestMainWindow::testSystemMenuActions()
{
    m_window = new MainWindow("admin", "admin");
    m_window->show();

    QAction *actionChangePwd = m_window->findChild<QAction*>("actionChangePassword");
    QAction *actionLogout = m_window->findChild<QAction*>("actionLogout");
    QAction *actionExit = m_window->findChild<QAction*>("actionExit");

    QVERIFY(actionChangePwd != nullptr);
    QVERIFY(actionLogout != nullptr);
    QVERIFY(actionExit != nullptr);

    QCOMPARE(actionChangePwd->text(), QString("Đổi mật khẩu"));
    QCOMPARE(actionLogout->text(), QString("Đăng xuất"));
    QCOMPARE(actionExit->text(), QString("Thoát"));
}

void TestMainWindow::testHistoryFilterInvalidDateRange()
{
    m_window = new MainWindow("admin", "admin");
    m_window->show();

    QDateEdit *dateFrom = m_window->findChild<QDateEdit*>("dateFrom");
    QDateEdit *dateTo = m_window->findChild<QDateEdit*>("dateTo");
    QPushButton *btnFilter = m_window->findChild<QPushButton*>("btnFilterHistory");

    QVERIFY(dateFrom != nullptr && dateTo != nullptr && btnFilter != nullptr);

    // Set invalid range: fromDate > toDate
    dateFrom->setDate(QDate(2026, 9, 25));
    dateTo->setDate(QDate(2026, 9, 20));

    dismissActiveMessageBox();
    QTest::mouseClick(btnFilter, Qt::LeftButton);

    // Window must stay stable and open
    QVERIFY(m_window->isVisible());
}

void TestMainWindow::testHistoryFilterValidRange()
{
    // Insert logs into database first
    m_db->addAccessLog("NV001", "User Filter Test", 101, "Xác thực thành công", "Đang mở", "Xe vào");

    m_window = new MainWindow("admin", "admin");
    m_window->show();

    QDateEdit *dateFrom = m_window->findChild<QDateEdit*>("dateFrom");
    QDateEdit *dateTo = m_window->findChild<QDateEdit*>("dateTo");
    QPushButton *btnFilter = m_window->findChild<QPushButton*>("btnFilterHistory");
    QTableWidget *tblHistory = m_window->findChild<QTableWidget*>("tblHistory");

    // Set valid date range covering today
    dateFrom->setDate(QDate::currentDate().addDays(-1));
    dateTo->setDate(QDate::currentDate().addDays(1));

    QTest::mouseClick(btnFilter, Qt::LeftButton);

    QCOMPARE(tblHistory->rowCount(), 1);
}

void TestMainWindow::testExportEmptyHistoryValidation()
{
    m_window = new MainWindow("admin", "admin");
    m_window->show();

    QPushButton *btnExport = m_window->findChild<QPushButton*>("btnExport");
    QVERIFY(btnExport != nullptr);

    dismissActiveMessageBox();
    QTest::mouseClick(btnExport, Qt::LeftButton);

    QVERIFY(m_window->isVisible());
}

void TestMainWindow::testLoadUsersPopulatesTable()
{
    m_db->addUser("NV_LOAD_1", "Nguyen A", "IT", 101, "Hoạt động", "Ghi chu 1");
    m_db->addUser("NV_LOAD_2", "Tran B", "HR", 102, "Không hoạt động", "Ghi chu 2");

    m_window = new MainWindow("admin", "admin");
    m_window->show();

    QTableWidget *tblUsers = m_window->findChild<QTableWidget*>("tblUsers");
    QVERIFY(tblUsers != nullptr);
    QCOMPARE(tblUsers->rowCount(), 2);
}

#ifndef RUN_ALL_TESTS_STANDALONE
#if !defined(RUN_ALL_TESTS_COMBINED)
QTEST_MAIN(TestMainWindow)
#endif
#endif
