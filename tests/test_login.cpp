#include "test_login.h"
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QTimer>
#include <QApplication>

TestLogin::TestLogin(QObject *parent)
    : QObject(parent)
    , m_login(nullptr)
{
}

void TestLogin::initTestCase()
{
}

void TestLogin::cleanupTestCase()
{
}

void TestLogin::init()
{
    m_login = new Login();
    m_login->show();
}

void TestLogin::cleanup()
{
    if (m_login)
    {
        m_login->close();
        delete m_login;
        m_login = nullptr;
    }
}

void TestLogin::dismissActiveMessageBox()
{
    // Schedules closing of any QMessageBox that pops up
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

void TestLogin::testInitialUIState()
{
    QLineEdit *txtUser = m_login->findChild<QLineEdit*>("txtUsername");
    QLineEdit *txtPass = m_login->findChild<QLineEdit*>("txtPassword");
    QPushButton *btnLog = m_login->findChild<QPushButton*>("btnLogin");
    QCheckBox *chkShow = m_login->findChild<QCheckBox*>("chkShowPassword");

    QVERIFY(txtUser != nullptr);
    QVERIFY(txtPass != nullptr);
    QVERIFY(btnLog != nullptr);
    QVERIFY(chkShow != nullptr);

    QCOMPARE(txtUser->text(), QString());
    QCOMPARE(txtPass->text(), QString());
    QCOMPARE(txtPass->echoMode(), QLineEdit::Password);
    QCOMPARE(chkShow->isChecked(), false);
}

void TestLogin::testShowPasswordToggle()
{
    QLineEdit *txtPass = m_login->findChild<QLineEdit*>("txtPassword");
    QCheckBox *chkShow = m_login->findChild<QCheckBox*>("chkShowPassword");
    QVERIFY(txtPass != nullptr && chkShow != nullptr);

    // Toggle on -> Normal mode
    chkShow->setChecked(true);
    QCOMPARE(txtPass->echoMode(), QLineEdit::Normal);

    // Toggle off -> Password mode
    chkShow->setChecked(false);
    QCOMPARE(txtPass->echoMode(), QLineEdit::Password);
}

void TestLogin::testEmptyUsernameAndPassword()
{
    QLineEdit *txtUser = m_login->findChild<QLineEdit*>("txtUsername");
    QLineEdit *txtPass = m_login->findChild<QLineEdit*>("txtPassword");
    QPushButton *btnLog = m_login->findChild<QPushButton*>("btnLogin");

    txtUser->clear();
    txtPass->clear();

    dismissActiveMessageBox();
    QTest::mouseClick(btnLog, Qt::LeftButton);

    // Login window must still be open
    QVERIFY(m_login->isVisible());
}

void TestLogin::testEmptyUsernameWithPassword()
{
    QLineEdit *txtUser = m_login->findChild<QLineEdit*>("txtUsername");
    QLineEdit *txtPass = m_login->findChild<QLineEdit*>("txtPassword");
    QPushButton *btnLog = m_login->findChild<QPushButton*>("btnLogin");

    txtUser->setText("   "); // whitespace only -> trimmed to empty
    txtPass->setText("123456");

    dismissActiveMessageBox();
    QTest::mouseClick(btnLog, Qt::LeftButton);

    QVERIFY(m_login->isVisible());
}

void TestLogin::testEmptyPasswordWithUsername()
{
    QLineEdit *txtUser = m_login->findChild<QLineEdit*>("txtUsername");
    QLineEdit *txtPass = m_login->findChild<QLineEdit*>("txtPassword");
    QPushButton *btnLog = m_login->findChild<QPushButton*>("btnLogin");

    txtUser->setText("admin");
    txtPass->clear();

    dismissActiveMessageBox();
    QTest::mouseClick(btnLog, Qt::LeftButton);

    QVERIFY(m_login->isVisible());
}

void TestLogin::testInvalidCredentials()
{
    QLineEdit *txtUser = m_login->findChild<QLineEdit*>("txtUsername");
    QLineEdit *txtPass = m_login->findChild<QLineEdit*>("txtPassword");
    QPushButton *btnLog = m_login->findChild<QPushButton*>("btnLogin");

    txtUser->setText("admin");
    txtPass->setText("wrong_password");

    dismissActiveMessageBox();
    QTest::mouseClick(btnLog, Qt::LeftButton);

    // Password should be cleared on failure
    QCOMPARE(txtPass->text(), QString());
    QVERIFY(m_login->isVisible());
}

void TestLogin::testValidAdminLogin()
{
    QLineEdit *txtUser = m_login->findChild<QLineEdit*>("txtUsername");
    QLineEdit *txtPass = m_login->findChild<QLineEdit*>("txtPassword");
    QPushButton *btnLog = m_login->findChild<QPushButton*>("btnLogin");

    txtUser->setText("admin");
    txtPass->setText("123456");

    dismissActiveMessageBox();
    QTest::mouseClick(btnLog, Qt::LeftButton);

    // Login window should be closed on success
    QVERIFY(!m_login->isVisible());
}

void TestLogin::testValidStaffLogin()
{
    QLineEdit *txtUser = m_login->findChild<QLineEdit*>("txtUsername");
    QLineEdit *txtPass = m_login->findChild<QLineEdit*>("txtPassword");
    QPushButton *btnLog = m_login->findChild<QPushButton*>("btnLogin");

    txtUser->setText("user");
    txtPass->setText("123456");

    dismissActiveMessageBox();
    QTest::mouseClick(btnLog, Qt::LeftButton);

    // Login window should be closed on success
    QVERIFY(!m_login->isVisible());
}

void TestLogin::testReturnPressedTriggersLogin()
{
    QLineEdit *txtUser = m_login->findChild<QLineEdit*>("txtUsername");
    QLineEdit *txtPass = m_login->findChild<QLineEdit*>("txtPassword");

    txtUser->setText("admin");
    txtPass->setText("123456");

    dismissActiveMessageBox();
    QTest::keyClick(txtPass, Qt::Key_Return);

    // Should succeed and close
    QVERIFY(!m_login->isVisible());
}

#ifndef RUN_ALL_TESTS_STANDALONE
#if !defined(RUN_ALL_TESTS_COMBINED)
QTEST_MAIN(TestLogin)
#endif
#endif
