#include "ble_comm.h"
#include "fingerprint.h"

#include <Arduino.h>
#include <NimBLEDevice.h>


// =====================================================
// BLE NAME
// =====================================================

static const char *BLE_NAME =
    "ParkingSystem-ESP32";


// =====================================================
// UUID
// =====================================================

static const char *SERVICE_UUID =
    "12345678-1234-1234-1234-123456789000";

static const char *STATUS_CHAR_UUID =
    "12345678-1234-1234-1234-123456789001";

static const char *COMMAND_CHAR_UUID =
    "12345678-1234-1234-1234-123456789002";


// =====================================================
// OBJECT
// =====================================================

static NimBLEServer *bleServer = nullptr;

static NimBLECharacteristic *statusCharacteristic =
    nullptr;

static NimBLECharacteristic *commandCharacteristic =
    nullptr;

static bool bleConnected = false;


// =====================================================
// ENROLL REQUEST
//
// onWrite() KHÔNG chạy fingerprintEnroll() trực tiếp.
//
// Nó chỉ đặt yêu cầu:
//
// ENROLL,3
//      ↓
// pendingEnroll = true
//
// Sau đó bleUpdate() xử lý.
//
// =====================================================

static volatile bool pendingEnroll =
    false;

static uint8_t pendingEnrollId =
    0;

static bool enrollRunning =
    false;


// =====================================================
// SERVER CALLBACK
// =====================================================

class ParkingServerCallbacks :
    public NimBLEServerCallbacks
{
public:

    void onConnect(
        NimBLEServer *server,
        NimBLEConnInfo &connInfo
    ) override
    {
        bleConnected = true;

        Serial.println();
        Serial.println(
            "================================"
            );

        Serial.println(
            "BLE: RASPBERRY PI DA KET NOI"
            );

        Serial.println(
            "================================"
            );
    }


    void onDisconnect(
        NimBLEServer *server,
        NimBLEConnInfo &connInfo,
        int reason
    ) override
    {
        bleConnected = false;

        Serial.println();
        Serial.println(
            "================================"
            );

        Serial.println(
            "BLE: RASPBERRY PI DA NGAT"
            );

        Serial.print(
            "Disconnect reason: "
            );

        Serial.println(
            reason
            );

        Serial.println(
            "BLE: DANG QUANG BA LAI..."
            );

        Serial.println(
            "================================"
            );

        NimBLEDevice::startAdvertising();
    }
};


// =====================================================
// COMMAND CALLBACK
//
// Qt -> ESP32:
//
// OPEN
// CLOSE
// STATUS
// ENROLL,<ID>
//
// =====================================================

class ParkingCommandCallbacks :
    public NimBLECharacteristicCallbacks
{
public:

    void onWrite(
        NimBLECharacteristic *characteristic,
        NimBLEConnInfo &connInfo
    ) override
    {
        std::string value =
            characteristic->getValue();


        // -------------------------------------------------
        // KHÔNG CÓ DỮ LIỆU
        // -------------------------------------------------

        if (value.empty())
        {
            return;
        }


        // -------------------------------------------------
        // IN LỆNH NHẬN ĐƯỢC
        // -------------------------------------------------

        Serial.print(
            "BLE <- PI: "
            );

        Serial.println(
            value.c_str()
            );


        // -------------------------------------------------
        // CHUYỂN SANG ARDUINO STRING
        // -------------------------------------------------

        String command =
            String(value.c_str());

        command.trim();


        // =================================================
        // OPEN
        // =================================================

        if (command == "OPEN")
        {
            Serial.println(
                "BLE COMMAND: OPEN"
                );

            return;
        }


        // =================================================
        // CLOSE
        // =================================================

        if (command == "CLOSE")
        {
            Serial.println(
                "BLE COMMAND: CLOSE"
                );

            return;
        }


        // =================================================
        // STATUS
        // =================================================

        if (command == "STATUS")
        {
            Serial.println(
                "BLE COMMAND: STATUS"
                );

            bleSend(
                "ESP32_READY"
                );

            return;
        }


        // =================================================
        // ENROLL
        // =================================================

        if (command.startsWith("ENROLL,"))
        {
            Serial.println();
            Serial.println(
                "================================"
                );

            Serial.println(
                "     FINGERPRINT ENROLL REQUEST"
                );

            Serial.println(
                "================================"
                );


            // -------------------------------------------------
            // TÌM DẤU PHẨY
            // -------------------------------------------------

            int commaIndex =
                command.indexOf(',');


            if (commaIndex < 0)
            {
                Serial.println(
                    "ENROLL ERROR: SAI CU PHAP"
                    );

                bleSend(
                    "ENROLL_FAIL,SAI_CU_PHAP"
                    );

                return;
            }


            // -------------------------------------------------
            // LẤY ID
            // -------------------------------------------------

            String idString =
                command.substring(
                    commaIndex + 1
                    );

            idString.trim();


            // -------------------------------------------------
            // KIỂM TRA ID RỖNG
            // -------------------------------------------------

            if (idString.length() == 0)
            {
                Serial.println(
                    "ENROLL ERROR: THIEU ID"
                    );

                bleSend(
                    "ENROLL_FAIL,THIEU_ID"
                    );

                return;
            }


            // -------------------------------------------------
            // KIỂM TRA ID CHỈ GỒM SỐ
            // -------------------------------------------------

            bool validNumber = true;

            for (
                unsigned int i = 0;
                i < idString.length();
                i++
            )
            {
                if (
                    !isDigit(
                        idString.charAt(i)
                        )
                )
                {
                    validNumber = false;
                    break;
                }
            }


            if (!validNumber)
            {
                Serial.println(
                    "ENROLL ERROR: ID KHONG PHAI SO"
                    );

                bleSend(
                    "ENROLL_FAIL,ID_KHONG_HOP_LE"
                    );

                return;
            }


            // -------------------------------------------------
            // CHUYỂN ID
            // -------------------------------------------------

            int id =
                idString.toInt();


            // -------------------------------------------------
            // KIỂM TRA PHẠM VI
            // -------------------------------------------------

            if (
                id < 1 ||
                id > 127
            )
            {
                Serial.println(
                    "ENROLL ERROR: ID KHONG HOP LE"
                    );

                bleSend(
                    "ENROLL_FAIL,ID_KHONG_HOP_LE"
                    );

                return;
            }


            // -------------------------------------------------
            // KHÔNG CHO ĐĂNG KÝ CHỒNG
            // -------------------------------------------------

            if (enrollRunning)
            {
                Serial.println(
                    "ENROLL ERROR: DANG CO PHIEN DANG KY"
                    );

                bleSend(
                    "ENROLL_FAIL,DANG_DANG_KY"
                    );

                return;
            }


            if (pendingEnroll)
            {
                Serial.println(
                    "ENROLL ERROR: DA CO YEU CAU DANG KY"
                    );

                bleSend(
                    "ENROLL_FAIL,DANG_CHO"
                    );

                return;
            }


            // =================================================
            // CHỈ LƯU YÊU CẦU
            //
            // KHÔNG gọi fingerprintEnroll() ở đây.
            // =================================================

            pendingEnrollId =
                (uint8_t)id;

            pendingEnroll =
                true;


            Serial.print(
                "ENROLL REQUEST ID = "
                );

            Serial.println(
                pendingEnrollId
                );

            Serial.println(
                "ENROLL REQUEST DA DUOC NHAN."
                );

            Serial.println(
                "BLE CALLBACK KET THUC."
                );

            return;
        }


        // =================================================
        // UNKNOWN COMMAND
        // =================================================

        Serial.print(
            "BLE COMMAND UNKNOWN: "
            );

        Serial.println(
            command
            );
    }
};


// =====================================================
// BLE INIT
// =====================================================

void bleInit()
{
    Serial.println();
    Serial.println(
        "BLE: KHOI TAO..."
        );


    // =================================================
    // DEVICE
    // =================================================

    NimBLEDevice::init(
        BLE_NAME
        );


    // =================================================
    // SERVER
    // =================================================

    bleServer =
        NimBLEDevice::createServer();

    bleServer->setCallbacks(
        new ParkingServerCallbacks()
        );


    // =================================================
    // SERVICE
    // =================================================

    NimBLEService *service =
        bleServer->createService(
            SERVICE_UUID
            );


    // =================================================
    // STATUS
    //
    // ESP32 -> Qt
    //
    // READ + NOTIFY
    // =================================================

    statusCharacteristic =
        service->createCharacteristic(
            STATUS_CHAR_UUID,
            NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::NOTIFY
            );

    statusCharacteristic->setValue(
        "ESP32_READY"
        );


    // =================================================
    // COMMAND
    //
    // Qt -> ESP32
    //
    // WRITE + WRITE WITHOUT RESPONSE
    // =================================================

    commandCharacteristic =
        service->createCharacteristic(
            COMMAND_CHAR_UUID,
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_NR
            );

    commandCharacteristic->setCallbacks(
        new ParkingCommandCallbacks()
        );


    // =================================================
    // START SERVICE
    // =================================================

    service->start();


    // =================================================
    // ADVERTISING
    // =================================================

    NimBLEAdvertising *advertising =
        NimBLEDevice::getAdvertising();

    advertising->addServiceUUID(
        SERVICE_UUID
        );

    advertising->setName(
        BLE_NAME
        );

    advertising->start();


    // =================================================
    // READY
    // =================================================

    Serial.println();
    Serial.println(
        "================================"
        );

    Serial.println(
        "BLE: READY"
        );

    Serial.print(
        "BLE NAME: "
        );

    Serial.println(
        BLE_NAME
        );

    Serial.print(
        "SERVICE UUID: "
        );

    Serial.println(
        SERVICE_UUID
        );

    Serial.print(
        "STATUS UUID: "
        );

    Serial.println(
        STATUS_CHAR_UUID
        );

    Serial.print(
        "COMMAND UUID: "
        );

    Serial.println(
        COMMAND_CHAR_UUID
        );

    Serial.println(
        "COMMAND MODE: WRITE + WRITE_NR"
        );

    Serial.println(
        "================================"
        );
}


// =====================================================
// BLE UPDATE
//
// Được gọi liên tục từ loop().
//
// Đây mới là nơi thực hiện fingerprintEnroll().
//
// =====================================================

void bleUpdate()
{
    if (!pendingEnroll)
    {
        return;
    }

    if (enrollRunning)
    {
        return;
    }


    // =================================================
    // LẤY ID
    // =================================================

    uint8_t id =
        pendingEnrollId;


    // =================================================
    // XÓA REQUEST TRƯỚC KHI CHẠY
    // =================================================

    pendingEnroll =
        false;

    enrollRunning =
        true;


    // =================================================
    // BẮT ĐẦU
    // =================================================

    Serial.println();
    Serial.println(
        "================================"
        );

    Serial.println(
        "       FINGERPRINT ENROLL"
        );

    Serial.println(
        "================================"
        );

    Serial.print(
        "ENROLL ID = "
        );

    Serial.println(
        id
        );

    Serial.println(
        "AS608: BAT DAU DANG KY..."
        );

    Serial.println(
        "Hay dat ngon tay vao cam bien."
        );


    // =================================================
    // GỌI AS608
    // =================================================

    bool success =
        fingerprintEnroll(
            id
            );


    // =================================================
    // THÀNH CÔNG
    // =================================================

    if (success)
    {
        Serial.println();
        Serial.println(
            "AS608: DANG KY THANH CONG"
            );


        String response =
            "ENROLL_OK," +
            String(id);


        bleSend(
            response
            );


        Serial.println(
            "Da gui ENROLL_OK ve Raspberry Pi."
            );
    }


    // =================================================
    // THẤT BẠI
    // =================================================

    else
    {
        Serial.println();
        Serial.println(
            "AS608: DANG KY THAT BAI"
            );


        bleSend(
            "ENROLL_FAIL,AS608"
            );


        Serial.println(
            "Da gui ENROLL_FAIL ve Raspberry Pi."
            );
    }


    // =================================================
    // KẾT THÚC
    // =================================================

    enrollRunning =
        false;


    Serial.println(
        "================================"
        );
}


// =====================================================
// SEND RAW MESSAGE
//
// ESP32 -> Qt
// =====================================================

void bleSend(
    const String &message
)
{
    if (!bleConnected)
    {
        Serial.println(
            "BLE SEND: CHUA KET NOI"
            );

        return;
    }


    if (
        statusCharacteristic == nullptr
    )
    {
        Serial.println(
            "BLE SEND: STATUS NULL"
            );

        return;
    }


    statusCharacteristic->setValue(
        message.c_str()
        );

    statusCharacteristic->notify();


    Serial.print(
        "BLE -> PI: "
        );

    Serial.println(
        message
        );
}


// =====================================================
// VEHICLE DETECTED
// =====================================================

void bleSendVehicleDetected(
    uint16_t distance
)
{
    String message =
        "VEHICLE," +
        String(distance);

    bleSend(
        message
        );
}


// =====================================================
// FINGERPRINT OK
// =====================================================

void bleSendFingerprintOK(
    int id,
    int confidence
)
{
    String message =
        "FP_OK," +
        String(id) +
        "," +
        String(confidence);

    bleSend(
        message
        );
}


// =====================================================
// FINGERPRINT FAIL
// =====================================================

void bleSendFingerprintFail()
{
    bleSend(
        "FP_FAIL"
        );
}


// =====================================================
// BARRIER OPEN
// =====================================================

void bleSendBarrierOpen()
{
    bleSend(
        "BARRIER,OPEN"
        );
}


// =====================================================
// BARRIER CLOSE
// =====================================================

void bleSendBarrierClose()
{
    bleSend(
        "BARRIER,CLOSE"
        );
}


// =====================================================
// VEHICLE EXIT
// =====================================================

void bleSendVehicleExit()
{
    bleSend(
        "VEHICLE_EXIT"
        );
}


// =====================================================
// CONNECTION
// =====================================================

bool bleIsConnected()
{
    return bleConnected;
}