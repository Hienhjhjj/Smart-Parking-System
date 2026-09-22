#include "bleclient.h"

#include <QDebug>

// ============================================================
// CONSTRUCTOR
// ============================================================

BLEClient::BLEClient(QObject *parent)
    : QObject(parent)
{
    discoveryAgent =
        new QBluetoothDeviceDiscoveryAgent(this);

    controller = nullptr;
    service = nullptr;
    foundDevice = false;

    // Dùng property để chống gửi BLE command chồng nhau
    setProperty("commandWriteInProgress", false);

    // ========================================================
    // DEVICE DISCOVERED
    // ========================================================

    connect(
        discoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
        this,
        &BLEClient::deviceDiscovered
        );

    // ========================================================
    // SCAN FINISHED
    // ========================================================

    connect(
        discoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::finished,
        this,
        &BLEClient::scanFinished
        );

    connect(
        discoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::canceled,
        this,
        &BLEClient::scanFinished
        );
}


// ============================================================
// START SCAN
// ============================================================

void BLEClient::startScan()
{
    foundDevice = false;

    // Cho phép command mới sau khi scan/kết nối lại
    setProperty("commandWriteInProgress", false);

    if (controller)
    {
        controller->disconnectFromDevice();
        controller->deleteLater();
        controller = nullptr;
    }

    if (service)
    {
        service->deleteLater();
        service = nullptr;
    }

    emit logMessage(
        "Dang quet BLE..."
        );

    discoveryAgent->start(
        QBluetoothDeviceDiscoveryAgent::LowEnergyMethod
        );
}


// ============================================================
// DEVICE DISCOVERED
// ============================================================

void BLEClient::deviceDiscovered(
    const QBluetoothDeviceInfo &device)
{
    QString name =
        device.name();

    QString address =
        device.address().toString();

    qDebug()
        << "[BLE DEVICE]"
        << name
        << address;

    emit logMessage(
        "Tim thay: " +
        name +
        " [" +
        address +
        "]"
        );

    // ========================================================
    // KIỂM TRA ĐÚNG ESP32
    // ========================================================

    if (name != targetName)
    {
        return;
    }

    if (foundDevice)
    {
        return;
    }

    foundDevice = true;

    emit logMessage(
        "Da tim thay ESP32: " +
        targetName
        );

    discoveryAgent->stop();

    // ========================================================
    // CREATE CENTRAL
    // ========================================================

    controller =
        QLowEnergyController::createCentral(
            device,
            this
            );

    if (!controller)
    {
        emit logMessage(
            "LOI: Khong tao duoc BLE Controller."
            );

        return;
    }

    // ========================================================
    // CONTROLLER SIGNAL
    // ========================================================

    connect(
        controller,
        &QLowEnergyController::connected,
        this,
        &BLEClient::controllerConnected
        );

    connect(
        controller,
        &QLowEnergyController::disconnected,
        this,
        &BLEClient::controllerDisconnected
        );

    connect(
        controller,
        &QLowEnergyController::serviceDiscovered,
        this,
        &BLEClient::serviceDiscovered
        );

    connect(
        controller,
        &QLowEnergyController::discoveryFinished,
        this,
        &BLEClient::serviceDiscoveryFinished
        );

    connect(
        controller,
        &QLowEnergyController::errorOccurred,
        this,
        &BLEClient::controllerError
        );

    emit logMessage(
        "Dang ket noi ESP32..."
        );

    controller->connectToDevice();
}


// ============================================================
// SCAN FINISHED
// ============================================================

void BLEClient::scanFinished()
{
    emit logMessage(
        "Ket thuc quet BLE."
        );

    if (!foundDevice)
    {
        emit logMessage(
            "Khong tim thay ParkingSystem-ESP32."
            );
    }
}


// ============================================================
// CONTROLLER CONNECTED
// ============================================================

void BLEClient::controllerConnected()
{
    // Kết nối mới -> cho phép command
    setProperty("commandWriteInProgress", false);

    emit logMessage(
        "BLE CONNECTED!"
        );

    emit bleConnected();

    emit logMessage(
        "Dang tim BLE Service..."
        );

    controller->discoverServices();
}


// ============================================================
// SERVICE DISCOVERED
// ============================================================

void BLEClient::serviceDiscovered(
    const QBluetoothUuid &uuid)
{
    emit logMessage(
        "Service: " +
        uuid.toString()
        );

    qDebug()
        << "[BLE SERVICE]"
        << uuid.toString();
}


// ============================================================
// SERVICE DISCOVERY FINISHED
// ============================================================

void BLEClient::serviceDiscoveryFinished()
{
    emit logMessage(
        "Da tim xong BLE Service."
        );

    service =
        controller->createServiceObject(
            serviceUuid,
            this
            );

    if (!service)
    {
        emit logMessage(
            "LOI: Khong tim thay Parking Service!"
            );

        return;
    }

    connect(
        service,
        &QLowEnergyService::stateChanged,
        this,
        &BLEClient::serviceStateChanged
        );

    connect(
        service,
        &QLowEnergyService::characteristicChanged,
        this,
        &BLEClient::characteristicChanged
        );

    // ========================================================
    // QUAN TRỌNG:
    // Khi ESP32 xác nhận write xong,
    // mở khóa để command tiếp theo được gửi.
    // ========================================================

    connect(
        service,
        &QLowEnergyService::characteristicWritten,
        this,
        [this](
            const QLowEnergyCharacteristic &characteristic,
            const QByteArray &value)
        {
            Q_UNUSED(value);

            if (characteristic.uuid() != commandUuid)
            {
                return;
            }

            setProperty(
                "commandWriteInProgress",
                false
                );

            emit logMessage(
                "[BLE] Command write hoan tat."
                );
        }
        );

    service->discoverDetails();
}


// ============================================================
// SERVICE STATE
// ============================================================

void BLEClient::serviceStateChanged(
    QLowEnergyService::ServiceState state)
{
    if (state !=
        QLowEnergyService::RemoteServiceDiscovered)
    {
        return;
    }

    emit logMessage(
        "Parking Service da san sang."
        );

    // ========================================================
    // STATUS CHARACTERISTIC
    // ESP32 -> Qt
    // ========================================================

    statusCharacteristic =
        service->characteristic(
            statusUuid
            );

    if (!statusCharacteristic.isValid())
    {
        emit logMessage(
            "LOI: Khong tim thay STATUS characteristic!"
            );

        return;
    }

    emit logMessage(
        "STATUS characteristic OK."
        );

    // ========================================================
    // COMMAND CHARACTERISTIC
    // Qt -> ESP32
    // ========================================================

    commandCharacteristic =
        service->characteristic(
            commandUuid
            );

    if (!commandCharacteristic.isValid())
    {
        emit logMessage(
            "LOI: Khong tim thay COMMAND characteristic!"
            );

        return;
    }

    emit logMessage(
        "COMMAND characteristic OK."
        );

    // Service sẵn sàng -> cho phép command
    setProperty(
        "commandWriteInProgress",
        false
        );

    // ========================================================
    // ENABLE NOTIFICATION
    // ========================================================

    QLowEnergyDescriptor descriptor =
        statusCharacteristic.descriptor(
            QBluetoothUuid::DescriptorType::
            ClientCharacteristicConfiguration
            );

    if (descriptor.isValid())
    {
        service->writeDescriptor(
            descriptor,
            QByteArray::fromHex("0100")
            );

        emit logMessage(
            "Da bat STATUS notification."
            );
    }
    else
    {
        emit logMessage(
            "CANH BAO: Khong tim thay CCCD."
            );
    }

    // ========================================================
    // READ INITIAL VALUE
    // ========================================================

    if (statusCharacteristic.properties()
        & QLowEnergyCharacteristic::Read)
    {
        service->readCharacteristic(
            statusCharacteristic
            );

        emit logMessage(
            "Dang doc trang thai ban dau..."
            );
    }

    emit logMessage(
        "===== BLE READY ====="
        );
}


// ============================================================
// CHARACTERISTIC CHANGED
// ============================================================
//
// ESP32 -> Qt:
//
// VEHICLE,61
// FP_OK,5,120
// FP_FAIL
// BARRIER,OPEN
// BARRIER,CLOSE
// VEHICLE_EXIT
//
// Đăng ký:
//
// ENROLL_OK,5
// ENROLL_FAIL,<reason>
//
// ============================================================

void BLEClient::characteristicChanged(
    const QLowEnergyCharacteristic &characteristic,
    const QByteArray &value)
{
    // Chỉ xử lý STATUS characteristic
    if (characteristic.uuid() != statusUuid)
    {
        return;
    }

    QString data =
        QString::fromUtf8(value).trimmed();

    if (data.isEmpty())
    {
        return;
    }

    qDebug()
        << "[ESP32 -> Qt]"
        << data;

    emit logMessage(
        "ESP32 -> Qt: " +
        data
        );

    // ========================================================
    // ENROLL STATUS
    // ========================================================
    // ESP32 gửi mã ngắn để tránh vượt giới hạn payload BLE:
    //
    // ENROLL_STATUS,1          -> Đặt ngón tay lần 1...
    // ENROLL_STATUS,2          -> Lần 1 OK - Nhấc ngón tay ra.
    // ENROLL_STATUS,3          -> Đặt lại ngón tay lần 2...
    // ENROLL_STATUS,4          -> Lần 2 OK - Đang tạo mẫu vân tay...
    // ENROLL_STATUS,5          -> Đã tạo mẫu - Đang lưu vân tay...
    // ENROLL_STATUS,6          -> Đã lưu vân tay thành công.
    // ENROLL_STATUS,ERR2       -> Lần 2 không hợp lệ...
    // ENROLL_STATUS,ERR_MODEL  -> Không tạo được mẫu...
    // ENROLL_STATUS,ERR_STORE  -> Lưu vân tay thất bại.
    //
    // Sau khi dịch mã, vẫn phát lại theo dạng:
    // ENROLL_STATUS,<nội dung>
    // để mainwindow.cpp hiện trực tiếp trong dialog.
    // ========================================================

    if (data.startsWith("ENROLL_STATUS,"))
    {
        QString code =
            data.section(',', 1).trimmed();

        QString message;

        if (code == "1")
        {
            message = "Đặt ngón tay lần 1...";
        }
        else if (code == "2")
        {
            message = "Lần 1 OK - Nhấc ngón tay ra.";
        }
        else if (code == "3")
        {
            message = "Đặt lại ngón tay lần 2...";
        }
        else if (code == "4")
        {
            message = "Lần 2 OK - Đang tạo mẫu vân tay...";
        }
        else if (code == "5")
        {
            message = "Đã tạo mẫu - Đang lưu vân tay...";
        }
        else if (code == "6")
        {
            message = "Đã lưu vân tay thành công.";
        }
        else if (code == "ERR2")
        {
            message = "Lần 2 không hợp lệ. Đăng ký thất bại.";
        }
        else if (code == "ERR_MODEL")
        {
            message = "Không tạo được mẫu vân tay.";
        }
        else if (code == "ERR_STORE")
        {
            message = "Lưu vân tay thất bại.";
        }

        if (!message.isEmpty())
        {
            QString translated =
                "ENROLL_STATUS," + message;

            emit logMessage(
                "ESP32 -> Qt: " +
                translated
                );

            emit statusReceived(
                translated
                );
        }
        else
        {
            // Mã không biết: vẫn chuyển tiếp raw để không làm mất status.
            emit statusReceived(
                data
                );
        }

        return;
    }

    // ========================================================
    // STATUS RAW
    // ========================================================

    emit statusReceived(
        data
        );

    // ========================================================
    // ĐĂNG KÝ VÂN TAY THÀNH CÔNG
    // ========================================================

    if (data.startsWith("ENROLL_OK,"))
    {
        // ESP32 đã xử lý xong enrollment
        setProperty(
            "commandWriteInProgress",
            false
            );

        QStringList parts =
            data.split(",");

        if (parts.size() >= 2)
        {
            bool ok = false;

            int id =
                parts.at(1).toInt(&ok);

            if (ok && id > 0)
            {
                emit logMessage(
                    "[AS608] Enroll OK ID = " +
                    QString::number(id)
                    );

                emit fingerprintEnrollSuccess(id);
            }
            else
            {
                emit fingerprintEnrollFailed(
                    "ID vân tay trả về không hợp lệ."
                    );
            }
        }
        else
        {
            emit fingerprintEnrollFailed(
                "ESP32 trả về ENROLL_OK không hợp lệ."
                );
        }

        return;
    }

    // ========================================================
    // ĐĂNG KÝ VÂN TAY THẤT BẠI
    // ========================================================

    if (data.startsWith("ENROLL_FAIL"))
    {
        // ESP32 đã xử lý xong enrollment
        setProperty(
            "commandWriteInProgress",
            false
            );

        QString reason =
            "Đăng ký vân tay thất bại.";

        if (data.contains(","))
        {
            reason =
                data.section(',', 1).trimmed();

            if (reason.isEmpty())
            {
                reason =
                    "Đăng ký vân tay thất bại.";
            }
        }

        emit logMessage(
            "[AS608] " +
            reason
            );

        emit fingerprintEnrollFailed(
            reason
            );

        return;
    }

    // ========================================================
    // XE ĐẾN
    // ========================================================

    if (data.startsWith("VEHICLE,"))
    {
        QStringList parts =
            data.split(",");

        int distance = 0;

        if (parts.size() >= 2)
        {
            bool ok = false;

            distance =
                parts.at(1).toInt(&ok);

            if (!ok)
            {
                distance = 0;
            }
        }

        emit vehicleDetected(
            distance
            );

        return;
    }

    // ========================================================
    // VÂN TAY OK
    // ========================================================

    if (data.startsWith("FP_OK,"))
    {
        QStringList parts =
            data.split(",");

        if (parts.size() >= 2)
        {
            bool okID = false;

            int id =
                parts.at(1).toInt(&okID);

            int confidence = 0;

            if (parts.size() >= 3)
            {
                bool okConfidence = false;

                confidence =
                    parts.at(2).toInt(
                        &okConfidence
                        );

                if (!okConfidence)
                {
                    confidence = 0;
                }
            }

            if (okID)
            {
                emit fingerprintOK(
                    id,
                    confidence
                    );
            }
        }

        return;
    }

    // ========================================================
    // VÂN TAY FAIL
    // ========================================================

    if (data == "FP_FAIL")
    {
        emit fingerprintFail();

        return;
    }

    // ========================================================
    // BARRIER OPEN
    // ========================================================

    if (data == "BARRIER,OPEN")
    {
        emit barrierOpened();

        return;
    }

    // ========================================================
    // BARRIER CLOSE
    // ========================================================

    if (data == "BARRIER,CLOSE")
    {
        emit barrierClosed();

        return;
    }

    // ========================================================
    // XE ĐÃ RA
    // ========================================================

    if (data == "VEHICLE_EXIT")
    {
        emit vehicleExit();

        return;
    }
}


// ============================================================
// SEND COMMAND
// ============================================================

void BLEClient::sendCommand(
    const QString &command)
{
    // ========================================================
    // KIỂM TRA SERVICE
    // ========================================================

    if (!service)
    {
        emit logMessage(
            "BLE Service chua san sang."
            );

        return;
    }

    // ========================================================
    // KIỂM TRA CHARACTERISTIC
    // ========================================================

    if (!commandCharacteristic.isValid())
    {
        emit logMessage(
            "COMMAND characteristic khong hop le."
            );

        return;
    }

    // ========================================================
    // CHỐNG GỬI COMMAND CHỒNG NHAU
    // ========================================================

    bool commandBusy =
        property(
            "commandWriteInProgress"
            ).toBool();

    if (commandBusy)
    {
        emit logMessage(
            "[BLE] Dang co command dang xu ly, bo qua lenh moi: " +
            command
            );

        return;
    }

    // ========================================================
    // ĐÁNH DẤU ĐANG WRITE
    // ========================================================

    setProperty(
        "commandWriteInProgress",
        true
        );

    QByteArray data =
        command.toUtf8();

    // ========================================================
    // WRITE COMMAND
    // ========================================================

    service->writeCharacteristic(
        commandCharacteristic,
        data,
        QLowEnergyService::WriteWithResponse
        );

    emit logMessage(
        "Qt -> ESP32: " +
        command
        );
}


// ============================================================
// ĐĂNG KÝ VÂN TAY
// ============================================================

void BLEClient::startFingerprintEnroll(int id)
{
    // ========================================================
    // KIỂM TRA ID
    // ========================================================

    if (id < 1 || id > 127)
    {
        emit fingerprintEnrollFailed(
            "ID vân tay không hợp lệ."
            );

        return;
    }


    // ========================================================
    // KIỂM TRA SERVICE
    // ========================================================

    if (!service)
    {
        emit fingerprintEnrollFailed(
            "BLE Service chưa sẵn sàng."
            );

        return;
    }


    // ========================================================
    // LẤY LẠI COMMAND CHARACTERISTIC
    // ========================================================
    //
    // Tránh trường hợp QLowEnergyCharacteristic cũ
    // bị invalid sau quá trình BLE thay đổi trạng thái.
    //
    // ========================================================

    if (!commandCharacteristic.isValid())
    {
        emit logMessage(
            "[BLE] COMMAND characteristic dang invalid, dang tim lai..."
            );

        commandCharacteristic =
            service->characteristic(commandUuid);
    }


    // ========================================================
    // KIỂM TRA LẠI
    // ========================================================

    if (!commandCharacteristic.isValid())
    {
        emit fingerprintEnrollFailed(
            "Không lấy lại được COMMAND characteristic."
            );

        emit logMessage(
            "[BLE] LOI: COMMAND characteristic van invalid."
            );

        return;
    }


    // ========================================================
    // KIỂM TRA QUYỀN WRITE
    // ========================================================

    if (!commandCharacteristic.properties().testFlag(
            QLowEnergyCharacteristic::WriteNoResponse
            ))
    {
        emit fingerprintEnrollFailed(
            "COMMAND characteristic không có quyền WRITE."
            );

        emit logMessage(
            "[BLE] LOI: COMMAND characteristic khong co WRITE."
            );

        return;
    }


    // ========================================================
    // KIỂM TRA COMMAND ĐANG BẬN
    // ========================================================

    bool commandBusy =
        property(
            "commandWriteInProgress"
            ).toBool();

    if (commandBusy)
    {
        emit logMessage(
            "[BLE] Dang co command dang xu ly, khong gui ENROLL lan 2."
            );

        return;
    }


    // ========================================================
    // TẠO LỆNH
    // ========================================================

    QString command =
        QString("ENROLL,%1").arg(id);


    emit logMessage(
        "[BLE] Gui lenh: " +
        command
        );


    // ========================================================
    // GỬI
    // ========================================================

    sendCommand(command);
}



// ============================================================
// DISCONNECTED
// ============================================================

void BLEClient::controllerDisconnected()
{
    // Mất kết nối -> mở khóa command
    setProperty(
        "commandWriteInProgress",
        false
        );

    emit logMessage(
        "BLE DISCONNECTED."
        );

    emit bleDisconnected();
}


// ============================================================
// ERROR
// ============================================================

void BLEClient::controllerError(
    QLowEnergyController::Error error)
{
    // Có lỗi BLE -> không giữ trạng thái busy
    setProperty(
        "commandWriteInProgress",
        false
        );

    emit logMessage(
        "BLE ERROR = " +
        QString::number(
            static_cast<int>(error)
            )
        );
}
