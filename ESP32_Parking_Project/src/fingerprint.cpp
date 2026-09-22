#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

#include "fingerprint.h"
#include "config.h"
#include "ble_comm.h"


// =====================================================
// AS608
// =====================================================

HardwareSerial FingerSerial(2);

Adafruit_Fingerprint finger(
    &FingerSerial
);


// =====================================================
// KẾT QUẢ
// =====================================================

static bool fingerprintResult = false;

static int fingerprintID = -1;


// =====================================================
// TCH
// =====================================================

volatile bool fingerprintTouchFlag = false;


// Một lần chạm chỉ tạo một event
static bool touchLocked = false;

static unsigned long lastTouchEvent = 0;


// =====================================================
// ISR
// =====================================================

void IRAM_ATTR fingerprintTouchISR()
{
    if (!touchLocked)
    {
        fingerprintTouchFlag = true;
    }
}


// =====================================================
// INIT
// =====================================================

void fingerprintInit()
{
    FingerSerial.begin(
        57600,
        SERIAL_8N1,
        AS606_RX,
        AS606_TX
    );


    finger.begin(
        57600
    );


    // TCH
    pinMode(
        AS606_TCH,
        INPUT
    );


    attachInterrupt(
        digitalPinToInterrupt(
            AS606_TCH
        ),
        fingerprintTouchISR,
        RISING
    );


    delay(100);


    // Kiểm tra AS608
    if (
        finger.verifyPassword()
    )
    {
        Serial.println(
            "AS608: Ket noi thanh cong!"
        );
    }
    else
    {
        Serial.println(
            "AS608: KHONG KET NOI DUOC!"
        );
    }


    fingerprintTouchFlag = false;

    touchLocked = false;

    fingerprintResult = false;

    fingerprintID = -1;

    lastTouchEvent = 0;
}


// =====================================================
// TCH AVAILABLE
// =====================================================

bool fingerprintTouchAvailable()
{
    // -------------------------------------------------
    // ĐANG KHÓA
    // -------------------------------------------------

    if (touchLocked)
    {
        // Chỉ mở khóa khi:
        // - TCH đã LOW
        // - Đã qua thời gian debounce

        if (
            digitalRead(
                AS606_TCH
            ) == LOW
        )
        {
            if (
                millis() -
                lastTouchEvent >=
                500
            )
            {
                touchLocked = false;
            }
        }


        return false;
    }


    // -------------------------------------------------
    // CÓ INTERRUPT
    // -------------------------------------------------

    if (
        fingerprintTouchFlag
    )
    {
        fingerprintTouchFlag =
            false;

        touchLocked =
            true;

        lastTouchEvent =
            millis();

        return true;
    }


    return false;
}


// =====================================================
// CLEAR TCH
// =====================================================

void fingerprintTouchClear()
{
    fingerprintTouchFlag =
        false;

    // Không unlock ngay.
    // Đợi nhả tay + debounce.
}


// =====================================================
// UPDATE
// =====================================================

void fingerprintUpdate()
{
    fingerprintResult = false;

    fingerprintID = -1;


    Serial.println();

    Serial.println(
        ">>> AS608: BAT DAU DOC VAN TAY"
    );


    // TCH có thể lên trước khi ảnh sẵn sàng.
    delay(120);


    // =================================================
    // GET IMAGE
    // =================================================

    uint8_t p =
        finger.getImage();


    if (
        p != FINGERPRINT_OK
    )
    {
        Serial.print(
            "AS608 getImage loi = "
        );

        Serial.println(
            p
        );


        // p = 2 thường là NOFINGER.
        //
        // Không coi là vân tay sai.
        // Chờ lần TCH tiếp theo.

        return;
    }


    Serial.println(
        ">>> AS608: DA LAY ANH"
    );


    // =================================================
    // IMAGE -> TEMPLATE
    // =================================================

    p =
        finger.image2Tz();


    if (
        p != FINGERPRINT_OK
    )
    {
        Serial.print(
            "AS608 image2Tz loi = "
        );

        Serial.println(
            p
        );

        return;
    }


    // =================================================
    // SEARCH
    // =================================================

    p =
        finger.fingerSearch();


    // =================================================
    // TÌM THẤY
    // =================================================

    if (
        p == FINGERPRINT_OK
    )
    {
        fingerprintID =
            finger.fingerID;

        fingerprintResult =
            true;


        Serial.println(
            ">>> VAN TAY HOP LE"
        );


        Serial.print(
            ">>> ID = "
        );

        Serial.println(
            fingerprintID
        );


        Serial.print(
            ">>> Confidence = "
        );

        Serial.println(
            finger.confidence
        );


        return;
    }


    // =================================================
    // KHÔNG TÌM THẤY
    // =================================================

    if (
        p == FINGERPRINT_NOTFOUND
    )
    {
        fingerprintID =
            -1;

        fingerprintResult =
            true;


        Serial.println(
            ">>> VAN TAY KHONG HOP LE"
        );


        return;
    }


    // Lỗi khác
    Serial.print(
        "AS608 fingerSearch loi = "
    );

    Serial.println(
        p
    );
}


// =====================================================
// HAS RESULT
// =====================================================

bool fingerprintHasResult()
{
    return fingerprintResult;
}


// =====================================================
// GET ID
// =====================================================

int fingerprintGetID()
{
    return fingerprintID;
}


// =====================================================
// CLEAR RESULT
// =====================================================

void fingerprintClearResult()
{
    fingerprintResult =
        false;

    fingerprintID =
        -1;
}


// =====================================================
// ENROLL
// =====================================================

bool fingerprintEnroll(
    uint8_t id
)
{
    uint8_t p;


    Serial.print(
        "Dang ky van tay ID = "
    );

    Serial.println(
        id
    );


    // =================================================
    // LẦN 1
    // =================================================

    Serial.println(
        "Dat ngon tay lan 1..."
    );

    // Gửi mã trạng thái ngắn để tránh vượt giới hạn payload BLE.
    bleSend(
        "ENROLL_STATUS,1"
    );


    while (true)
    {
        p =
            finger.getImage();


        if (
            p == FINGERPRINT_OK
        )
        {
            break;
        }


        if (
            p != FINGERPRINT_NOFINGER
        )
        {
            return false;
        }


        delay(50);
    }


    if (
        finger.image2Tz(1)
        != FINGERPRINT_OK
    )
    {
        return false;
    }


    Serial.println(
        "Anh lan 1 OK"
    );

    bleSend(
        "ENROLL_STATUS,2"
    );


    // =================================================
    // NHẤC TAY
    // =================================================

    Serial.println(
        "Nhac ngon tay ra..."
    );


    while (
        finger.getImage()
        != FINGERPRINT_NOFINGER
    )
    {
        delay(50);
    }


    delay(300);


    // =================================================
    // LẦN 2
    // =================================================

    Serial.println(
        "Dat lai ngon tay lan 2..."
    );

    bleSend(
        "ENROLL_STATUS,3"
    );


    while (true)
    {
        p =
            finger.getImage();


        if (
            p == FINGERPRINT_OK
        )
        {
            break;
        }


        if (
            p != FINGERPRINT_NOFINGER
        )
        {
            return false;
        }


        delay(50);
    }


    if (
        finger.image2Tz(2)
        != FINGERPRINT_OK
    )
    {
        bleSend(
            "ENROLL_STATUS,ERR2"
        );

        return false;
    }

    bleSend(
        "ENROLL_STATUS,4"
    );


    // =================================================
    // CREATE MODEL
    // =================================================

    if (
        finger.createModel()
        != FINGERPRINT_OK
    )
    {
        bleSend(
            "ENROLL_STATUS,ERR_MODEL"
        );

        return false;
    }

    bleSend(
        "ENROLL_STATUS,5"
    );


    // =================================================
    // STORE
    // =================================================

    if (
        finger.storeModel(id)
        != FINGERPRINT_OK
    )
    {
        bleSend(
            "ENROLL_STATUS,ERR_STORE"
        );

        return false;
    }

    bleSend(
        "ENROLL_STATUS,6"
    );


    Serial.print(
        "DA DANG KY ID = "
    );

    Serial.println(
        id
    );


    return true;
}


// =====================================================
// SEARCH TEST
// =====================================================

bool fingerprintSearch()
{
    uint8_t p;


    while (true)
    {
        p =
            finger.getImage();


        if (
            p == FINGERPRINT_OK
        )
        {
            break;
        }


        if (
            p != FINGERPRINT_NOFINGER
        )
        {
            return false;
        }


        delay(50);
    }


    if (
        finger.image2Tz()
        != FINGERPRINT_OK
    )
    {
        return false;
    }


    p =
        finger.fingerSearch();


    if (
        p == FINGERPRINT_OK
    )
    {
        Serial.print(
            "Tim thay ID = "
        );

        Serial.println(
            finger.fingerID
        );


        Serial.print(
            "Confidence = "
        );

        Serial.println(
            finger.confidence
        );


        return true;
    }


    return false;
}


// =====================================================
// DELETE
// =====================================================

bool fingerprintDelete(
    uint8_t id
)
{
    return
        finger.deleteModel(id)
        == FINGERPRINT_OK;
}


// =====================================================
// DELETE ALL
// =====================================================

bool fingerprintEmptyDatabase()
{
    return
        finger.emptyDatabase()
        == FINGERPRINT_OK;
}


// =====================================================
// COUNT
// =====================================================

uint16_t fingerprintShowCount()
{
    if (
        finger.getTemplateCount()
        == FINGERPRINT_OK
    )
    {
        return finger.templateCount;
    }


    return 0;
}