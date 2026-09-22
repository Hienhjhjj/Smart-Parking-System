#ifndef FINGERPRINTENROLLDIALOG_H
#define FINGERPRINTENROLLDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class FingerprintEnrollDialog;
}
QT_END_NAMESPACE

class FingerprintEnrollDialog : public QDialog
{
    Q_OBJECT

public:

    explicit FingerprintEnrollDialog(
        const QString &userCode,
        const QString &userName,
        int fingerprintId,
        QWidget *parent = nullptr
    );

    ~FingerprintEnrollDialog();


    QString userCode() const;
    QString userName() const;
    int fingerprintId() const;


public slots:

    void setStatus(
        const QString &status
    );

    void enrollSuccess(
        int id
    );

    void enrollFailed(
        const QString &reason
    );


signals:

    void startEnroll(
        int id
    );


private slots:

    void onStartClicked();


private:

    Ui::FingerprintEnrollDialog *ui;

    QString m_userCode;
    QString m_userName;

    int m_fingerprintId;

    bool enrolling;
};

#endif
