<h1 align="center">🚗 Smart Parking System</h1>

<p align="center">
  <b>Hệ thống bãi đỗ xe thông minh tích hợp nhận dạng vân tay, cảm biến khoảng cách và giao tiếp BLE</b><br/>
  <i>Qt6 Desktop App (Raspberry Pi / Linux) + ESP32 Firmware (PlatformIO)</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Qt-6-green?logo=qt" />
  <img src="https://img.shields.io/badge/ESP32-Arduino-blue?logo=espressif" />
  <img src="https://img.shields.io/badge/BLE-NimBLE-orange" />
  <img src="https://img.shields.io/badge/SQLite-database-lightgrey?logo=sqlite" />
  <img src="https://img.shields.io/badge/PlatformIO-ESP32-purple?logo=platformio" />
</p>

---

## 📋 Tổng quan

**Smart Parking System** là hệ thống quản lý bãi đỗ xe thông minh gồm hai thành phần:

| Thành phần | Công nghệ | Mô tả |
|---|---|---|
| **Desktop App** | Qt6 / C++ / SQLite | Giao diện quản lý trên Raspberry Pi / Linux |
| **ESP32 Firmware** | Arduino / PlatformIO | Điều khiển cổng chắn, cảm biến, vân tay |

Hai thành phần giao tiếp với nhau qua **Bluetooth Low Energy (BLE)**.

---

## 🏗️ Kiến trúc hệ thống

```
┌─────────────────────────────────────┐       BLE        ┌──────────────────────────────┐
│        Raspberry Pi / Linux         │◄────────────────►│           ESP32              │
│                                     │                  │                              │
│  ┌─────────────────────────────┐    │                  │  ┌──────────────────────┐    │
│  │    Qt6 Desktop App          │    │                  │  │  VL53L0X (ToF)       │    │
│  │  ┌──────┐  ┌─────────────┐  │    │                  │  │  Cảm biến khoảng cách│    │
│  │  │Login │  │ MainWindow  │  │    │                  │  └──────────────────────┘    │
│  │  └──────┘  └─────────────┘  │    │                  │  ┌──────────────────────┐    │
│  │  ┌──────────────────────┐   │    │                  │  │  AS608 Fingerprint   │    │
│  │  │   BLE Client         │   │    │                  │  │  Cảm biến vân tay    │    │
│  │  └──────────────────────┘   │    │                  │  └──────────────────────┘    │
│  │  ┌──────────────────────┐   │    │                  │  ┌──────────────────────┐    │
│  │  │   SQLite Database    │   │    │                  │  │  Servo Motor         │    │
│  │  └──────────────────────┘   │    │                  │  │  Barrier (cổng chắn) │    │
│  └─────────────────────────────┘    │                  │  └──────────────────────┘    │
└─────────────────────────────────────┘                  └──────────────────────────────┘
```

---

## ✨ Tính năng

### 🖥️ Desktop App (Qt6)
- **Đăng nhập** bảo mật với tài khoản Admin / User
- **Quản lý người dùng**: thêm, sửa, xóa tài khoản
- **Đăng ký vân tay** từ xa qua BLE
- **Lịch sử ra/vào** theo dõi real-time
- **Kết nối BLE** tự động với ESP32
- **Database SQLite** lưu trữ người dùng và logs

### ⚡ ESP32 Firmware
- **State machine** 5 trạng thái quản lý chu trình xe vào/ra
- **VL53L0X ToF** phát hiện xe đến (ngưỡng 300mm)
- **AS608 Fingerprint** xác thực vân tay (timeout 15s)
- **Servo motor** điều khiển cổng chắn
- **BLE NimBLE** giao tiếp với desktop app

---

## 🗂️ Cấu trúc dự án

```
ParkingSystem/
├── 📁 Qt Desktop App
│   ├── main.cpp                    # Entry point
│   ├── login.{cpp,h,ui}            # Màn hình đăng nhập
│   ├── mainwindow.{cpp,h,ui}       # Cửa sổ chính
│   ├── bleclient.{cpp,h}           # BLE client
│   ├── database.{cpp,h}            # SQLite database
│   ├── userdialog.{cpp,h,ui}       # Dialog quản lý user
│   ├── userdashboard.ui            # Dashboard người dùng
│   ├── fingerprintenrolldialog.{cpp,h,ui}  # Dialog đăng ký vân tay
│   ├── resources.qrc               # Qt resources
│   ├── CMakeLists.txt              # CMake build config
│   ├── images/                     # Assets & icons
│   └── tests/                      # Unit tests Qt
│
└── 📁 ESP32_Parking_Project/       # PlatformIO project
    ├── platformio.ini              # PlatformIO config
    ├── src/
    │   ├── main.cpp                # State machine chính
    │   ├── barrier.cpp             # Điều khiển servo
    │   ├── ble_comm.cpp            # BLE server
    │   ├── distance_sensor.cpp     # VL53L0X driver
    │   └── fingerprint.cpp         # AS608 driver
    ├── include/
    │   ├── config.h                # Pin & threshold config
    │   ├── barrier.h
    │   ├── ble_comm.h
    │   ├── distance_sensor.h
    │   └── fingerprint.h
    ├── lib/                        # Custom libraries
    └── test/                       # Unit tests ESP32
```

---

## 🔧 Yêu cầu phần cứng

### ESP32
| Linh kiện | Kết nối ESP32 |
|---|---|
| VL53L0X (ToF sensor) | SDA=21, SCL=22, INT=27, XSHUT=26 |
| AS608 Fingerprint | RX=16, TX=17, TOUCH=34 |
| Servo Motor | PIN=25 |

### Desktop App
- Raspberry Pi 4 (hoặc Linux PC có Bluetooth)
- Qt 6.x với modules: Widgets, Bluetooth, Sql

---

## 🚀 Hướng dẫn build

### Desktop App (Qt6 / CMake)

```bash
# Clone repo
git clone https://github.com/YOUR_USERNAME/ParkingSystem.git
cd ParkingSystem

# Tạo build directory
mkdir build && cd build

# Configure & build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Chạy ứng dụng
./ParkingSystem
```

**Yêu cầu:**
```bash
sudo apt install qt6-base-dev qt6-connectivity-dev libqt6sql6-sqlite
```

### ESP32 Firmware (PlatformIO)

```bash
cd ESP32_Parking_Project

# Build & upload
pio run --target upload

# Mở Serial Monitor
pio device monitor --baud 115200
```

**Thư viện PlatformIO (tự động cài qua `platformio.ini`):**
- `Adafruit Fingerprint Sensor Library`
- `VL53L0X` (pololu)
- `NimBLE-Arduino` (h2zero)
- `Unity` (test framework)

---

## 🔄 Luồng hoạt động ESP32

```
WAIT_VEHICLE ──────────────────────────────────────────────────────────┐
     │                                                                 │
     │ Xe phát hiện (< 300mm)                                          │
     ▼                                                                 │
WAIT_FINGERPRINT ──── timeout 15s ──► WAIT_VEHICLE_EXIT               │
     │                                      │                         │
     │ Vân tay OK                            │ Xe rời (> 450mm × 5)   │
     ▼                                      ▼                         │
BARRIER_OPEN ──── Xe rời (> 450mm × 5) ─► BARRIER_CLOSING ───────────┘
```

---

## 📦 Dependencies

### Qt App
| Package | Version |
|---|---|
| Qt | ≥ 6.0 |
| CMake | ≥ 3.5 |
| SQLite | Built-in Qt |

### ESP32
| Library | Purpose |
|---|---|
| NimBLE-Arduino | BLE Server |
| Adafruit Fingerprint | AS608 driver |
| VL53L0X (pololu) | ToF sensor |
| Unity | Unit testing |

---

## 👤 Tác giả

**HIEN** — Smart Parking System Project

---

## 📄 License

MIT License — xem file [LICENSE](LICENSE) để biết thêm chi tiết.
