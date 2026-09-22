#include "fingerprintenrolldialog.h"
#include "./ui_fingerprintenrolldialog.h"

#include <QMessageBox>


FingerprintEnrollDialog::FingerprintEnrollDialog(
    const QString &userCode,
    const QString &userName,
    int fingerprintId,
    QWidget *parent
)
    : QDialog(parent)
    , ui(new Ui::FingerprintEnrollDialog)
    , m_userCode(userCode)
    , m_userName(userName)
    , m_fingerprintId(fingerprintId)
    , enrolling(false)
{
    ui->setupUi(this);


    ui->lbUserCode->setText(
        m_userCode
    );

    ui->lbUserName->setText(
        m_userName
    );

    ui->lbFingerprintID->setText(
        QString::number(
            m_fingerprintId
        )
    );


    ui->lbStatus->setText(
        "Sẵn sàng đăng ký"
    );


    ui->btnStart->setEnabled(
        true
    );


    connect(
        ui->btnStart,
        &QPushButton::clicked,
        this,
        &FingerprintEnrollDialog::onStartClicked
    );


    connect(
        ui->btnCancel,
        &QPushButton::clicked,
        this,
        &QDialog::reject
    );
}


FingerprintEnrollDialog::~FingerprintEnrollDialog()
{
    delete ui;
}


// =====================================================
// GETTERS
// =====================================================

QString FingerprintEnrollDialog::userCode() const
{
    return m_userCode;
}


QString FingerprintEnrollDialog::userName() const
{
    return m_userName;
}


int FingerprintEnrollDialog::fingerprintId() const
{
    return m_fingerprintId;
}


// =====================================================
// BẮT ĐẦU ĐĂNG KÝ
// =====================================================

void FingerprintEnrollDialog::onStartClicked()
{
    if (enrolling)
        return;


    enrolling = true;


    ui->btnStart->setEnabled(
        false
    );


    ui->lbStatus->setText(
        "Đang bắt đầu đăng ký..."
    );


    emit startEnroll(
        m_fingerprintId
    );
}


// =====================================================
// CẬP NHẬT TRẠNG THÁI
// =====================================================

void FingerprintEnrollDialog::setStatus(
    const QString &status
)
{
    ui->lbStatus->setText(
        status
    );
}


// =====================================================
// THÀNH CÔNG
// =====================================================

void FingerprintEnrollDialog::enrollSuccess(
    int id
)
{
    enrolling = false;


    ui->lbStatus->setText(
        "Đăng ký thành công!"
    );


    ui->lbFingerprintID->setText(
        QString::number(id)
    );


    QMessageBox::information(
        this,
        "Đăng ký vân tay",
        "Đăng ký vân tay thành công.\n\n"
        "ID vân tay: "
        + QString::number(id)
    );


    accept();
}


// =====================================================
// THẤT BẠI
// =====================================================

void FingerprintEnrollDialog::enrollFailed(
    const QString &reason
)
{
    enrolling = false;


    ui->btnStart->setEnabled(
        true
    );


    ui->lbStatus->setText(
        "Đăng ký thất bại"
    );


    QMessageBox::warning(
        this,
        "Đăng ký vân tay",
        reason
    );
}

