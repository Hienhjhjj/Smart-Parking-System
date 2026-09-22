#include "test_user_dialog.h"
#include <QSignalSpy>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QTimer>
#include <QApplication>

TestUserDialog::TestUserDialog(QObject *parent)
    : QObject(parent)
{
}

void TestUserDialog::initTestCase()
{
}

void TestUserDialog::cleanupTestCase()
{
}

void TestUserDialog::testUserDialogDefaults()
{
    UserDialog dlg;
    QCOMPARE(dlg.userCode(), QString());
    QCOMPARE(dlg.userName(), QString());
    QCOMPARE(dlg.department(), QString());
    QCOMPARE(dlg.fingerprintId(), -1);
    QCOMPARE(dlg.status(), QString("Hoạt động"));
    QCOMPARE(dlg.note(), QString());
}

void TestUserDialog::testUserDialogGettersAndSetters()
{
    UserDialog dlg;
    dlg.setUserCode("NV_TEST_01");
    dlg.setUserName("Nguyen Van Test");
    dlg.setDepartment("Ky Thuat");
    dlg.setFingerprintId(150);
    dlg.setStatus("Không hoạt động");
    dlg.setNote("Ghi chu kiem thu");

    QCOMPARE(dlg.userCode(), QString("NV_TEST_01"));
    QCOMPARE(dlg.userName(), QString("Nguyen Van Test"));
    QCOMPARE(dlg.department(), QString("Ky Thuat"));
    QCOMPARE(dlg.fingerprintId(), 150);
    QCOMPARE(dlg.status(), QString("Không hoạt động"));
    QCOMPARE(dlg.note(), QString("Ghi chu kiem thu"));
}

void TestUserDialog::testUserDialogWhitespaceTrimming()
{
    UserDialog dlg;
    dlg.setUserCode("   NV_TRIM_01   ");
    dlg.setUserName("   Le Thi Trim   ");
    dlg.setDepartment("   Tai Chinh   ");
    dlg.setNote("   Trimmed Note   ");

    // All text fields in UserDialog should return trimmed values according to userdialog.cpp
    QCOMPARE(dlg.userCode(), QString("NV_TRIM_01"));
    QCOMPARE(dlg.userName(), QString("Le Thi Trim"));
    QCOMPARE(dlg.department(), QString("Tai Chinh"));
    QCOMPARE(dlg.note(), QString("Trimmed Note"));
}

void TestUserDialog::testUserDialogFingerprintLimits()
{
    UserDialog dlg;
    QSpinBox *spin = dlg.findChild<QSpinBox*>("spinFingerprint");
    QVERIFY(spin != nullptr);
    QCOMPARE(spin->minimum(), -1);
    QCOMPARE(spin->maximum(), 10000);

    dlg.setFingerprintId(-1);
    QCOMPARE(dlg.fingerprintId(), -1);

    dlg.setFingerprintId(10000);
    QCOMPARE(dlg.fingerprintId(), 10000);
}

void TestUserDialog::testUserDialogStatusOptions()
{
    UserDialog dlg;
    QComboBox *cmb = dlg.findChild<QComboBox*>("cmbStatus");
    QVERIFY(cmb != nullptr);
    QCOMPARE(cmb->count(), 2);
    QCOMPARE(cmb->itemText(0), QString("Hoạt động"));
    QCOMPARE(cmb->itemText(1), QString("Không hoạt động"));

    dlg.setStatus("Không hoạt động");
    QCOMPARE(dlg.status(), QString("Không hoạt động"));

    dlg.setStatus("Hoạt động");
    QCOMPARE(dlg.status(), QString("Hoạt động"));
}

void TestUserDialog::testFingerprintEnrollDialogInit()
{
    FingerprintEnrollDialog dlg("NV001", "Tran Van A", 45);
    QCOMPARE(dlg.userCode(), QString("NV001"));
    QCOMPARE(dlg.userName(), QString("Tran Van A"));
    QCOMPARE(dlg.fingerprintId(), 45);

    QLabel *lbCode = dlg.findChild<QLabel*>("lbUserCode");
    QLabel *lbName = dlg.findChild<QLabel*>("lbUserName");
    QLabel *lbFp = dlg.findChild<QLabel*>("lbFingerprintID");
    QLabel *lbStatus = dlg.findChild<QLabel*>("lbStatus");
    QPushButton *btnStart = dlg.findChild<QPushButton*>("btnStart");

    QVERIFY(lbCode != nullptr);
    QVERIFY(lbName != nullptr);
    QVERIFY(lbFp != nullptr);
    QVERIFY(lbStatus != nullptr);
    QVERIFY(btnStart != nullptr);

    QCOMPARE(lbCode->text(), QString("NV001"));
    QCOMPARE(lbName->text(), QString("Tran Van A"));
    QCOMPARE(lbFp->text(), QString("45"));
    QCOMPARE(lbStatus->text(), QString("Sẵn sàng đăng ký"));
    QCOMPARE(btnStart->isEnabled(), true);
}

void TestUserDialog::testFingerprintEnrollDialogSignals()
{
    FingerprintEnrollDialog dlg("NV002", "Le Van B", 77);
    QSignalSpy spy(&dlg, &FingerprintEnrollDialog::startEnroll);

    QPushButton *btnStart = dlg.findChild<QPushButton*>("btnStart");
    QVERIFY(btnStart != nullptr);

    QTest::mouseClick(btnStart, Qt::LeftButton);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 77);
    QCOMPARE(btnStart->isEnabled(), false);
}

void TestUserDialog::testFingerprintEnrollDialogSuccessAndFailure()
{
    // Auto-dismiss information / warning messageboxes
    auto dismissMessageBox = []() {
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
    };

    // Test failure scenario
    {
        FingerprintEnrollDialog dlg("NV003", "Test Fail", 88);
        dlg.show();
        dismissMessageBox();
        dlg.enrollFailed("Cảm biến không nhận diện được");
        QLabel *lbStatus = dlg.findChild<QLabel*>("lbStatus");
        QCOMPARE(lbStatus->text(), QString("Đăng ký thất bại"));
        QPushButton *btnStart = dlg.findChild<QPushButton*>("btnStart");
        QCOMPARE(btnStart->isEnabled(), true);
        dlg.close();
    }

    // Test success scenario
    {
        FingerprintEnrollDialog dlg("NV004", "Test Success", 99);
        dlg.show();
        dismissMessageBox();
        dlg.enrollSuccess(99);
        QLabel *lbStatus = dlg.findChild<QLabel*>("lbStatus");
        QCOMPARE(lbStatus->text(), QString("Đăng ký thành công!"));
        QLabel *lbFp = dlg.findChild<QLabel*>("lbFingerprintID");
        QCOMPARE(lbFp->text(), QString("99"));
        dlg.close();
    }
}

#ifndef RUN_ALL_TESTS_STANDALONE
#if !defined(RUN_ALL_TESTS_COMBINED)
QTEST_MAIN(TestUserDialog)
#endif
#endif
