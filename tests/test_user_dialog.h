#ifndef TEST_USER_DIALOG_H
#define TEST_USER_DIALOG_H

#include <QObject>
#include <QtTest>
#include "../userdialog.h"
#include "../fingerprintenrolldialog.h"

class TestUserDialog : public QObject
{
    Q_OBJECT

public:
    explicit TestUserDialog(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void cleanupTestCase();

    // UserDialog Tests
    void testUserDialogDefaults();
    void testUserDialogGettersAndSetters();
    void testUserDialogWhitespaceTrimming();
    void testUserDialogFingerprintLimits();
    void testUserDialogStatusOptions();

    // FingerprintEnrollDialog Tests
    void testFingerprintEnrollDialogInit();
    void testFingerprintEnrollDialogSignals();
    void testFingerprintEnrollDialogSuccessAndFailure();
};

#endif // TEST_USER_DIALOG_H
