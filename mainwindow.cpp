#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "./ui_userdashboard.h"
#include "userdialog.h"

#include <QHeaderView>
#include <QApplication>
#include <QDateTime>
#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QPushButton>
#include <QLabel>
#include <QTableWidgetItem>
#include "fingerprintenrolldialog.h"
#include "login.h"


// =========================================================
// CONSTRUCTOR
// =========================================================

MainWindow::MainWindow(const QString &username,
                       const QString &role,
                       int userId,
                       QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentUsername(username)
    , currentRole(role)
    , currentUserId(userId)
    , bleClient(new BLEClient(this))
    , database(new Database(this))
{
    ui->setupUi(this);

    // Khởi tạo SQLite TRƯỚC khi đọc thông tin tài khoản User.
    // Nếu đọc user trước khi initialize(), currentUserId có thể
    // chưa truy xuất được dữ liệu nên dashboard sẽ hiện "---".
    const bool databaseReady = database->initialize();

    // =====================================================
    // GIAO DIỆN RIÊNG CHO USER
    // =====================================================
    // Admin giữ nguyên giao diện mainwindow.ui.
    // User dùng userdashboard.ui cho tab Tổng quan và
    // chỉ giữ lại tab Nhật ký.
    if (currentRole != "admin")
    {
        // Ẩn toàn bộ nội dung Tổng quan cũ.
        const auto oldOverviewWidgets =
            ui->tabOverview->findChildren<QWidget *>();

        for (QWidget *widget : oldOverviewWidgets)
            widget->setVisible(false);

        // Tạo dashboard User từ userdashboard.ui.
        QWidget *userDashboard =
            new QWidget(ui->tabOverview);

        userDashboard->setObjectName("UserDashboard");

        Ui::UserDashboard dashboardUi;
        dashboardUi.setupUi(userDashboard);

        ui->tabOverview->layout()->addWidget(userDashboard);

        // Chỉ User xem giao diện Tổng quan mới.
        userDashboard->show();

        // Nhật ký User chỉ hiển thị 4 thông tin cần thiết.
        ui->tblHistory->setColumnCount(4);
        ui->tblHistory->setHorizontalHeaderLabels(
            QStringList()
                << "Thời gian"
                << "Chiều"
                << "Kết quả"
                << "Barrier"
            );

        ui->tblHistory->setColumnWidth(0, 170);
        ui->tblHistory->setColumnWidth(1, 120);
        ui->tblHistory->setColumnWidth(2, 180);
        ui->tblHistory->setColumnWidth(3, 130);

        // User không cần bộ lọc ngày và nút xuất dữ liệu
        // trên giao diện đơn giản.
        ui->dateFrom->setVisible(false);
        ui->dateTo->setVisible(false);
        ui->btnFilterHistory->setVisible(false);
        ui->btnExport->setVisible(false);

        // Ẩn các nhãn của khu vực lọc nếu có.
        const auto historyLabels =
            ui->tabHistory->findChildren<QLabel *>();

        for (QLabel *label : historyLabels)
        {
            const QString text = label->text();

            if (text.contains("Từ ngày") ||
                text.contains("Đến ngày") ||
                text.contains("Từ") ||
                text.contains("Đến"))
            {
                label->setVisible(false);
            }
        }

        // Hiển thị thông tin tài khoản thật.
        QVariantList currentUser;

        if (databaseReady && currentUserId > 0)
        {
            currentUser =
                database->getUserById(currentUserId);
        }

        QString userCode = "---";
        QString userName = currentUsername;
        QString department = "---";
        QString fingerprintId = "Chưa đăng ký";
        QString accountStatus = "Hoạt động";

        if (!currentUser.isEmpty())
        {
            const QVariantMap user =
                currentUser.first().toMap();

            userCode =
                user["user_code"].toString();

            if (!user["name"].toString().isEmpty())
                userName =
                    user["name"].toString();

            if (!user["department"].toString().isEmpty())
                department =
                    user["department"].toString();

            if (user["fingerprint_id"].isValid() &&
                !user["fingerprint_id"].isNull() &&
                user["fingerprint_id"].toInt() > 0)
            {
                fingerprintId =
                    QString::number(user["fingerprint_id"].toInt());
            }

            if (!user["status"].toString().isEmpty())
                accountStatus =
                    user["status"].toString();
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lblGreeting"))
        {
            label->setText(
                QString("Xin chào, %1!")
                    .arg(userName)
                );
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lblUserCode"))
        {
            label->setText(
                QString("Mã người dùng: %1")
                    .arg(userCode)
                );
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbAccountCode"))
        {
            label->setText(userCode);
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbAccountName"))
        {
            label->setText(userName);
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbDepartment"))
        {
            label->setText(department);
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbUsername"))
        {
            label->setText(currentUsername);
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbFingerprintId"))
        {
            label->setText(fingerprintId);
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbAccountStatus"))
        {
            label->setText(accountStatus);
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbTodayCount"))
        {
            label->setText("0");
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbGateStatus"))
        {
            label->setText("SẴN SÀNG");
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbLastAccess"))
        {
            label->setText("Chưa có dữ liệu");
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbLastResult"))
        {
            label->setText("Chưa có lần xác thực");
            label->setStyleSheet(
                "font-size:18px;"
                "font-weight:bold;"
                "color:#607D8B;"
                "padding:6px;"
                );
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbLastUser"))
        {
            label->setText("---");
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbLastTime"))
        {
            label->setText("--/--/---- --:--:--");
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbLastBarrier"))
        {
            label->setText("Barrier: Đang đóng");
        }

        if (QLabel *label =
                userDashboard->findChild<QLabel *>("lbLastDirection"))
        {
            label->setText("Chiều: ---");
        }
    }

    // =====================================================
    // TÀI KHOẢN HIỆN TẠI + MENU HỆ THỐNG
    // =====================================================

    // Menu "Hệ thống" đã có sẵn trong mainwindow.ui.
    // Không tạo thêm menu mới để tránh bị trùng 2 mục "Hệ thống".
    QMenu *systemMenu = nullptr;

    if (!menuBar()->actions().isEmpty())
    {
        QAction *firstMenuAction = menuBar()->actions().first();

        if (firstMenuAction)
            systemMenu = firstMenuAction->menu();
    }

    if (systemMenu)
    {
        // Xóa các action cũ trong menu "Hệ thống"
        // rồi tạo lại đúng 3 chức năng cần dùng.
        systemMenu->clear();

        QAction *actionChangePassword =
            new QAction("Đổi mật khẩu", this);

        QAction *actionLogout =
            new QAction("Đăng xuất", this);

        QAction *actionExit =
            new QAction("Thoát", this);

        actionChangePassword->setObjectName(
            "actionChangePassword");

        actionLogout->setObjectName(
            "actionLogout");

        actionExit->setObjectName(
            "actionExit");

        systemMenu->addAction(actionChangePassword);
        systemMenu->addAction(actionLogout);
        systemMenu->addSeparator();
        systemMenu->addAction(actionExit);

        connect(
            actionChangePassword,
            &QAction::triggered,
            this,
            &MainWindow::changePassword
            );

        connect(
            actionLogout,
            &QAction::triggered,
            this,
            &MainWindow::logout
            );

        connect(
            actionExit,
            &QAction::triggered,
            this,
            &MainWindow::exitApplication
            );
    }

    // Áp dụng quyền ngay khi mở MainWindow.
    applyPermissions();
    // =====================================================
    // CĂN GỌN THANH LỌC NHẬT KÝ
    // =====================================================

    ui->dateFrom->setFixedWidth(150);
    ui->dateTo->setFixedWidth(150);

    ui->btnFilterHistory->setFixedWidth(100);
    ui->btnExport->setFixedWidth(120);

    // Không để các widget bị layout ép giãn
    ui->dateFrom->setSizePolicy(
        QSizePolicy::Fixed,
        QSizePolicy::Fixed
        );

    ui->dateTo->setSizePolicy(
        QSizePolicy::Fixed,
        QSizePolicy::Fixed
        );

    ui->btnFilterHistory->setSizePolicy(
        QSizePolicy::Fixed,
        QSizePolicy::Fixed
        );

    ui->btnExport->setSizePolicy(
        QSizePolicy::Fixed,
        QSizePolicy::Fixed
        );


    // =====================================================
    // CẤU HÌNH BẢNG NHẬT KÝ
    // =====================================================

    ui->tblRecentLog->setColumnWidth(0, 160);
    ui->tblRecentLog->setColumnWidth(1, 160);
    ui->tblRecentLog->setColumnWidth(2, 100);
    ui->tblRecentLog->setColumnWidth(3, 170);
    ui->tblRecentLog->setColumnWidth(4, 170);

    ui->tblRecentLog->horizontalHeader()
        ->setStretchLastSection(false);

    ui->tblRecentLog->verticalHeader()
        ->setDefaultSectionSize(32);


    // =====================================================
    // KHỞI TẠO TRẠNG THÁI
    // =====================================================

    vehiclesInParking = 0;
    todayAccessCount = 0;
    vehicleWaiting = false;

    // =====================================================
    // GIÁ TRỊ BAN ĐẦU
    // =====================================================

    ui->lbESP->setText("Offline");
    ui->lbBLE->setText("Đang quét...");
    ui->lbSQLite->setText("Đang khởi tạo...");
    ui->lbSensor->setText("Sẵn sàng");
    ui->lbFinger->setText("Sẵn sàng");
    ui->lbServo->setText("Đang đóng");

    // 4 thiết bị phần cứng của hệ thống:
    // ESP32, VL53L0X, AS608 và Servo Barrier.
    // Chỉ tính là Online khi ESP32 đã kết nối BLE.
    ui->lbOnline->setText("0 / 4");
    ui->lbToday->setText("0");

    ui->lbFingerID->setText("---");
    ui->lbResult->setText("Chưa xác thực");
    ui->lbBarrier->setText("Đang đóng");
    ui->lbTime->setText("--/--/---- --:--:--");

    ui->lbName->setText("---");
    ui->lbID->setText("---");

    ui->tblRecentLog->setRowCount(0);
    ui->tblHistory->setRowCount(0);


    // =====================================================
    // KHỞI TẠO SQLITE
    // =====================================================

    if (databaseReady)
    {
        ui->lbSQLite->setText("Ready");

        // Load người dùng
        loadUsers();

        // Load nhật ký cũ từ SQLite
        loadAccessLogs();

        // =====================================================
        // LỌC NHẬT KÝ THEO KHOẢNG NGÀY
        // =====================================================
        // Mặc định hiển thị khoảng từ đầu ngày hiện tại đến
        // ngày hiện tại. Người dùng có thể đổi lại hai ngày
        // trên giao diện rồi bấm nút "Lọc".
        ui->dateFrom->setDate(QDate::currentDate());
        ui->dateTo->setDate(QDate::currentDate());

        connect(
            ui->btnFilterHistory,
            &QPushButton::clicked,
            this,
            [this]()
            {
                QDate fromDate = ui->dateFrom->date();
                QDate toDate = ui->dateTo->date();

                if (fromDate > toDate)
                {
                    QMessageBox::warning(
                        this,
                        "Lọc nhật ký",
                        "Ngày bắt đầu không được lớn hơn ngày kết thúc."
                    );

                    return;
                }

                QVariantList logs;

                if (currentRole == "admin")
                {
                    logs = database->getAccessLogs(1000);
                }
                else
                {
                    QString userCode;

                    QVariantList user =
                        database->getUserById(currentUserId);

                    if (!user.isEmpty())
                    {
                        userCode =
                            user.first().toMap()["user_code"].toString();
                    }

                    if (!userCode.isEmpty())
                    {
                        logs =
                            database->getAccessLogsForUser(
                                userCode,
                                1000
                                );
                    }
                }

                ui->tblHistory->setRowCount(0);

                for (const QVariant &value : logs)
                {
                    QVariantMap log =
                        value.toMap();

                    QString time =
                        log["time"].toString();

                    QDateTime dateTime =
                        QDateTime::fromString(
                            time,
                            "dd/MM/yyyy hh:mm:ss"
                        );

                    if (!dateTime.isValid())
                        continue;

                    QDate logDate =
                        dateTime.date();

                    if (logDate < fromDate ||
                        logDate > toDate)
                    {
                        continue;
                    }

                    addHistoryLog(
                        time,
                        log["name"].toString(),
                        log["user_code"].toString(),
                        log["fingerprint_id"].toString(),
                        log["result"].toString(),
                        log["barrier"].toString(),
                        log["direction"].toString()
                    );
                }
            }
        );


        // =====================================================
        // XUẤT DỮ LIỆU NHẬT KÝ
        // =====================================================
        // Xuất đúng dữ liệu đang hiển thị trong tblHistory.
        // Nếu người dùng đã lọc theo ngày thì file chỉ chứa
        // các bản ghi sau khi lọc.
        connect(
            ui->btnExport,
            &QPushButton::clicked,
            this,
            [this]()
            {
                if (ui->tblHistory->rowCount() == 0)
                {
                    QMessageBox::information(
                        this,
                        "Xuất dữ liệu",
                        "Không có dữ liệu để xuất."
                    );

                    return;
                }

                QString defaultName =
                    QString("nhat_ky_%1.csv")
                        .arg(
                            QDate::currentDate()
                                .toString("ddMMyyyy")
                        );

                QString fileName =
                    QFileDialog::getSaveFileName(
                        this,
                        "Xuất dữ liệu nhật ký",
                        defaultName,
                        "CSV Files (*.csv);;All Files (*)"
                    );

                if (fileName.isEmpty())
                    return;

                if (!fileName.endsWith(".csv", Qt::CaseInsensitive))
                    fileName += ".csv";

                QFile file(fileName);

                if (!file.open(QIODevice::WriteOnly |
                               QIODevice::Text))
                {
                    QMessageBox::critical(
                        this,
                        "Xuất dữ liệu",
                        "Không thể tạo file xuất dữ liệu."
                    );

                    return;
                }

                QTextStream out(&file);
                out.setEncoding(QStringConverter::Utf8);

                // BOM để Excel trên Windows nhận đúng tiếng Việt.
                out << QChar(0xFEFF);

                auto csvValue = [](const QString &value)
                {
                    QString escaped = value;
                    escaped.replace('"', """");
                    return QString("\"%1\"").arg(escaped);
                };

                // Tiêu đề 7 cột.
                out
                    << csvValue("Thời gian") << ","
                    << csvValue("Họ tên") << ","
                    << csvValue("Mã người dùng") << ","
                    << csvValue("ID vân tay") << ","
                    << csvValue("Chiều") << ","
                    << csvValue("Kết quả") << ","
                    << csvValue("Barrier") << "\n";

                for (int row = 0;
                     row < ui->tblHistory->rowCount();
                     ++row)
                {
                    for (int column = 0;
                         column < ui->tblHistory->columnCount();
                         ++column)
                    {
                        QTableWidgetItem *item =
                            ui->tblHistory->item(row, column);

                        QString value =
                            item ? item->text() : QString();

                        out << csvValue(value);

                        if (column < ui->tblHistory->columnCount() - 1)
                            out << ",";
                    }

                    out << "\n";
                }

                file.close();

                QMessageBox::information(
                    this,
                    "Xuất dữ liệu",
                    QString("Đã xuất %1 bản ghi thành công.\n\nFile: %2")
                        .arg(
                            ui->tblHistory->rowCount()
                        )
                        .arg(
                            fileName
                        )
                );
            }
        );
    }
    else
    {
        ui->lbSQLite->setText("Error");

        QMessageBox::warning(
            this,
            "SQLite",
            "Không thể khởi tạo cơ sở dữ liệu."
        );
    }


    // =====================================================
    // THÊM NGƯỜI DÙNG
    // =====================================================

    connect(
        ui->btnAddUser,
        &QPushButton::clicked,
        this,
        [this]()
        {
            UserDialog dialog(this);

            if (dialog.exec() != QDialog::Accepted)
                return;


            if (dialog.userCode().isEmpty() ||
                dialog.userName().isEmpty())
            {
                QMessageBox::warning(
                    this,
                    "Lỗi",
                    "Mã người dùng và họ tên không được để trống."
                );

                return;
            }


            if (database->addUser(
                    dialog.userCode(),
                    dialog.userName(),
                    dialog.department(),
                    dialog.fingerprintId(),
                    dialog.status(),
                    dialog.note()))
            {
                loadUsers();

                QMessageBox::information(
                    this,
                    "Thành công",
                    "Đã thêm người dùng."
                );
            }
            else
            {
                QMessageBox::warning(
                    this,
                    "Lỗi",
                    "Không thể thêm người dùng.\n"
                    "Có thể mã người dùng hoặc ID vân tay đã tồn tại."
                );
            }
        }
    );

    // =====================================================
    // ĐĂNG KÝ VÂN TAY
    // =====================================================

    connect(
        ui->btnEnroll,
        &QPushButton::clicked,
        this,
        [this]()
        {
            // ---------------------------------------------
            // KIỂM TRA ĐÃ CHỌN NGƯỜI DÙNG CHƯA
            // ---------------------------------------------

            int row =
                ui->tblUsers->currentRow();

            if (row < 0)
            {
                QMessageBox::warning(
                    this,
                    "Đăng ký vân tay",
                    "Vui lòng chọn người dùng cần đăng ký."
                    );

                return;
            }


            QTableWidgetItem *codeItem =
                ui->tblUsers->item(row, 0);

            QTableWidgetItem *nameItem =
                ui->tblUsers->item(row, 1);

            if (!codeItem || !nameItem)
                return;


            QString userCode =
                codeItem->text();

            QString userName =
                nameItem->text();


            int userId =
                codeItem->data(
                            Qt::UserRole
                            ).toInt();


            if (userId <= 0)
            {
                QMessageBox::warning(
                    this,
                    "Lỗi",
                    "Không xác định được người dùng."
                    );

                return;
            }


            // ---------------------------------------------
            // TÌM ID VÂN TAY TRỐNG
            // ---------------------------------------------

            QVariantList users =
                database->getUsers();

            int nextFingerprintId = 1;


            while (true)
            {
                bool used = false;


                for (const QVariant &value : users)
                {
                    QVariantMap user =
                        value.toMap();


                    QVariant fp =
                        user["fingerprint_id"];


                    if (
                        fp.isValid() &&
                        !fp.isNull() &&
                        fp.toInt() ==
                            nextFingerprintId
                        )
                    {
                        used = true;
                        break;
                    }
                }


                if (!used)
                    break;


                nextFingerprintId++;


                if (nextFingerprintId > 127)
                {
                    QMessageBox::warning(
                        this,
                        "Lỗi",
                        "Không còn ID vân tay trống."
                        );

                    return;
                }
            }


            // ---------------------------------------------
            // TẠO DIALOG
            // ---------------------------------------------

            FingerprintEnrollDialog dialog(
                userCode,
                userName,
                nextFingerprintId,
                this
                );


            // ---------------------------------------------
            // BẮT ĐẦU ENROLL
            // ---------------------------------------------

            connect(
                &dialog,
                &FingerprintEnrollDialog::startEnroll,
                this,
                [this, &dialog](int id)
                {
                    dialog.setStatus(
                        "Đang gửi lệnh đến ESP32..."
                        );

                    bleClient->startFingerprintEnroll(
                        id
                        );
                }
                );


            // ---------------------------------------------
            // ESP32 -> DIALOG: TIẾN TRÌNH ĐĂNG KÝ
            // ---------------------------------------------
            // BLEClient đã chuyển các mã:
            // ENROLL_STATUS,1 ... ENROLL_STATUS,6
            // thành câu tiếng Việt hoàn chỉnh trước khi phát
            // signal statusReceived().
            // Hiển thị trực tiếp vào ô Trạng thái của dialog.
            // ---------------------------------------------

            connect(
                bleClient,
                &BLEClient::statusReceived,
                &dialog,
                [&dialog](const QString &status)
                {
                    const QString prefix =
                        "ENROLL_STATUS,";

                    if (!status.startsWith(prefix))
                    {
                        return;
                    }

                    const QString message =
                        status.mid(prefix.length()).trimmed();

                    if (!message.isEmpty())
                    {
                        dialog.setStatus(message);
                    }
                }
                );


            // ---------------------------------------------
            // ENROLL THÀNH CÔNG
            // ---------------------------------------------

            connect(
                bleClient,
                &BLEClient::fingerprintEnrollSuccess,
                &dialog,
                [this, &dialog, userId](int id)
                {
                    QVariantList result =
                        database->getUserById(
                            userId
                            );


                    if (result.isEmpty())
                    {
                        dialog.enrollFailed(
                            "Không tìm thấy người dùng trong SQLite."
                            );

                        return;
                    }


                    QVariantMap user =
                        result.first().toMap();


                    bool success =
                        database->updateUser(
                            userId,

                            user["user_code"]
                                .toString(),

                            user["name"]
                                .toString(),

                            user["department"]
                                .toString(),

                            id,

                            user["status"]
                                .toString(),

                            user["note"]
                                .toString()
                            );


                    if (!success)
                    {
                        dialog.enrollFailed(
                            "AS608 đã đăng ký thành công "
                            "nhưng SQLite không cập nhật được."
                            );

                        return;
                    }


                    // Cập nhật bảng người dùng
                    loadUsers();

                    // Hiển thị thành công
                    dialog.enrollSuccess(id);
                }
                );


            // ---------------------------------------------
            // ENROLL THẤT BẠI
            // ---------------------------------------------

            connect(
                bleClient,
                &BLEClient::fingerprintEnrollFailed,
                &dialog,
                [&dialog](const QString &reason)
                {
                    dialog.enrollFailed(
                        reason
                        );
                }
                );


            // ---------------------------------------------
            // HIỂN THỊ DIALOG
            // ---------------------------------------------

            dialog.exec();
        }
        );


    // =====================================================
    // SỬA NGƯỜI DÙNG
    // =====================================================

    connect(
        ui->btnEditUser,
        &QPushButton::clicked,
        this,
        [this]()
        {
            int row =
                ui->tblUsers->currentRow();

            if (row < 0)
            {
                QMessageBox::warning(
                    this,
                    "Thông báo",
                    "Vui lòng chọn người dùng cần sửa."
                );

                return;
            }


            QTableWidgetItem *idItem =
                ui->tblUsers->item(row, 0);

            if (!idItem)
                return;


            int id =
                idItem->data(Qt::UserRole).toInt();


            QVariantList users =
                database->getUserById(id);

            if (users.isEmpty())
            {
                QMessageBox::warning(
                    this,
                    "Lỗi",
                    "Không tìm thấy người dùng trong cơ sở dữ liệu."
                );

                return;
            }


            QVariantMap user =
                users.first().toMap();


            UserDialog dialog(this);

            dialog.setUserCode(
                user["user_code"].toString()
            );

            dialog.setUserName(
                user["name"].toString()
            );

            dialog.setDepartment(
                user["department"].toString()
            );

            dialog.setFingerprintId(
                user["fingerprint_id"].toInt()
            );

            dialog.setStatus(
                user["status"].toString()
            );

            dialog.setNote(
                user["note"].toString()
            );


            if (dialog.exec() != QDialog::Accepted)
                return;


            if (dialog.userCode().isEmpty() ||
                dialog.userName().isEmpty())
            {
                QMessageBox::warning(
                    this,
                    "Lỗi",
                    "Mã người dùng và họ tên không được để trống."
                );

                return;
            }


            if (database->updateUser(
                    id,
                    dialog.userCode(),
                    dialog.userName(),
                    dialog.department(),
                    dialog.fingerprintId(),
                    dialog.status(),
                    dialog.note()))
            {
                loadUsers();

                QMessageBox::information(
                    this,
                    "Thành công",
                    "Đã cập nhật người dùng."
                );
            }
            else
            {
                QMessageBox::warning(
                    this,
                    "Lỗi",
                    "Không thể cập nhật người dùng."
                );
            }
        }
    );


    // =====================================================
    // XÓA NGƯỜI DÙNG
    // =====================================================

    connect(
        ui->btnDeleteUser,
        &QPushButton::clicked,
        this,
        [this]()
        {
            int row =
                ui->tblUsers->currentRow();

            if (row < 0)
            {
                QMessageBox::warning(
                    this,
                    "Thông báo",
                    "Vui lòng chọn người dùng cần xóa."
                );

                return;
            }


            QTableWidgetItem *idItem =
                ui->tblUsers->item(row, 0);

            if (!idItem)
                return;


            QString userCode =
                ui->tblUsers->item(row, 0)->text();

            QString userName =
                ui->tblUsers->item(row, 1)->text();


            QMessageBox::StandardButton answer =
                QMessageBox::question(
                    this,
                    "Xác nhận xóa",
                    "Bạn có chắc muốn xóa người dùng:\n\n"
                    + userCode
                    + " - "
                    + userName
                    + "?",
                    QMessageBox::Yes |
                    QMessageBox::No
                );


            if (answer != QMessageBox::Yes)
                return;


            int id =
                idItem->data(Qt::UserRole).toInt();


            if (database->deleteUser(id))
            {
                loadUsers();

                QMessageBox::information(
                    this,
                    "Thành công",
                    "Đã xóa người dùng."
                );
            }
            else
            {
                QMessageBox::warning(
                    this,
                    "Lỗi",
                    "Không thể xóa người dùng."
                );
            }
        }
    );


    // =====================================================
    // TÌM KIẾM NGƯỜI DÙNG
    // =====================================================

    connect(
        ui->btnSearch,
        &QPushButton::clicked,
        this,
        [this]()
        {
            QString keyword =
                ui->txtSearch->text().trimmed();


            if (keyword.isEmpty())
            {
                loadUsers();
                return;
            }


            QVariantList users =
                database->searchUsers(keyword);


            ui->tblUsers->setRowCount(0);


            for (const QVariant &value : users)
            {
                QVariantMap user =
                    value.toMap();


                int row =
                    ui->tblUsers->rowCount();


                ui->tblUsers->insertRow(row);


                QTableWidgetItem *code =
                    new QTableWidgetItem(
                        user["user_code"].toString()
                    );


                code->setData(
                    Qt::UserRole,
                    user["id"]
                );


                ui->tblUsers->setItem(
                    row,
                    0,
                    code
                );


                ui->tblUsers->setItem(
                    row,
                    1,
                    new QTableWidgetItem(
                        user["name"].toString()
                    )
                );


                ui->tblUsers->setItem(
                    row,
                    2,
                    new QTableWidgetItem(
                        user["department"].toString()
                    )
                );


                ui->tblUsers->setItem(
                    row,
                    3,
                    new QTableWidgetItem(
                        user["fingerprint_id"].toString()
                    )
                );


                ui->tblUsers->setItem(
                    row,
                    4,
                    new QTableWidgetItem(
                        user["status"].toString()
                    )
                );


                ui->tblUsers->setItem(
                    row,
                    5,
                    new QTableWidgetItem(
                        user["note"].toString()
                    )
                );
            }


            refreshUserCount();
        }
    );


    // =====================================================
    // TÌM KIẾM BẰNG ENTER
    // =====================================================

    connect(
        ui->txtSearch,
        &QLineEdit::returnPressed,
        ui->btnSearch,
        &QPushButton::click
    );


    // =====================================================
    // BLE LOG
    // =====================================================

    connect(
        bleClient,
        &BLEClient::logMessage,
        this,
        [](const QString &message)
        {
            qDebug()
                << "[BLE]"
                << message;
        }
    );


    // =====================================================
    // BLE CONNECTED
    // =====================================================

    connect(
        bleClient,
        &BLEClient::bleConnected,
        this,
        [this]()
        {
            qDebug()
                << "[BLE] ESP32 CONNECTED";


            ui->lbESP->setText(
                "Online"
            );

            ui->lbBLE->setText(
                "Connected"
            );

            // ESP32 đã kết nối BLE -> 4 thiết bị phần cứng
            // được xem là sẵn sàng trong kiến trúc hiện tại.
            ui->lbOnline->setText(
                "4 / 4"
            );

            ui->lbBLESettingStatus->setText(
                "Đã kết nối"
            );
        }
    );


    // =====================================================
    // BLE DISCONNECTED
    // =====================================================

    connect(
        bleClient,
        &BLEClient::bleDisconnected,
        this,
        [this]()
        {
            qDebug()
                << "[BLE] ESP32 DISCONNECTED";


            ui->lbESP->setText(
                "Offline"
            );

            ui->lbBLE->setText(
                "Disconnected"
            );

            // Mất kết nối ESP32 -> các thiết bị phía ESP32
            // không còn được xem là Online từ phía Qt.
            ui->lbOnline->setText(
                "0 / 4"
            );

            ui->lbBLESettingStatus->setText(
                "Mất kết nối"
            );
        }
    );


    // =====================================================
    // STATUS ESP32
    // =====================================================

    connect(
        bleClient,
        &BLEClient::statusReceived,
        this,
        [this](const QString &status)
        {
            qDebug()
                << "[ESP32 STATUS]"
                << status;
        }
    );


    // =====================================================
    // VL53L0X - PHÁT HIỆN XE
    // =====================================================

    connect(
        bleClient,
        &BLEClient::vehicleDetected,
        this,
        [this](int distance)
        {
            qDebug()
                << "[VL53L0X] Vehicle detected:"
                << distance
                << "mm";


            vehicleWaiting = true;


            ui->lbSensor->setText(
                "Có xe"
            );

            if (currentRole != "admin")
            {
                QWidget *dashboard =
                    ui->tabOverview->findChild<QWidget *>(
                        "UserDashboard"
                        );

                if (dashboard)
                {
                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateStatus"))
                    {
                        label->setText("ĐANG XỬ LÝ");
                        label->setStyleSheet(
                            "font-size:22px;"
                            "font-weight:bold;"
                            "color:#FB8C00;"
                            );
                    }

                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateDot"))
                    {
                        label->setStyleSheet(
                            "font-size:20px;color:#FB8C00;"
                            );
                    }

                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateReady"))
                    {
                        label->setText(
                            "●  Đang xử lý phương tiện"
                            );
                        label->setStyleSheet(
                            "font-size:15px;"
                            "color:#FB8C00;"
                            "padding:4px;"
                            );
                    }
                }
            }
        }
    );


    // =====================================================
    // AS608 - VÂN TAY THÀNH CÔNG
    // =====================================================

    connect(
        bleClient,
        &BLEClient::fingerprintOK,
        this,
        [this](int id, int confidence)
        {
            Q_UNUSED(confidence);


            QString time =
                QDateTime::currentDateTime()
                    .toString(
                        "dd/MM/yyyy hh:mm:ss"
                    );


            QString fingerId =
                QString::number(id);


            qDebug()
                << "[AS608] Fingerprint OK:"
                << id;


            // -------------------------------------------------
            // TÌM NGƯỜI DÙNG THEO ID VÂN TAY
            // -------------------------------------------------

            QString userCode = "---";
            QString userName = "---";


            QVariantList users =
                database->getUserByFingerprintId(id);


            if (!users.isEmpty())
            {
                QVariantMap user =
                    users.first().toMap();


                userCode =
                    user["user_code"].toString();

                userName =
                    user["name"].toString();


                qDebug()
                    << "[USER]"
                    << userCode
                    << userName;
            }


            // -------------------------------------------------
            // HIỂN THỊ NGƯỜI DÙNG
            // -------------------------------------------------

            ui->lbName->setText(
                userName
            );

            ui->lbID->setText(
                userCode
            );


            // -------------------------------------------------
            // HIỂN THỊ VÂN TAY
            // -------------------------------------------------

            ui->lbFinger->setText(
                "Đã quét"
            );

            ui->lbFingerID->setText(
                fingerId
            );

            ui->lbResult->setText(
                "Xác thực thành công"
            );

            ui->lbTime->setText(
                time
            );

            // Cập nhật xác thực gần nhất cho User.
            if (currentRole != "admin")
            {
                QWidget *dashboard =
                    ui->tabOverview->findChild<QWidget *>(
                        "UserDashboard"
                        );

                if (dashboard)
                {
                    QVariantList currentUser =
                        database->getUserByFingerprintId(id);

                    bool isCurrentUser = false;

                    if (!currentUser.isEmpty())
                    {
                        const QVariantMap user =
                            currentUser.first().toMap();

                        isCurrentUser =
                            user["id"].toInt() == currentUserId;
                    }

                    if (isCurrentUser)
                    {
                        if (QLabel *label =
                                dashboard->findChild<QLabel *>(
                                    "lbLastResult"))
                        {
                            label->setText(
                                "✓  Xác thực thành công"
                                );
                            label->setStyleSheet(
                                "font-size:18px;"
                                "font-weight:bold;"
                                "color:#2E7D32;"
                                "padding:6px;"
                                );
                        }

                        if (QLabel *label =
                                dashboard->findChild<QLabel *>(
                                    "lbLastUser"))
                        {
                            label->setText(
                                QString("%1  •  %2  •  Vân tay %3")
                                    .arg(userName)
                                    .arg(userCode)
                                    .arg(fingerId)
                                );
                        }

                        if (QLabel *label =
                                dashboard->findChild<QLabel *>(
                                    "lbLastTime"))
                        {
                            label->setText(time);
                        }

                        if (QLabel *label =
                                dashboard->findChild<QLabel *>(
                                    "lbLastBarrier"))
                        {
                            label->setText(
                                "Barrier: Đang xử lý"
                                );
                        }
                    }
                }
            }


            // -------------------------------------------------
            // XE VÀO
            // -------------------------------------------------

            if (vehicleWaiting)
            {
                vehiclesInParking++;
                todayAccessCount++;

                vehicleWaiting = false;

                ui->lbToday->setText(
                    QString::number(
                        todayAccessCount
                    )
                );


                ui->lbSensor->setText(
                    "Đã xác thực"
                );


                qDebug()
                    << "[PARKING] Vehicle IN";


                qDebug()
                    << "[PARKING] Vehicles:"
                    << vehiclesInParking;


                qDebug()
                    << "[PARKING] Today:"
                    << todayAccessCount;


                // =================================================
                // LƯU NHẬT KÝ VÀO SQLITE
                // =================================================

                database->addAccessLog(
                    userCode,
                    userName,
                    id,
                    "Xác thực thành công",
                    "Mở",
                    "Xe vào"
                );


                // =================================================
                // HIỂN THỊ NHẬT KÝ THEO QUYỀN
                // =================================================
                // Admin xem toàn bộ. User chỉ xem bản ghi của
                // người dùng được liên kết với tài khoản đăng nhập.
                QString currentUserCode;

                if (currentRole != "admin")
                {
                    QVariantList currentUser =
                        database->getUserById(currentUserId);

                    if (!currentUser.isEmpty())
                    {
                        currentUserCode =
                            currentUser.first()
                                .toMap()["user_code"]
                                .toString();
                    }
                }

                const bool canViewThisLog =
                    (currentRole == "admin") ||
                    (!currentUserCode.isEmpty() &&
                     currentUserCode == userCode);

                if (canViewThisLog)
                {
                    addRecentLog(
                        time,
                        userName,
                        fingerId,
                        "Xác thực thành công",
                        "Mở"
                    );

                    addHistoryLog(
                        time,
                        userName,
                        userCode,
                        fingerId,
                        "Xác thực thành công",
                        "Mở",
                        "Xe vào"
                    );
                }
            }
        }
    );


    // =====================================================
    // AS608 - VÂN TAY THẤT BẠI
    // =====================================================

    connect(
        bleClient,
        &BLEClient::fingerprintFail,
        this,
        [this]()
        {
            QString time =
                QDateTime::currentDateTime()
                    .toString(
                        "dd/MM/yyyy hh:mm:ss"
                    );


            qDebug()
                << "[AS608] Fingerprint FAIL";


            ui->lbFinger->setText(
                "Đã quét"
            );

            ui->lbFingerID->setText(
                "---"
            );

            ui->lbResult->setText(
                "Xác thực thất bại"
            );

            ui->lbBarrier->setText(
                "Đang đóng"
            );

            ui->lbTime->setText(
                time
            );

            ui->lbName->setText(
                "---"
            );

            ui->lbID->setText(
                "---"
            );

            // =================================================
            // GHI NHẬT KÝ XÁC THỰC THẤT BẠI
            // =================================================

            database->addAccessLog(
                "---",
                "---",
                -1,
                "Xác thực thất bại",
                "Đóng",
                "Xe vào"
            );


            // =================================================
            // NHẬT KÝ HIỂN THỊ
            // =================================================
            // Xác thực thất bại không xác định được userCode,
            // vì vậy chỉ Admin được xem bản ghi này trên GUI.
            if (currentRole == "admin")
            {
                addRecentLog(
                    time,
                    "---",
                    "---",
                    "Xác thực thất bại",
                    "Đóng"
                );

                addHistoryLog(
                    time,
                    "---",
                    "---",
                    "---",
                    "Xác thực thất bại",
                    "Đóng",
                    "Xe vào"
                );
            }
        }
    );


    // =====================================================
    // SERVO - MỞ
    // =====================================================

    connect(
        bleClient,
        &BLEClient::barrierOpened,
        this,
        [this]()
        {
            qDebug()
                << "[SERVO] Barrier OPENED";


            ui->lbBarrier->setText(
                "Đang mở"
            );

            ui->lbServo->setText(
                "Đang mở"
            );

            if (currentRole != "admin")
            {
                QWidget *dashboard =
                    ui->tabOverview->findChild<QWidget *>(
                        "UserDashboard"
                        );

                if (dashboard)
                {
                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateStatus"))
                    {
                        label->setText("ĐANG MỞ");
                        label->setStyleSheet(
                            "font-size:22px;"
                            "font-weight:bold;"
                            "color:#FB8C00;"
                            );
                    }

                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateDot"))
                    {
                        label->setStyleSheet(
                            "font-size:20px;color:#FB8C00;"
                            );
                    }

                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbLastBarrier"))
                    {
                        label->setText("Barrier: Đã mở");
                    }

                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateReady"))
                    {
                        label->setText(
                            "●  Cổng đang mở"
                            );
                        label->setStyleSheet(
                            "font-size:15px;"
                            "color:#FB8C00;"
                            "padding:4px;"
                            );
                    }
                }
            }
        }
    );


    // =====================================================
    // SERVO - ĐÓNG
    // =====================================================

    connect(
        bleClient,
        &BLEClient::barrierClosed,
        this,
        [this]()
        {
            qDebug()
                << "[SERVO] Barrier CLOSED";


            ui->lbBarrier->setText(
                "Đang đóng"
            );

            ui->lbServo->setText(
                "Đang đóng"
            );

            if (currentRole != "admin")
            {
                QWidget *dashboard =
                    ui->tabOverview->findChild<QWidget *>(
                        "UserDashboard"
                        );

                if (dashboard)
                {
                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateStatus"))
                    {
                        label->setText("SẴN SÀNG");
                        label->setStyleSheet(
                            "font-size:22px;"
                            "font-weight:bold;"
                            "color:#2E7D32;"
                            );
                    }

                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateDot"))
                    {
                        label->setStyleSheet(
                            "font-size:20px;color:#43A047;"
                            );
                    }

                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateReady"))
                    {
                        label->setText(
                            "●  Cổng sẵn sàng"
                            );
                        label->setStyleSheet(
                            "font-size:15px;"
                            "color:#2E7D32;"
                            "padding:4px;"
                            );
                    }
                }
            }
        }
    );


    // =====================================================
    // XE RA
    // =====================================================

    connect(
        bleClient,
        &BLEClient::vehicleExit,
        this,
        [this]()
        {
            QString time =
                QDateTime::currentDateTime()
                    .toString(
                        "dd/MM/yyyy hh:mm:ss"
                    );


            qDebug()
                << "[PARKING] Vehicle EXIT";


            if (vehiclesInParking > 0)
            {
                vehiclesInParking--;
            }


            if (currentRole == "admin")
            {
                todayAccessCount++;

                ui->lbToday->setText(
                    QString::number(
                        todayAccessCount
                    )
                );
            }


            ui->lbSensor->setText(
                "Không có xe"
            );

            if (currentRole != "admin")
            {
                QWidget *dashboard =
                    ui->tabOverview->findChild<QWidget *>(
                        "UserDashboard"
                        );

                if (dashboard)
                {
                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateStatus"))
                    {
                        label->setText("SẴN SÀNG");
                        label->setStyleSheet(
                            "font-size:22px;"
                            "font-weight:bold;"
                            "color:#2E7D32;"
                            );
                    }

                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateDot"))
                    {
                        label->setStyleSheet(
                            "font-size:20px;color:#43A047;"
                            );
                    }

                    if (QLabel *label =
                            dashboard->findChild<QLabel *>(
                                "lbGateReady"))
                    {
                        label->setText(
                            "●  Cổng sẵn sàng"
                            );
                        label->setStyleSheet(
                            "font-size:15px;"
                            "color:#2E7D32;"
                            "padding:4px;"
                            );
                    }
                }
            }


            qDebug()
                << "[PARKING] Vehicles:"
                << vehiclesInParking;


            qDebug()
                << "[PARKING] Today:"
                << todayAccessCount;
        }
    );


    // =====================================================
    // CĂN CHỈNH BẢNG NGƯỜI DÙNG
    // =====================================================

    ui->tblUsers->resizeColumnsToContents();


    // Không dùng resizeColumnsToContents()
    // cho 2 bảng nhật ký vì sẽ làm thay đổi
    // kích thước cột đã cấu hình ở trên.


    // =====================================================
    // BẮT ĐẦU QUÉT BLE
    // =====================================================

    bleClient->startScan();
}


// =========================================================
// LOAD USERS
// =========================================================

void MainWindow::loadUsers()
{
    ui->tblUsers->setRowCount(0);

    // Chỉ Admin được xem danh sách người dùng.
    if (currentRole != "admin")
    {
        refreshUserCount();
        return;
    }

    QVariantList users =
        database->getUsers();


    for (const QVariant &value : users)
    {
        QVariantMap user =
            value.toMap();


        int row =
            ui->tblUsers->rowCount();


        ui->tblUsers->insertRow(row);


        QTableWidgetItem *code =
            new QTableWidgetItem(
                user["user_code"].toString()
            );


        code->setData(
            Qt::UserRole,
            user["id"]
        );


        ui->tblUsers->setItem(
            row,
            0,
            code
        );


        ui->tblUsers->setItem(
            row,
            1,
            new QTableWidgetItem(
                user["name"].toString()
            )
        );


        ui->tblUsers->setItem(
            row,
            2,
            new QTableWidgetItem(
                user["department"].toString()
            )
        );


        ui->tblUsers->setItem(
            row,
            3,
            new QTableWidgetItem(
                user["fingerprint_id"].toString()
            )
        );


        ui->tblUsers->setItem(
            row,
            4,
            new QTableWidgetItem(
                user["status"].toString()
            )
        );


        ui->tblUsers->setItem(
            row,
            5,
            new QTableWidgetItem(
                user["note"].toString()
            )
        );
    }


    refreshUserCount();


    ui->tblUsers->resizeColumnsToContents();
}


// =========================================================
// CẬP NHẬT SỐ NGƯỜI DÙNG
// =========================================================

void MainWindow::refreshUserCount()
{
    if (currentRole == "admin")
    {
        ui->lbTotalUsers->setText(
            QString::number(
                database->userCount()
            )
        );
    }
    else
    {
        // Không hiển thị tổng số người dùng của toàn hệ thống.
        ui->lbTotalUsers->setText("1");
    }
}


// =========================================================
// LOAD NHẬT KÝ TỪ SQLITE
// =========================================================

void MainWindow::loadAccessLogs()
{
    QVariantList logs;

    if (currentRole == "admin")
    {
        logs =
            database->getAccessLogs(100);
    }
    else
    {
        QString userCode;

        QVariantList user =
            database->getUserById(currentUserId);

        if (!user.isEmpty())
        {
            userCode =
                user.first().toMap()["user_code"].toString();
        }

        if (!userCode.isEmpty())
        {
            logs =
                database->getAccessLogsForUser(
                    userCode,
                    100
                    );
        }
    }


    ui->tblRecentLog->setRowCount(0);
    ui->tblHistory->setRowCount(0);


    int todayCount = 0;

    QVariantMap latestLog;
    bool hasLatestLog = false;

    QString today =
        QDate::currentDate()
            .toString("dd/MM/yyyy");


    for (const QVariant &value : logs)
    {
        QVariantMap log =
            value.toMap();


        QString time =
            log["time"].toString();

        QString userCode =
            log["user_code"].toString();

        QString name =
            log["name"].toString();

        QString fingerprintId =
            log["fingerprint_id"].toString();

        QString result =
            log["result"].toString();

        QString barrier =
            log["barrier"].toString();

        QString direction =
            log["direction"].toString();

        if (!hasLatestLog)
        {
            latestLog = log;
            hasLatestLog = true;
        }

        // =================================================
        // ĐẾM LƯỢT HÔM NAY
        // =================================================
        // User chỉ tính các lần xác thực thành công.
        // Admin giữ cách đếm nhật ký hiện tại.
        if (time.startsWith(today))
        {
            if (currentRole == "admin" ||
                result == "Xác thực thành công")
            {
                todayCount++;
            }
        }


        // =================================================
        // RECENT LOG
        // =================================================

        addRecentLog(
            time,
            name,
            fingerprintId,
            result,
            barrier
        );


        // =================================================
        // HISTORY
        // =================================================

        addHistoryLog(
            time,
            name,
            userCode,
            fingerprintId,
            result,
            barrier,
            direction
        );
    }


    todayAccessCount =
        todayCount;


    ui->lbToday->setText(
        QString::number(
            todayAccessCount
        )
    );

    // =====================================================
    // CẬP NHẬT DASHBOARD USER
    // =====================================================
    if (currentRole != "admin")
    {
        QWidget *dashboard =
            ui->tabOverview->findChild<QWidget *>("UserDashboard");

        if (dashboard)
        {
            if (QLabel *label =
                    dashboard->findChild<QLabel *>("lbTodayCount"))
            {
                label->setText(
                    QString::number(todayAccessCount)
                    );
            }

            if (hasLatestLog)
            {
                const QString latestName =
                    latestLog["name"].toString();

                const QString latestCode =
                    latestLog["user_code"].toString();

                const QString latestFp =
                    latestLog["fingerprint_id"].toString();

                const QString latestResult =
                    latestLog["result"].toString();

                const QString latestBarrier =
                    latestLog["barrier"].toString();

                const QString latestTime =
                    latestLog["time"].toString();

                if (QLabel *label =
                        dashboard->findChild<QLabel *>("lbLastResult"))
                {
                    if (latestResult == "Xác thực thành công")
                    {
                        label->setText(
                            "✓  Xác thực thành công"
                            );
                        label->setStyleSheet(
                            "font-size:18px;"
                            "font-weight:bold;"
                            "color:#2E7D32;"
                            "padding:6px;"
                            );
                    }
                    else
                    {
                        label->setText(
                            "✕  Xác thực thất bại"
                            );
                        label->setStyleSheet(
                            "font-size:18px;"
                            "font-weight:bold;"
                            "color:#C62828;"
                            "padding:6px;"
                            );
                    }
                }

                if (QLabel *label =
                        dashboard->findChild<QLabel *>("lbLastUser"))
                {
                    label->setText(
                        QString("%1  •  %2  •  Vân tay %3")
                            .arg(
                                latestName.isEmpty()
                                    ? "---"
                                    : latestName
                                )
                            .arg(
                                latestCode.isEmpty()
                                    ? "---"
                                    : latestCode
                                )
                            .arg(
                                latestFp.isEmpty()
                                    ? "---"
                                    : latestFp
                                )
                        );
                }

                if (QLabel *label =
                        dashboard->findChild<QLabel *>("lbLastTime"))
                {
                    label->setText(latestTime);
                }

                if (QLabel *label =
                        dashboard->findChild<QLabel *>("lbLastAccess"))
                {
                    label->setText(latestTime);
                }

                if (QLabel *label =
                        dashboard->findChild<QLabel *>("lbLastBarrier"))
                {
                    label->setText(
                        QString("Barrier: %1")
                            .arg(latestBarrier)
                        );
                }

                if (QLabel *label =
                        dashboard->findChild<QLabel *>("lbLastDirection"))
                {
                    label->setText(
                        QString("Chiều: %1")
                            .arg(latestLog["direction"].toString())
                        );
                }
            }
        }
    }
}


// =========================================================
// THÊM VÀO NHẬT KÝ TỔNG QUAN
// =========================================================

void MainWindow::addRecentLog(
    const QString &time,
    const QString &name,
    const QString &fingerprintId,
    const QString &result,
    const QString &barrier
)
{
    ui->tblRecentLog->insertRow(0);


    ui->tblRecentLog->setItem(
        0,
        0,
        new QTableWidgetItem(time)
    );


    ui->tblRecentLog->setItem(
        0,
        1,
        new QTableWidgetItem(name)
    );


    ui->tblRecentLog->setItem(
        0,
        2,
        new QTableWidgetItem(fingerprintId)
    );


    ui->tblRecentLog->setItem(
        0,
        3,
        new QTableWidgetItem(result)
    );


    ui->tblRecentLog->setItem(
        0,
        4,
        new QTableWidgetItem(barrier)
    );
}


// =========================================================
// THÊM VÀO NHẬT KÝ CHI TIẾT
// =========================================================

void MainWindow::addHistoryLog(
    const QString &time,
    const QString &name,
    const QString &userCode,
    const QString &fingerprintId,
    const QString &result,
    const QString &barrier,
    const QString &direction
)
{
    ui->tblHistory->insertRow(0);

    if (currentRole != "admin")
    {
        // User chỉ xem: Thời gian, Chiều, Kết quả, Barrier.
        ui->tblHistory->setItem(
            0,
            0,
            new QTableWidgetItem(time)
            );

        ui->tblHistory->setItem(
            0,
            1,
            new QTableWidgetItem(direction)
            );

        ui->tblHistory->setItem(
            0,
            2,
            new QTableWidgetItem(result)
            );

        ui->tblHistory->setItem(
            0,
            3,
            new QTableWidgetItem(barrier)
            );

        return;
    }

    // Admin vẫn giữ đầy đủ 7 cột.
    ui->tblHistory->setItem(
        0,
        0,
        new QTableWidgetItem(time)
        );

    ui->tblHistory->setItem(
        0,
        1,
        new QTableWidgetItem(name)
        );

    ui->tblHistory->setItem(
        0,
        2,
        new QTableWidgetItem(userCode)
        );

    ui->tblHistory->setItem(
        0,
        3,
        new QTableWidgetItem(fingerprintId)
        );

    ui->tblHistory->setItem(
        0,
        4,
        new QTableWidgetItem(result)
        );

    ui->tblHistory->setItem(
        0,
        5,
        new QTableWidgetItem(barrier)
        );

    ui->tblHistory->setItem(
        0,
        6,
        new QTableWidgetItem(direction)
        );
}


// =========================================================
// PHÂN QUYỀN
// =========================================================

void MainWindow::applyPermissions()
{
    const bool isAdmin = (currentRole == "admin");

    // =====================================================
    // QUYỀN QUẢN LÝ NGƯỜI DÙNG
    // =====================================================
    ui->btnAddUser->setEnabled(isAdmin);
    ui->btnEditUser->setEnabled(isAdmin);
    ui->btnDeleteUser->setEnabled(isAdmin);
    ui->btnEnroll->setEnabled(isAdmin);

    // =====================================================
    // PHÂN QUYỀN TAB
    // =====================================================
    const int overviewIndex =
        ui->tabWidget->indexOf(ui->tabOverview);

    const int usersIndex =
        ui->tabWidget->indexOf(ui->tabUsers);

    const int historyIndex =
        ui->tabWidget->indexOf(ui->tabHistory);

    const int settingIndex =
        ui->tabWidget->indexOf(ui->tabSettings);

    if (overviewIndex >= 0)
        ui->tabWidget->setTabVisible(
            overviewIndex,
            true
            );

    if (usersIndex >= 0)
        ui->tabWidget->setTabVisible(
            usersIndex,
            isAdmin
            );

    if (historyIndex >= 0)
        ui->tabWidget->setTabVisible(
            historyIndex,
            true
            );

    if (settingIndex >= 0)
        ui->tabWidget->setTabVisible(
            settingIndex,
            isAdmin
            );

    // User chỉ có Tổng quan và Nhật ký.
    if (!isAdmin && overviewIndex >= 0)
    {
        ui->tabWidget->setCurrentIndex(
            overviewIndex
            );
    }
}


// =========================================================
// ĐỔI MẬT KHẨU
// =========================================================

void MainWindow::changePassword()
{
    if (!database->initialize())
    {
        QMessageBox::warning(
            this,
            "Đổi mật khẩu",
            "Không thể truy cập cơ sở dữ liệu."
            );
        return;
    }

    bool ok = false;

    QString oldPassword = QInputDialog::getText(
        this,
        "Đổi mật khẩu",
        "Mật khẩu hiện tại:",
        QLineEdit::Password,
        QString(),
        &ok
        );

    if (!ok)
        return;

    QString newPassword = QInputDialog::getText(
        this,
        "Đổi mật khẩu",
        "Mật khẩu mới:",
        QLineEdit::Password,
        QString(),
        &ok
        );

    if (!ok)
        return;

    if (newPassword.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Đổi mật khẩu",
            "Mật khẩu mới không được để trống."
            );
        return;
    }

    QString confirmPassword = QInputDialog::getText(
        this,
        "Đổi mật khẩu",
        "Nhập lại mật khẩu mới:",
        QLineEdit::Password,
        QString(),
        &ok
        );

    if (!ok)
        return;

    if (newPassword != confirmPassword)
    {
        QMessageBox::warning(
            this,
            "Đổi mật khẩu",
            "Mật khẩu mới nhập lại không khớp."
            );
        return;
    }

    if (database->changePassword(
            currentUsername,
            oldPassword,
            newPassword))
    {
        QMessageBox::information(
            this,
            "Đổi mật khẩu",
            "Đổi mật khẩu thành công."
            );
    }
    else
    {
        QMessageBox::warning(
            this,
            "Đổi mật khẩu",
            "Mật khẩu hiện tại không đúng hoặc tài khoản không còn hoạt động."
            );
    }
}


// =========================================================
// ĐĂNG XUẤT
// =========================================================

void MainWindow::logout()
{
    QMessageBox::StandardButton answer =
        QMessageBox::question(
            this,
            "Đăng xuất",
            "Bạn có chắc muốn đăng xuất không?",
            QMessageBox::Yes | QMessageBox::No
            );

    if (answer != QMessageBox::Yes)
        return;

    Login *login = new Login();
    login->setAttribute(Qt::WA_DeleteOnClose);
    login->show();

    close();
}


// =========================================================
// THOÁT CHƯƠNG TRÌNH
// =========================================================

void MainWindow::exitApplication()
{
    QMessageBox::StandardButton answer =
        QMessageBox::question(
            this,
            "Thoát",
            "Bạn có chắc muốn thoát chương trình không?",
            QMessageBox::Yes | QMessageBox::No
            );

    if (answer == QMessageBox::Yes)
    {
        qApp->quit();
    }
}


// =========================================================
// DESTRUCTOR
// =========================================================

MainWindow::~MainWindow()
{
    delete ui;
}
