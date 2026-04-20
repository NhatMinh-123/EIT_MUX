# EIT_MUX_Test

Firmware Arduino cho ESP32 để kiểm tra và điều khiển hệ thống **Electrical Impedance Tomography (EIT) 16 điện cực** sử dụng 4 IC multiplexer CD74HC4067 và bộ tạo sóng DDS AD9850.

---

## Phần cứng

| Linh kiện | Số lượng | Mô tả |
|---|---|---|
| ESP32-DEVKIT-V1 | 1 | MCU trung tâm |
| CD74HC4067 | 4 | IC MUX 16 kênh (U2, U3, U4, U5) |
| AD9850 | 1 | Bộ tạo sóng Sin DDS 0–40 MHz |
| ADAS1000 | 1 | ADC đo điện áp vi sai (mở rộng) |

### Sơ đồ kết nối MUX

| MUX | Vai trò | COM | S0 | S1 | S2 | S3 |
|---|---|---|---|---|---|---|
| U2 (TX) | Kênh Phát | ← DDS_IN | GPIO13 | GPIO12 | GPIO14 | GPIO27 |
| U3 (LA) | Kênh Thu LA | → ADAS1000 LA | GPIO26 | GPIO25 | GPIO33 | GPIO32 |
| U4 (LL) | Kênh Thu LL | → ADAS1000 LL | GPIO5 | GPIO18 | GPIO15 | GPIO19 |
| U5 (RA) | Kênh Thu RA | → ADAS1000 RA | GPIO4 | GPIO2 | GPIO22 | GPIO23 |

### Kết nối DDS AD9850

| Tín hiệu | GPIO ESP32 |
|---|---|
| W_CLK | GPIO16 (RX2) |
| FQ_UD | GPIO17 (TX2) |
| Serial DATA | GPIO21 |
| RESET | GND (nối cứng) |

> **Lưu ý Strapping Pins:**  
> - **GPIO12** (TX_S1): ảnh hưởng flash voltage khi boot  
> - **GPIO15** (LL_S2): ảnh hưởng UART debug khi boot  
> - **GPIO2** (RA_S1): có LED onboard, cẩn thận khi boot  

---

## Cấu trúc file

```
EIT_MUX_Test/
├── EIT_MUX_Test.ino   # Sketch chính, menu tương tác Serial
├── PinConfig.h        # Định nghĩa toàn bộ GPIO và hằng số thời gian
├── MUX_Control.h      # Lớp điều khiển CD74HC4067
├── AD9850.h           # Driver DDS AD9850 (Serial mode)
└── README.md
```

---

## Cài đặt & Upload

1. Cài đặt **Arduino IDE** và thêm board **ESP32** (Espressif).
2. Mở `EIT_MUX_Test.ino`.
3. Chọn board **ESP32 Dev Module**, tốc độ baud upload **115200**.
4. Upload lên ESP32.

---

## Sử dụng

1. Mở **Serial Monitor** tại **115200 baud**.
2. Chọn chức năng từ menu:

| Phím | Chức năng |
|---|---|
| `1` | Quét toàn bộ kênh 0–15 trên tất cả MUX |
| `2` | Quét từng MUX riêng lẻ |
| `3` | Chọn kênh thủ công |
| `4` | Mô phỏng chu trình quét EIT đầy đủ |
| `5` | Xác minh trạng thái GPIO (readback) |
| `6` | Giữ cấu hình cố định để đo bằng đồng hồ vạn năng |
| `7` | Test tốc độ chuyển kênh |

---

## Nguyên lý hoạt động EIT

```
DDS AD9850 → Howland Current Pump → MUX TX (U2) → Điện cực phát (Ex)
                                                              ↓
                                               16 điện cực trên vật thể
                                                              ↓
MUX LA/LL/RA (U3/U4/U5) ← Điện cực đo (Ey, Ez, Ew) → ADAS1000 (đo UAD)
```

Mỗi chu kỳ quét EIT:
- Lần lượt kích hoạt từng điện cực làm cực **phát** (16 vị trí).
- Mỗi vị trí phát, đo điện áp tại tất cả điện cực còn lại qua 3 kênh thu (LA, LL, RA).
- Tổng: **16 × 16 = 256 phép đo** → tái tạo ảnh phân bố điện dẫn suất.

---

## Thông số thời gian

| Tham số | Giá trị | Ghi chú |
|---|---|---|
| `MUX_SETTLE_US` | 50 µs | Thời gian ổn định sau chuyển kênh MUX |
| `ADC_SETTLE_US` | 200 µs | Thời gian ổn định ADC sau chuyển kênh |
| CD74HC4067 propagation delay | ~70–100 ns | Theo datasheet |

---

## Mở rộng

- Kết nối **ADAS1000** qua SPI (chân đã dự phòng trong `PinConfig.h`):  
  CS → GPIO5, MOSI → GPIO23, MISO → GPIO19, SCK → GPIO18, DRDY → GPIO34
- Điều chỉnh tần số DDS trong `AD9850.h`: `setFrequency(freqHz)` (0–40 MHz, thạch anh 125 MHz mặc định).
- Thay đổi GPIO: chỉ cần sửa giá trị `#define` trong `PinConfig.h`.
