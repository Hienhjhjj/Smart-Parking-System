#ifndef CONFIG_H
#define CONFIG_H

// =====================================================
// VL53L0X
// =====================================================

#define I2C_SDA 21
#define I2C_SCL 22

// VL53L0X GPIO1 -> ESP32
#define VL53L0X_INT 27

// XSHUT của VL53L0X
#define VL53L0X_XSHUT 26


// =====================================================
// AS608
// =====================================================

#define AS606_RX 16
#define AS606_TX 17

// AS608 TOUCH -> GPIO34
#define AS606_TCH 34


// =====================================================
// SERVO
// =====================================================

#define SERVO_PIN 25


// =====================================================
// SERIAL
// =====================================================

#define SERIAL_BAUD 115200


// =====================================================
// NGƯỠNG KHOẢNG CÁCH
// =====================================================

// Xe tiến đến barrier
#define VEHICLE_DETECT_MM 300

// Xe đã đi qua barrier
#define VEHICLE_PASSED_MM 450

// Thời gian tối đa chờ đặt vân tay
#define PARKING_FINGERPRINT_TIMEOUT 10000

// Thời gian barrier mở tối đa
#define BARRIER_TIMEOUT 8000

#endif