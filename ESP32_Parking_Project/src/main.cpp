#include <Arduino.h>

#include "config.h"
#include "distance_sensor.h"
#include "fingerprint.h"
#include "barrier.h"
#include "ble_comm.h"


// =====================================================
// CẤU HÌNH NGƯỠNG & THỜI GIAN
// =====================================================

#define VEHICLE_DETECT_DISTANCE       300   // mm: Xe đến gần barrier
#define VEHICLE_LEAVE_DISTANCE        450   // mm: Xe đã ra xa vùng kiểm soát
#define VEHICLE_LEAVE_CONFIRM_COUNT     5   // Số mẫu liên tiếp xác nhận xe rời
#define FINGERPRINT_WAIT_TIME        15000  // ms: Chờ vân tay tối đa 15s

// Khoảng thời gian bảo vệ sau khi barrier mở.
// Trong thời gian này KHÔNG được phép đóng barrier chỉ vì cảm biến
// trả về khoảng cách lớn.
#define BARRIER_GUARD_TIME            2000  // ms: Không đóng ngay sau khi mở
#define BARRIER_CLOSE_SETTLE_TIME      1000  // ms: Chờ barrier đóng hoàn toàn

// Thời gian barrier được phép mở tối đa.
// Hết thời gian mà xe chưa rời -> tiếp tục giữ mở và chờ xe rời.
#define BARRIER_OPEN_TIME             8000  // ms


// =====================================================
// STATE MACHINE
// =====================================================

enum SystemState
{
    WAIT_VEHICLE,        // Chờ xe đến
    WAIT_FINGERPRINT,    // Chờ quét vân tay
    BARRIER_OPEN,        // Barrier đang mở
    BARRIER_CLOSING,     // Barrier đang đóng, chưa nhận xe mới
    WAIT_VEHICLE_EXIT    // Chờ xe rời vùng cảm biến
};


static SystemState systemState = WAIT_VEHICLE;

static unsigned long stateStartTime = 0;

// Thời điểm barrier thực sự được mở
static unsigned long barrierOpenTime = 0;

// Thời điểm bắt đầu ra lệnh đóng barrier
static unsigned long barrierClosingTime = 0;

// Số mẫu liên tiếp xác nhận xe đã rời
static uint8_t vehicleLeaveCount = 0;

// Đã xác nhận có xe đến barrier trước khi xác thực vân tay
static bool vehicleDetectedForCycle = false;


// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(SERIAL_BAUD);

    delay(500);


    Serial.println(F("\n================================"));
    Serial.println(F("   PARKING SYSTEM ESP32 READY   "));
    Serial.println(F("================================"));


    // -------------------------------------------------
    // KHỞI TẠO CÁC THIẾT BỊ
    // -------------------------------------------------

    distanceSensorInit();

    fingerprintInit();

    barrierInit();

    barrierClose();

    bleInit();


    // -------------------------------------------------
    // BẮT ĐẦU CHU TRÌNH
    // -------------------------------------------------

    vehicleLeaveCount = 0;

    systemState = WAIT_VEHICLE;

    distanceSensorResume();


    Serial.println(F("Waiting for vehicle..."));
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    // =================================================
    // XỬ LÝ YÊU CẦU BLE
    // =================================================
    // ENROLL được nhận trong onWrite() và chỉ đặt cờ.
    // bleUpdate() thực hiện fingerprintEnroll() ngoài BLE callback.
    bleUpdate();

    // Cập nhật trạng thái cảm biến
    distanceSensorUpdate();


    switch (systemState)
    {

        // =================================================
        // TRẠNG THÁI 1
        // CHỜ XE ĐẾN
        // =================================================

        case WAIT_VEHICLE:
        {
            if (!distanceSensorAvailable())
                break;


            uint16_t distance = distanceSensorRead();


            // -------------------------------------------------
            // Không có dữ liệu hợp lệ
            // -------------------------------------------------

            if (distance == 0)
            {
                distanceSensorStart();
                break;
            }


            Serial.print(F("Khoang cach = "));
            Serial.print(distance);
            Serial.println(F(" mm"));


            // -------------------------------------------------
            // PHÁT HIỆN XE ĐẾN
            // -------------------------------------------------

            if (distance <= VEHICLE_DETECT_DISTANCE)
            {
                Serial.println(
                    F("\n>>> XE DA DEN GAN -> MOI DAT NGON TAY")
                );


                bleSendVehicleDetected(distance);


                // -------------------------------------------------
                // DỪNG VL53L0X
                // -------------------------------------------------

                distanceSensorStop();


                // -------------------------------------------------
                // RESET TRẠNG THÁI VÂN TAY
                // -------------------------------------------------

                fingerprintClearResult();

                fingerprintTouchClear();


                stateStartTime = millis();

                vehicleLeaveCount = 0;
                vehicleDetectedForCycle = false;

                systemState = WAIT_FINGERPRINT;
            }
            else
            {
                // -------------------------------------------------
                // Chưa có xe
                // Single-shot -> tạo phép đo tiếp theo
                // -------------------------------------------------

                distanceSensorStart();
            }

            break;
        }


        // =================================================
        // TRẠNG THÁI 2
        // CHỜ QUÉT VÂN TAY
        // =================================================

        case WAIT_FINGERPRINT:
        {

            // -------------------------------------------------
            // TIMEOUT 15 GIÂY
            // -------------------------------------------------

            if (
                millis() - stateStartTime
                >= FINGERPRINT_WAIT_TIME
            )
            {
                Serial.println(
                    F("\n>>> TIMEOUT: KHONG CO VAN TAY")
                );


                bleSend(
                    F("FP_TIMEOUT")
                );


                fingerprintClearResult();

                fingerprintTouchClear();

                vehicleLeaveCount = 0;


                // -------------------------------------------------
                // BẬT LẠI VL53L0X
                // -------------------------------------------------

                distanceSensorResume();

                systemState = WAIT_VEHICLE_EXIT;


                break;
            }


            // -------------------------------------------------
            // KIỂM TRA TOUCH
            // -------------------------------------------------

            if (fingerprintTouchAvailable())
            {
                fingerprintUpdate();


                // Chưa có kết quả
                if (!fingerprintHasResult())
                    break;


                int id = fingerprintGetID();


                // =================================================
                // VÂN TAY HỢP LỆ
                // =================================================

                if (id >= 0)
                {
                    Serial.print(
                        F(">>> XAC THUC THANH CONG - ID: ")
                    );

                    Serial.println(id);


                    char buf[16];

                    snprintf(
                        buf,
                        sizeof(buf),
                        "FP_OK,%d",
                        id
                    );


                    bleSend(buf);


                    // -------------------------------------------------
                    // MỞ BARRIER
                    // -------------------------------------------------

                    barrierOpen();

                    bleSendBarrierOpen();

                    stateStartTime = millis();
                    barrierOpenTime = millis();

                    vehicleLeaveCount = 0;

                    // Bật lại VL53L0X ngay sau khi barrier mở.
                    distanceSensorResume();

                    systemState = BARRIER_OPEN;
                }


                // =================================================
                // VÂN TAY KHÔNG HỢP LỆ
                // =================================================

                else
                {
                    Serial.println(
                        F(">>> TU CHOI TRUY CAP")
                    );


                    bleSendFingerprintFail();


                    vehicleLeaveCount = 0;


                    // -------------------------------------------------
                    // BẬT LẠI CẢM BIẾN
                    // -------------------------------------------------

                    distanceSensorResume();

                    systemState = WAIT_VEHICLE_EXIT;
                }


                fingerprintClearResult();
            }


            break;
        }


        // =================================================
        // TRẠNG THÁI 3
        // BARRIER ĐANG MỞ
        // =================================================

        case BARRIER_OPEN:
        {
            unsigned long now = millis();

            // =================================================
            // 1. THỜI GIAN BẢO VỆ SAU KHI MỞ
            // =================================================
            // Trong 2 giây đầu tuyệt đối không đóng barrier
            // dựa trên khoảng cách.
            if (now - barrierOpenTime < BARRIER_GUARD_TIME)
            {
                break;
            }

            // =================================================
            // 2. KIỂM TRA CẢM BIẾN
            // =================================================
            if (!distanceSensorAvailable())
                break;

            uint16_t distance = distanceSensorRead();

            // 0 = dữ liệu không hợp lệ -> không đếm xe rời
            if (distance == 0)
            {
                distanceSensorStart();
                break;
            }

            Serial.print(F("Khoang cach sau khi mo barrier = "));
            Serial.print(distance);
            Serial.println(F(" mm"));

            // =================================================
            // 3. XE ĐÃ RỜI KHỎI VÙNG KIỂM SOÁT?
            // =================================================
            if (
                vehicleDetectedForCycle &&
                distance > VEHICLE_LEAVE_DISTANCE
            )
            {
                vehicleLeaveCount++;

                Serial.print(F(">>> XAC NHAN XE ROI: "));
                Serial.print(vehicleLeaveCount);
                Serial.print(F("/"));
                Serial.println(VEHICLE_LEAVE_CONFIRM_COUNT);

                // Phải có đủ 5 mẫu liên tiếp.
                if (
                    vehicleLeaveCount
                    >= VEHICLE_LEAVE_CONFIRM_COUNT
                )
                {
                    Serial.println(
                        F("\n>>> XE DA RA -> DONG BARRIER")
                    );

                    // Ra lệnh đóng ngay.
                    barrierClose();
                    bleSendBarrierClose();
                    bleSendVehicleExit();

                    // QUAN TRỌNG:
                    // Chưa quay về WAIT_VEHICLE ngay.
                    // Phải chờ barrier thực sự đóng xong.
                    barrierClosingTime = now;
                    vehicleLeaveCount = 0;
                    vehicleDetectedForCycle = false;

                    systemState = BARRIER_CLOSING;

                    break;
                }
            }
            else
            {
                // Xe vẫn còn trong vùng -> reset bộ đếm.
                vehicleLeaveCount = 0;

                Serial.println(
                    F(">>> XE VAN TRONG VUNG -> GIU BARRIER MO")
                );
            }

            distanceSensorStart();

            // =================================================
            // 4. TIMEOUT
            // =================================================
            if (now - barrierOpenTime >= BARRIER_OPEN_TIME)
            {
                Serial.println(
                    F("\n>>> BARRIER TIMEOUT")
                );

                Serial.println(
                    F(">>> CHUA XAC NHAN XE DA RA")
                );

                Serial.println(
                    F(">>> GIU BARRIER MO -> TIEP TUC CHO XE RA")
                );

                vehicleLeaveCount = 0;

                // Không đóng ở timeout.
                // Chỉ chuyển sang trạng thái chờ xe rời.
                systemState = WAIT_VEHICLE_EXIT;
            }

            break;
        }


        // =================================================
        // TRẠNG THÁI: BARRIER ĐANG ĐÓNG
        // =================================================

        case BARRIER_CLOSING:
        {
            // Trong lúc servo đang về vị trí đóng:
            // KHÔNG đọc xe mới.
            // KHÔNG chuyển sang WAIT_VEHICLE.
            if (
                millis() - barrierClosingTime
                < BARRIER_CLOSE_SETTLE_TIME
            )
            {
                break;
            }

            // Barrier đã có thời gian đóng hoàn toàn.
            distanceSensorResume();

            systemState = WAIT_VEHICLE;

            Serial.println(
                F(">>> BARRIER DA DONG -> SAN SANG NHAN XE MOI")
            );

            break;
        }


        // =================================================
        // TRẠNG THÁI 4
        // CHỜ XE RỜI
        // =================================================

        case WAIT_VEHICLE_EXIT:
        {
            if (!distanceSensorAvailable())
                break;

            uint16_t distance = distanceSensorRead();

            // Không có dữ liệu hợp lệ:
            // không được coi là xe đã rời.
            if (distance == 0)
            {
                distanceSensorStart();
                break;
            }

            Serial.print(F("Khoang cach = "));
            Serial.print(distance);
            Serial.println(F(" mm"));

            if (distance > VEHICLE_LEAVE_DISTANCE)
            {
                vehicleLeaveCount++;

                Serial.print(F(">>> XAC NHAN XE ROI: "));
                Serial.print(vehicleLeaveCount);
                Serial.print(F("/"));
                Serial.println(VEHICLE_LEAVE_CONFIRM_COUNT);

                if (
                    vehicleLeaveCount
                    >= VEHICLE_LEAVE_CONFIRM_COUNT
                )
                {
                    Serial.println(
                        F("\n>>> XE DA RA HOAN TOAN -> DONG BARRIER")
                    );

                    // Timeout trước đó KHÔNG được phép đóng barrier.
                    // Chỉ khi lúc này xác nhận xe đã ra mới đóng.
                    barrierClose();
                    bleSendBarrierClose();
                    bleSendVehicleExit();

                    barrierClosingTime = millis();

                    vehicleLeaveCount = 0;
                    vehicleDetectedForCycle = false;

                    // Không nhận xe mới cho tới khi barrier
                    // có thời gian đóng hoàn toàn.
                    systemState = BARRIER_CLOSING;

                    break;
                }
            }
            else
            {
                vehicleLeaveCount = 0;

                Serial.println(
                    F(">>> XE VAN CON TRONG VUNG -> RESET BO DEM")
                );
            }

            distanceSensorStart();

            break;
        }
        }
    }