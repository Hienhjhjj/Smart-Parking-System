#include "distance_sensor.h"
#include "config.h"

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>


// =====================================================
// VL53L0X
// =====================================================

VL53L0X distanceSensor;


// =====================================================
// INTERRUPT FLAG
//
// GPIO1 VL53L0X -> GPIO27 ESP32
// =====================================================

volatile bool distanceInterrupt = false;


// =====================================================
// TRẠNG THÁI
// =====================================================

static bool sensorReady = false;

static bool sensorRunning = false;

static bool measurementActive = false;


// =====================================================
// ISR
//
// ISR CHỈ ĐẶT CỜ.
// KHÔNG ĐỌC I2C TRONG ISR.
// =====================================================

void IRAM_ATTR distanceSensorISR()
{
    distanceInterrupt = true;
}


// =====================================================
// START SINGLE-SHOT
// =====================================================

static void startSingleMeasurement()
{
    // -------------------------------------------------
    // Kiểm tra cảm biến
    // -------------------------------------------------

    if (!sensorReady)
        return;


    // -------------------------------------------------
    // Cảm biến chưa được bật
    // -------------------------------------------------

    if (!sensorRunning)
        return;


    // -------------------------------------------------
    // Đang có phép đo
    // Không tạo phép đo thứ hai
    // -------------------------------------------------

    if (measurementActive)
        return;


    // -------------------------------------------------
    // Xóa cờ interrupt software
    // -------------------------------------------------

    distanceInterrupt = false;


    // -------------------------------------------------
    // Clear interrupt VL53L0X
    // -------------------------------------------------

    distanceSensor.writeReg(
        0x0B,
        0x01
    );


    // -------------------------------------------------
    // START SINGLE RANGE
    // -------------------------------------------------

    distanceSensor.writeReg(
        0x00,
        0x01
    );


    measurementActive = true;
}


// =====================================================
// INIT
// =====================================================

void distanceSensorInit()
{
    // =================================================
    // I2C
    // =================================================

    Wire.begin(
        I2C_SDA,
        I2C_SCL
    );


    distanceSensor.setTimeout(
        100
    );


    // =================================================
    // KẾT NỐI VL53L0X
    // =================================================

    if (
        !distanceSensor.init()
    )
    {
        Serial.println(
            "VL53L0X: Ket noi that bai!"
        );


        sensorReady = false;

        return;
    }


    Serial.println(
        "VL53L0X: Ket noi thanh cong!"
    );


    // =================================================
    // GPIO INTERRUPT
    //
    // GPIO1 VL53L0X
    // -> GPIO27 ESP32
    // =================================================

    pinMode(
        VL53L0X_INT,
        INPUT_PULLUP
    );


    attachInterrupt(
        digitalPinToInterrupt(
            VL53L0X_INT
        ),
        distanceSensorISR,
        FALLING
    );


    // =================================================
    // INTERRUPT MODE
    //
    // New sample ready
    // =================================================

    distanceSensor.writeReg(
        0x0A,
        0x04
    );


    // -------------------------------------------------
    // Active LOW
    // -------------------------------------------------

    distanceSensor.writeReg(
        0x84,
        distanceSensor.readReg(0x84)
        & ~0x10
    );


    // -------------------------------------------------
    // Clear interrupt
    // -------------------------------------------------

    distanceSensor.writeReg(
        0x0B,
        0x01
    );


    // =================================================
    // STATE
    // =================================================

    distanceInterrupt = false;

    measurementActive = false;

    sensorReady = true;

    sensorRunning = false;


    Serial.println(
        "VL53L0X: SINGLE-SHOT INTERRUPT READY"
    );

    Serial.println(
        "VL53L0X: READY"
    );
}


// =====================================================
// UPDATE
//
// Không polling.
// VL53L0X báo kết quả bằng GPIO interrupt.
// =====================================================

void distanceSensorUpdate()
{
    // Không cần xử lý ở đây.
    //
    // ISR đã đặt distanceInterrupt = true.
}


// =====================================================
// START
//
// Mỗi lần gọi = 1 phép đo SINGLE-SHOT.
// =====================================================

void distanceSensorStart()
{
    startSingleMeasurement();
}


// =====================================================
// AVAILABLE
//
// TRUE khi:
// - cảm biến đã sẵn sàng
// - sensor đang chạy
// - đang có phép đo
// - VL53L0X đã phát interrupt
// =====================================================

bool distanceSensorAvailable()
{
    return
        sensorReady &&
        sensorRunning &&
        measurementActive &&
        distanceInterrupt;
}


// =====================================================
// READ
// =====================================================

uint16_t distanceSensorRead()
{
    // -------------------------------------------------
    // Không có kết quả
    // -------------------------------------------------

    if (
        !distanceSensorAvailable()
    )
    {
        return 0;
    }


    // =================================================
    // XÓA CỜ SOFTWARE
    // =================================================

    distanceInterrupt = false;


    // =================================================
    // ĐỌC KẾT QUẢ
    // =================================================

    uint16_t distance =
        distanceSensor.readReg16Bit(
            0x1E
        );


    // =================================================
    // CLEAR INTERRUPT HARDWARE
    // =================================================

    distanceSensor.writeReg(
        0x0B,
        0x01
    );


    // -------------------------------------------------
    // Phép đo đã hoàn thành
    // -------------------------------------------------

    measurementActive = false;


    // =================================================
    // TIMEOUT
    // =================================================

    if (
        distanceSensor.timeoutOccurred()
    )
    {
        Serial.println(
            "VL53L0X: Timeout!"
        );


        return 0;
    }


    // =================================================
    // NGOÀI VÙNG ĐO
    // =================================================

    if (
        distance == 0 ||
        distance >= 8190
    )
    {
        return 0;
    }


    // =================================================
    // DEBUG
    // =================================================

    Serial.print(
        ">>> VL53L0X INTERRUPT -> "
    );

    Serial.print(
        distance
    );

    Serial.println(
        " mm"
    );


    return distance;
}


// =====================================================
// STOP
//
// Dùng khi xe đã đến gần và hệ thống chuyển sang
// chờ vân tay.
// =====================================================

void distanceSensorStop()
{
    if (!sensorReady)
        return;


    // =================================================
    // TẮT TRẠNG THÁI HOẠT ĐỘNG
    // =================================================

    sensorRunning = false;

    measurementActive = false;

    distanceInterrupt = false;


    // =================================================
    // STOP SINGLE MEASUREMENT
    // =================================================

    distanceSensor.writeReg(
        0x00,
        0x00
    );


    // =================================================
    // CLEAR INTERRUPT
    // =================================================

    distanceSensor.writeReg(
        0x0B,
        0x01
    );


    Serial.println(
        "VL53L0X: STOP"
    );
}


// =====================================================
// RESUME
//
// Bật lại VL53L0X.
//
// Nếu sensor đã RUNNING nhưng không có phép đo,
// tạo phép đo mới.
//
// Nếu sensor đang STOPPED,
// bật lại và tạo phép đo đầu tiên.
// =====================================================

void distanceSensorResume()
{
    if (!sensorReady)
        return;


    // =================================================
    // SENSOR ĐANG RUNNING
    // =================================================

    if (
        sensorRunning
    )
    {
        // -------------------------------------------------
        // Không có phép đo đang chạy
        // -> tạo phép đo mới
        // -------------------------------------------------

        if (
            !measurementActive
        )
        {
            startSingleMeasurement();
        }


        return;
    }


    // =================================================
    // BẬT SENSOR
    // =================================================

    sensorRunning = true;

    measurementActive = false;

    distanceInterrupt = false;


    // =================================================
    // CLEAR TRẠNG THÁI CŨ
    // =================================================

    distanceSensor.writeReg(
        0x00,
        0x00
    );


    distanceSensor.writeReg(
        0x0B,
        0x01
    );


    Serial.println(
        "VL53L0X: RESUME"
    );


    // =================================================
    // BẮT ĐẦU PHÉP ĐO ĐẦU TIÊN
    // =================================================

    startSingleMeasurement();
}


// =====================================================
// GIỮ API CŨ
// =====================================================

bool vehicleDetected()
{
    return false;
}


void vehicleDetectedClear()
{
}