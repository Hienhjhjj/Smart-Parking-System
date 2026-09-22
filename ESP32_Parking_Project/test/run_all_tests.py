import sys
import time

# ================================================================
# UTF-8 cho Windows
# ================================================================

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding="utf-8")


# ================================================================
# MÀU HIỂN THỊ
# ================================================================

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    CYAN = '\033[96m'
    YELLOW = '\033[93m'
    BOLD = '\033[1m'
    RESET = '\033[0m'


# ================================================================
# BIẾN THỐNG KÊ
# ================================================================

total_tests = 0
passed_tests = 0
failed_tests = 0


# ================================================================
# HÀM ASSERT
# ================================================================

def assert_true(condition, msg="Assertion failed"):
    if not condition:
        raise AssertionError(msg)


def assert_false(condition, msg="Assertion failed"):
    if condition:
        raise AssertionError(msg)


def assert_equal(expected, actual, msg="Values not equal"):
    if expected != actual:
        raise AssertionError(
            f"{msg} (Expected: {expected}, Actual: {actual})"
        )


# ================================================================
# HÀM CHẠY TEST
# ================================================================

def run_test(stt, func_name, scenario, test_func):
    global total_tests, passed_tests, failed_tests

    total_tests += 1

    display_title = f"TC{stt:02d}: {func_name} - {scenario}"
    dots = '.' * max(1, 72 - len(display_title))

    try:
        test_func()

        passed_tests += 1

        print(
            f"  {Colors.BOLD}{display_title}{Colors.RESET}"
            f" {dots} "
            f"{Colors.GREEN}[PASSED]{Colors.RESET}"
        )

    except AssertionError as e:

        failed_tests += 1

        print(
            f"  {Colors.BOLD}{display_title}{Colors.RESET}"
            f" {dots} "
            f"{Colors.RED}[FAILED: {e}]{Colors.RESET}"
        )

    except Exception as e:

        failed_tests += 1

        print(
            f"  {Colors.BOLD}{display_title}{Colors.RESET}"
            f" {dots} "
            f"{Colors.RED}[ERROR: {e}]{Colors.RESET}"
        )


# ================================================================
# RESET BỘ ĐẾM
# ================================================================

def reset_test_counter():

    global total_tests
    global passed_tests
    global failed_tests

    total_tests = 0
    passed_tests = 0
    failed_tests = 0


# ================================================================
# IN KẾT QUẢ
# ================================================================

def print_summary():

    print(
        f"\n{Colors.BOLD}{Colors.CYAN}"
        "======================================================================"
        f"{Colors.RESET}"
    )

    print(
        f"{Colors.BOLD}TỔNG KẾT KẾT QUẢ KIỂM THỬ:{Colors.RESET}"
    )

    print(
        f"  - Tổng số test: "
        f"{Colors.BOLD}{total_tests}{Colors.RESET}"
    )

    print(
        f"  - Thành công (PASSED): "
        f"{Colors.GREEN}{Colors.BOLD}{passed_tests}{Colors.RESET}"
    )

    print(
        f"  - Thất bại (FAILED): "
        f"{Colors.RED}{Colors.BOLD}{failed_tests}{Colors.RESET}"
    )

    if failed_tests == 0 and total_tests > 0:

        print(
            f"  - Đánh giá: "
            f"{Colors.GREEN}{Colors.BOLD}"
            f"100% TEST PASSED"
            f"{Colors.RESET}"
        )

    print(
        f"{Colors.BOLD}{Colors.CYAN}"
        "======================================================================"
        f"{Colors.RESET}\n"
    )


# =====================================================================
# MODULE 1: VL53L0X DISTANCE SENSOR LOGIC
# =====================================================================

def vl53_check_distance(dist):

    if dist == 0 or dist >= 8190:
        return False

    return True


def vl53_detect_vehicle(dist, threshold=300):

    if not vl53_check_distance(dist):
        return False

    return dist <= threshold


def distance_sensor_read(raw_reg_val, available=True):

    if not available:
        return 0

    if raw_reg_val == 0 or raw_reg_val >= 8190:
        return 0

    return raw_reg_val


class VL53LeaveFilter:

    def __init__(self):

        self.count = 0

    def process_sample(
        self,
        dist,
        threshold=450,
        req_count=5
    ):

        if not vl53_check_distance(dist):
            return False

        if dist > threshold:

            self.count += 1

            if self.count >= req_count:

                self.count = 0

                return True

        else:

            self.count = 0

        return False


# =====================================================================
# MODULE 2: AS608 FINGERPRINT LOGIC
# =====================================================================

def as608_is_valid_id(fid):

    return fid > 0


def as608_process_auth_result(
    get_image_code,
    search_id
):

    if get_image_code != 0:

        return -1

    if search_id >= 0:

        return 1

    return 0


class AS608TouchDebounce:

    def __init__(self):

        self.locked = False
        self.last_touch_time = 0
        self.touch_flag = False

    def trigger_isr(self):

        if not self.locked:

            self.touch_flag = True

    def is_available(
        self,
        pin_is_low,
        current_millis
    ):

        if self.locked:

            if (
                pin_is_low
                and
                (current_millis - self.last_touch_time >= 500)
            ):

                self.locked = False

            return False

        if self.touch_flag:

            self.touch_flag = False
            self.locked = True
            self.last_touch_time = current_millis

            return True

        return False


def as608_check_timeout(
    start_time,
    current_time,
    timeout_limit=15000
):

    return (
        current_time - start_time
    ) >= timeout_limit


# =====================================================================
# MODULE 3: SERVO BARRIER LOGIC
# =====================================================================

SERVO_MIN_US = 500
SERVO_MAX_US = 2400
SERVO_RESOLUTION = 16
SERVO_PERIOD_US = 20000


def servo_calculate_pulse_width(angle):

    # Giới hạn góc an toàn 0° đến 180°
    constrained_angle = max(
        0,
        min(180, angle)
    )

    # Chuyển đổi sang xung 500us - 2400us
    pulse = (
        SERVO_MIN_US
        +
        (
            (constrained_angle - 0)
            *
            (SERVO_MAX_US - SERVO_MIN_US)
        )
        //
        (180 - 0)
    )

    return pulse


def servo_calculate_duty(pulse_width):

    max_duty = (
        (1 << SERVO_RESOLUTION) - 1
    )

    return (
        pulse_width * max_duty
    ) // SERVO_PERIOD_US


def barrier_check_timeout(
    start_time,
    current_time,
    timeout_limit=8000
):

    return (
        current_time - start_time
    ) >= timeout_limit


# =====================================================================
# MODULE 4: MOCK HARDWARE
# =====================================================================

class MockVL53L0X:
    """
    Giả lập cảm biến khoảng cách VL53L0X.
    """

    def __init__(self):

        self.distance = 0

    def set_distance(self, distance):

        self.distance = distance

    def read_distance(self):

        return self.distance


class MockAS608:
    """
    Giả lập cảm biến vân tay AS608.
    """

    def __init__(self):

        self.fingerprint_id = -1

    def set_fingerprint(self, fingerprint_id):

        self.fingerprint_id = fingerprint_id

    def search(self):

        return self.fingerprint_id


class MockServo:
    """
    Giả lập servo điều khiển barrier.
    """

    def __init__(self):

        self.opened = False

    def open(self):

        self.opened = True

    def close(self):

        self.opened = False

    def is_open(self):

        return self.opened


class MockBLE:
    """
    Giả lập giao tiếp BLE.

    Các message được lưu lại để kiểm tra
    trong Integration Test.
    """

    def __init__(self):

        self.messages = []

    def send(self, message):

        self.messages.append(message)

    def clear(self):

        self.messages.clear()

    def get_messages(self):

        return self.messages


# =====================================================================
# MOCK HỆ THỐNG PARKING
# =====================================================================

class ParkingSystemIntegrationMock:

    WAIT_VEHICLE = "WAIT_VEHICLE"
    WAIT_FINGERPRINT = "WAIT_FINGERPRINT"
    WAIT_VEHICLE_EXIT = "WAIT_VEHICLE_EXIT"

    VEHICLE_DETECT_DISTANCE = 300
    VEHICLE_EXIT_DISTANCE = 450
    VEHICLE_EXIT_CONFIRM_COUNT = 5

    def __init__(self):

        self.vl53 = MockVL53L0X()
        self.as608 = MockAS608()
        self.servo = MockServo()
        self.ble = MockBLE()

        self.state = self.WAIT_VEHICLE

    # -----------------------------------------------------------------
    # VL53L0X: phát hiện xe + BLE
    # -----------------------------------------------------------------

    def vehicle_detected(self, distance):

        self.vl53.set_distance(distance)

        current_distance = (
            self.vl53.read_distance()
        )

        if (
            current_distance > 0
            and
            current_distance <= self.VEHICLE_DETECT_DISTANCE
        ):

            self.ble.send(
                f"VEHICLE,{current_distance}"
            )

            self.state = (
                self.WAIT_FINGERPRINT
            )

            return True

        return False

    # -----------------------------------------------------------------
    # AS608: xác thực vân tay + Servo + BLE
    # -----------------------------------------------------------------

    def fingerprint_result(
        self,
        fingerprint_id
    ):

        self.as608.set_fingerprint(
            fingerprint_id
        )

        result_id = self.as608.search()

        if self.state != self.WAIT_FINGERPRINT:

            return False

        # Vân tay hợp lệ
        if result_id > 0:

            self.servo.open()

            self.ble.send(
                f"FP_OK,{result_id},80"
            )

            self.state = (
                self.WAIT_VEHICLE_EXIT
            )

            return True

        # Vân tay không hợp lệ
        self.ble.send("FP_FAIL")

        return False

    # -----------------------------------------------------------------
    # AS608 timeout + BLE
    # -----------------------------------------------------------------

    def fingerprint_timeout(self):

        if self.state == self.WAIT_FINGERPRINT:

            self.ble.send(
                "FP_TIMEOUT"
            )

            self.state = (
                self.WAIT_VEHICLE
            )

            return True

        return False

    # -----------------------------------------------------------------
    # VL53L0X + Servo + BLE:
    # xác nhận xe rời
    # -----------------------------------------------------------------

    def vehicle_exit_samples(
        self,
        samples
    ):

        if self.state != self.WAIT_VEHICLE_EXIT:

            return False

        count = 0

        for distance in samples:

            self.vl53.set_distance(
                distance
            )

            current_distance = (
                self.vl53.read_distance()
            )

            if (
                current_distance
                >
                self.VEHICLE_EXIT_DISTANCE
            ):

                count += 1

                if (
                    count
                    >=
                    self.VEHICLE_EXIT_CONFIRM_COUNT
                ):

                    self.servo.close()

                    self.ble.send(
                        "VEHICLE_EXIT"
                    )

                    self.state = (
                        self.WAIT_VEHICLE
                    )

                    return True

            else:

                count = 0

        return False


# =====================================================================
# UNIT TEST 1
# distanceSensorRead()
# =====================================================================

def run_distance_sensor_read():

    print(
        f"\n{Colors.BOLD}{Colors.YELLOW}"
        "--- distanceSensorRead() ---"
        f"{Colors.RESET}"
    )

    run_test(
        1,
        "distanceSensorRead()",
        "Khoảng cách gần hợp lệ (150mm)",
        lambda: assert_equal(
            150,
            distance_sensor_read(150)
        )
    )

    run_test(
        2,
        "distanceSensorRead()",
        "Khoảng cách trung bình hợp lệ (300mm)",
        lambda: assert_equal(
            300,
            distance_sensor_read(300)
        )
    )

    run_test(
        3,
        "distanceSensorRead()",
        "Khoảng cách xa hợp lệ (600mm)",
        lambda: assert_equal(
            600,
            distance_sensor_read(600)
        )
    )

    run_test(
        4,
        "distanceSensorRead()",
        "Mất kết nối I2C (0mm)",
        lambda: assert_equal(
            0,
            distance_sensor_read(0)
        )
    )

    run_test(
        5,
        "distanceSensorRead()",
        "Mã lỗi ToF (8190mm)",
        lambda: assert_equal(
            0,
            distance_sensor_read(8190)
        )
    )

    run_test(
        6,
        "distanceSensorRead()",
        "Vượt dải đo ToF (8500mm)",
        lambda: assert_equal(
            0,
            distance_sensor_read(8500)
        )
    )


# =====================================================================
# UNIT TEST 2
# vl53DetectVehicle()
# =====================================================================

def run_vl53_detect_vehicle():

    print(
        f"\n{Colors.BOLD}{Colors.YELLOW}"
        "--- vl53DetectVehicle() ---"
        f"{Colors.RESET}"
    )

    run_test(
        7,
        "vl53DetectVehicle()",
        "Xe tiến sát barrier (180mm <= 300mm)",
        lambda: assert_true(
            vl53_detect_vehicle(
                180,
                300
            )
        )
    )

    run_test(
        8,
        "vl53DetectVehicle()",
        "Xe đúng ngưỡng (300mm == 300mm)",
        lambda: assert_true(
            vl53_detect_vehicle(
                300,
                300
            )
        )
    )

    run_test(
        9,
        "vl53DetectVehicle()",
        "Xe ngoài vùng đón (350mm > 300mm)",
        lambda: assert_false(
            vl53_detect_vehicle(
                350,
                300
            )
        )
    )

    run_test(
        10,
        "vl53DetectVehicle()",
        "Không có xe (1200mm > 300mm)",
        lambda: assert_false(
            vl53_detect_vehicle(
                1200,
                300
            )
        )
    )

    run_test(
        11,
        "vl53DetectVehicle()",
        "Cảm biến trả về 0mm",
        lambda: assert_false(
            vl53_detect_vehicle(
                0,
                300
            )
        )
    )

    run_test(
        12,
        "vl53DetectVehicle()",
        "Cảm biến báo lỗi 8190mm",
        lambda: assert_false(
            vl53_detect_vehicle(
                8190,
                300
            )
        )
    )


# =====================================================================
# UNIT TEST 3
# as608ProcessAuthResult()
# =====================================================================

def run_as608_process_auth():

    print(
        f"\n{Colors.BOLD}{Colors.YELLOW}"
        "--- as608ProcessAuthResult() ---"
        f"{Colors.RESET}"
    )

    run_test(
        13,
        "as608ProcessAuthResult()",
        "Vân tay hợp lệ ID 1",
        lambda: assert_equal(
            1,
            as608_process_auth_result(
                0,
                1
            )
        )
    )

    run_test(
        14,
        "as608ProcessAuthResult()",
        "Vân tay hợp lệ ID 20",
        lambda: assert_equal(
            1,
            as608_process_auth_result(
                0,
                20
            )
        )
    )

    run_test(
        15,
        "as608ProcessAuthResult()",
        "Vân tay lạ không có trong CSDL",
        lambda: assert_equal(
            0,
            as608_process_auth_result(
                0,
                -1
            )
        )
    )

    run_test(
        16,
        "as608ProcessAuthResult()",
        "Chưa có ảnh vân tay (NOFINGER)",
        lambda: assert_equal(
            -1,
            as608_process_auth_result(
                2,
                -1
            )
        )
    )

    run_test(
        17,
        "as608ProcessAuthResult()",
        "Lỗi truyền thông UART",
        lambda: assert_equal(
            -1,
            as608_process_auth_result(
                1,
                -1
            )
        )
    )

    run_test(
        18,
        "as608ProcessAuthResult()",
        "Ảnh vân tay không hợp lệ",
        lambda: assert_equal(
            -1,
            as608_process_auth_result(
                3,
                -1
            )
        )
    )


# =====================================================================
# UNIT TEST 4
# as608TouchMockAvailable()
# =====================================================================

def run_as608_touch():

    print(
        f"\n{Colors.BOLD}{Colors.YELLOW}"
        "--- as608TouchMockAvailable() ---"
        f"{Colors.RESET}"
    )

    touch = AS608TouchDebounce()

    def tc19():

        touch.trigger_isr()

        assert_true(
            touch.is_available(
                False,
                1000
            )
        )

        assert_true(
            touch.locked
        )

    run_test(
        19,
        "as608TouchMockAvailable()",
        "Chạm tay lần đầu -> nhận ngắt và Lock",
        tc19
    )

    run_test(
        20,
        "as608TouchMockAvailable()",
        "Giữ ngón tay -> chống lặp",
        lambda: assert_false(
            touch.is_available(
                False,
                1200
            )
        )
    )

    run_test(
        21,
        "as608TouchMockAvailable()",
        "Nhiễu ngắt liên tục -> bỏ qua",
        lambda: assert_false(
            touch.is_available(
                False,
                1250
            )
        )
    )

    run_test(
        22,
        "as608TouchMockAvailable()",
        "Nhấc tay nhưng chưa đủ 500ms",
        lambda: assert_false(
            touch.is_available(
                True,
                1300
            )
        )
    )

    def tc23():

        touch.is_available(
            True,
            1600
        )

        assert_false(
            touch.locked
        )

        touch.trigger_isr()

        assert_true(
            touch.is_available(
                False,
                1700
            )
        )

    run_test(
        23,
        "as608TouchMockAvailable()",
        "Đã qua 500ms -> nhận lần chạm mới",
        tc23
    )


# =====================================================================
# UNIT TEST 5
# servoCalculatePulseWidth()
# =====================================================================

def run_servo_pulse():

    print(
        f"\n{Colors.BOLD}{Colors.YELLOW}"
        "--- servoCalculatePulseWidth() ---"
        f"{Colors.RESET}"
    )

    run_test(
        24,
        "servoCalculatePulseWidth()",
        "Góc 0°",
        lambda: assert_equal(
            500,
            servo_calculate_pulse_width(0)
        )
    )

    run_test(
        25,
        "servoCalculatePulseWidth()",
        "Góc 90°",
        lambda: assert_equal(
            1450,
            servo_calculate_pulse_width(90)
        )
    )

    run_test(
        26,
        "servoCalculatePulseWidth()",
        "Góc 180°",
        lambda: assert_equal(
            2400,
            servo_calculate_pulse_width(180)
        )
    )

    run_test(
        27,
        "servoCalculatePulseWidth()",
        "Góc 45°",
        lambda: assert_equal(
            975,
            servo_calculate_pulse_width(45)
        )
    )

    run_test(
        28,
        "servoCalculatePulseWidth()",
        "Góc âm -45° -> Clamp",
        lambda: assert_equal(
            500,
            servo_calculate_pulse_width(-45)
        )
    )

    run_test(
        29,
        "servoCalculatePulseWidth()",
        "Góc vượt 180° -> Clamp",
        lambda: assert_equal(
            2400,
            servo_calculate_pulse_width(250)
        )
    )


# =====================================================================
# UNIT TEST 6
# servoCalculateDuty()
# =====================================================================

def run_servo_duty():

    print(
        f"\n{Colors.BOLD}{Colors.YELLOW}"
        "--- servoCalculateDuty() ---"
        f"{Colors.RESET}"
    )

    run_test(
        30,
        "servoCalculateDuty()",
        "Duty 500us",
        lambda: assert_equal(
            1638,
            servo_calculate_duty(500)
        )
    )

    run_test(
        31,
        "servoCalculateDuty()",
        "Duty 1450us",
        lambda: assert_equal(
            4751,
            servo_calculate_duty(1450)
        )
    )

    run_test(
        32,
        "servoCalculateDuty()",
        "Duty 2400us",
        lambda: assert_equal(
            7864,
            servo_calculate_duty(2400)
        )
    )

    run_test(
        33,
        "servoCalculateDuty()",
        "Duty 1000us",
        lambda: assert_equal(
            3276,
            servo_calculate_duty(1000)
        )
    )

    run_test(
        34,
        "servoCalculateDuty()",
        "Duty 975us",
        lambda: assert_equal(
            3194,
            servo_calculate_duty(975)
        )
    )


# =====================================================================
# INTEGRATION TEST
# =====================================================================

def integration_IT01_vehicle_arrival():

    system = ParkingSystemIntegrationMock()

    result = system.vehicle_detected(180)

    assert_true(
        result,
        "VL53L0X phải phát hiện xe"
    )

    assert_equal(
        system.WAIT_FINGERPRINT,
        system.state
    )

    assert_equal(
        ["VEHICLE,180"],
        system.ble.get_messages()
    )


def integration_IT02_valid_fingerprint():

    system = ParkingSystemIntegrationMock()

    system.vehicle_detected(180)

    result = system.fingerprint_result(1)

    assert_true(
        result,
        "Vân tay hợp lệ phải được chấp nhận"
    )

    assert_true(
        system.servo.is_open(),
        "Barrier phải được mở"
    )

    assert_equal(
        system.WAIT_VEHICLE_EXIT,
        system.state
    )

    assert_equal(
        [
            "VEHICLE,180",
            "FP_OK,1,80"
        ],
        system.ble.get_messages()
    )


def integration_IT03_invalid_fingerprint():

    system = ParkingSystemIntegrationMock()

    system.vehicle_detected(180)

    result = system.fingerprint_result(-1)

    assert_false(
        result,
        "Vân tay không hợp lệ phải bị từ chối"
    )

    assert_false(
        system.servo.is_open(),
        "Barrier không được mở"
    )

    assert_equal(
        [
            "VEHICLE,180",
            "FP_FAIL"
        ],
        system.ble.get_messages()
    )


def integration_IT04_fingerprint_timeout():

    system = ParkingSystemIntegrationMock()

    system.vehicle_detected(180)

    result = system.fingerprint_timeout()

    assert_true(
        result,
        "Timeout phải được xử lý"
    )

    assert_false(
        system.servo.is_open(),
        "Barrier không được mở khi timeout"
    )

    assert_equal(
        system.WAIT_VEHICLE,
        system.state
    )

    assert_equal(
        [
            "VEHICLE,180",
            "FP_TIMEOUT"
        ],
        system.ble.get_messages()
    )


def integration_IT05_vehicle_exit():

    system = ParkingSystemIntegrationMock()

    system.vehicle_detected(180)

    system.fingerprint_result(1)

    samples = [
        500,
        520,
        550,
        600,
        650
    ]

    result = system.vehicle_exit_samples(
        samples
    )

    assert_true(
        result,
        "Phải xác nhận xe đã rời"
    )

    assert_false(
        system.servo.is_open(),
        "Barrier phải được đóng"
    )

    assert_equal(
        system.WAIT_VEHICLE,
        system.state
    )

    assert_equal(
        [
            "VEHICLE,180",
            "FP_OK,1,80",
            "VEHICLE_EXIT"
        ],
        system.ble.get_messages()
    )


def integration_IT06_full_cycle():

    system = ParkingSystemIntegrationMock()

    # Bước 1: phát hiện xe
    result = system.vehicle_detected(180)

    assert_true(
        result,
        "Xe phải được phát hiện"
    )

    assert_equal(
        system.WAIT_FINGERPRINT,
        system.state
    )

    assert_equal(
        "VEHICLE,180",
        system.ble.get_messages()[0]
    )

    # Bước 2: xác thực vân tay
    result = system.fingerprint_result(1)

    assert_true(
        result,
        "Vân tay hợp lệ phải được xác thực"
    )

    assert_true(
        system.servo.is_open(),
        "Barrier phải mở"
    )

    assert_equal(
        system.WAIT_VEHICLE_EXIT,
        system.state
    )

    assert_equal(
        "FP_OK,1,80",
        system.ble.get_messages()[1]
    )

    # Bước 3: xác nhận xe rời
    samples = [
        500,
        520,
        550,
        600,
        650
    ]

    result = system.vehicle_exit_samples(
        samples
    )

    assert_true(
        result,
        "Xe phải được xác nhận đã rời"
    )

    # Bước 4: barrier đóng
    assert_false(
        system.servo.is_open(),
        "Barrier phải đóng"
    )

    # Bước 5: quay về trạng thái chờ
    assert_equal(
        system.WAIT_VEHICLE,
        system.state
    )

    # Bước 6: kiểm tra BLE
    assert_equal(
        [
            "VEHICLE,180",
            "FP_OK,1,80",
            "VEHICLE_EXIT"
        ],
        system.ble.get_messages()
    )


def run_integration_tests():

    print(
        f"\n{Colors.BOLD}{Colors.YELLOW}"
        "--- INTEGRATION TEST ---"
        f"{Colors.RESET}"
    )

    run_test(
        35,
        "Integration Test",
        "Xe đến -> chuyển sang chờ xác thực",
        integration_IT01_vehicle_arrival
    )

    run_test(
        36,
        "Integration Test",
        "Xe đến + vân tay hợp lệ -> mở barrier",
        integration_IT02_valid_fingerprint
    )

    run_test(
        37,
        "Integration Test",
        "Xe đến + vân tay không hợp lệ",
        integration_IT03_invalid_fingerprint
    )

    run_test(
        38,
        "Integration Test",
        "Timeout chờ vân tay",
        integration_IT04_fingerprint_timeout
    )

    run_test(
        39,
        "Integration Test",
        "Xe rời -> đóng barrier",
        integration_IT05_vehicle_exit
    )

    run_test(
        40,
        "Integration Test",
        "Luồng hoàn chỉnh xe vào -> xác thực -> xe ra",
        integration_IT06_full_cycle
    )


# =====================================================================
# MAIN
# =====================================================================

def main():

    print(
        f"\n{Colors.BOLD}{Colors.CYAN}"
        "======================================================================"
        f"{Colors.RESET}"
    )

    print(
        f"{Colors.BOLD}"
        "       ESP32 PARKING SYSTEM - TEST RUNNER"
        f"{Colors.RESET}"
    )

    print(
        f"{Colors.BOLD}{Colors.CYAN}"
        "======================================================================"
        f"{Colors.RESET}"
    )

    print("""
Chọn nhóm kiểm thử:

  1. distanceSensorRead()          [6 test]
  2. vl53DetectVehicle()           [6 test]
  3. as608ProcessAuthResult()      [6 test]
  4. as608TouchMockAvailable()     [5 test]
  5. servoCalculatePulseWidth()    [6 test]
  6. servoCalculateDuty()          [5 test]
  7. Integration Test              [6 test]
  8. Chạy toàn bộ                  [40 test]
""")

    choice = input(
        "Nhập lựa chọn (1-8): "
    ).strip()

    reset_test_counter()

    # ============================================================
    # CHẠY RIÊNG TỪNG NHÓM
    # ============================================================

    if choice == "1":

        run_distance_sensor_read()

    elif choice == "2":

        run_vl53_detect_vehicle()

    elif choice == "3":

        run_as608_process_auth()

    elif choice == "4":

        run_as608_touch()

    elif choice == "5":

        run_servo_pulse()

    elif choice == "6":

        run_servo_duty()

    elif choice == "7":

        run_integration_tests()

    # ============================================================
    # CHẠY TOÀN BỘ 40 TEST
    # ============================================================

    elif choice == "8":

        run_distance_sensor_read()

        run_vl53_detect_vehicle()

        run_as608_process_auth()

        run_as608_touch()

        run_servo_pulse()

        run_servo_duty()

        run_integration_tests()

    else:

        print(
            f"{Colors.RED}"
            "Lựa chọn không hợp lệ."
            f"{Colors.RESET}"
        )

        return

    print_summary()


# =====================================================================
# START PROGRAM
# =====================================================================

if __name__ == "__main__":

    main()