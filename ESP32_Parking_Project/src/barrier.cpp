#include "barrier.h"
#include "config.h"

// =====================================================
// PWM SERVO
// =====================================================

#define SERVO_CHANNEL     0
#define SERVO_FREQ        50
#define SERVO_RESOLUTION  16

#define SERVO_MIN_US      500
#define SERVO_MAX_US      2400

// Góc barrier
#define BARRIER_CLOSE_ANGLE 0
#define BARRIER_OPEN_ANGLE  90

static bool barrierOpened = false;

// =====================================================
// ĐỔI GÓC SERVO -> PWM
// =====================================================

static void servoWriteAngle(int angle)
{
    angle = constrain(
        angle,
        0,
        180
    );

    uint32_t pulseWidth =
        map(
            angle,
            0,
            180,
            SERVO_MIN_US,
            SERVO_MAX_US
        );

    uint32_t maxDuty =
        (1UL << SERVO_RESOLUTION) - 1;

    uint32_t duty =
        (pulseWidth * maxDuty) / 20000;

    ledcWrite(
        SERVO_CHANNEL,
        duty
    );
}

// =====================================================
// KHỞI TẠO SERVO
// =====================================================

void barrierInit()
{
    // ESP32 Arduino 2.x
    ledcSetup(
        SERVO_CHANNEL,
        SERVO_FREQ,
        SERVO_RESOLUTION
    );

    ledcAttachPin(
        SERVO_PIN,
        SERVO_CHANNEL
    );

    // Ban đầu đóng barrier
    barrierClose();

    Serial.println(
        "SERVO: Khoi tao thanh cong!"
    );
}

// =====================================================
// MỞ BARRIER
// =====================================================

void barrierOpen()
{
    servoWriteAngle(
        BARRIER_OPEN_ANGLE
    );

    barrierOpened = true;

    Serial.println(
        "BARRIER: OPEN"
    );
}

// =====================================================
// ĐÓNG BARRIER
// =====================================================

void barrierClose()
{
    servoWriteAngle(
        BARRIER_CLOSE_ANGLE
    );

    barrierOpened = false;

    Serial.println(
        "BARRIER: CLOSE"
    );
}

// =====================================================
// KIỂM TRA TRẠNG THÁI
// =====================================================

bool barrierIsOpen()
{
    return barrierOpened;
}