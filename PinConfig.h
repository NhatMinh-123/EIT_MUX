/**
 * =============================================================================
 * PinConfig.h - Cấu hình chân GPIO ESP32 cho hệ thống EIT 16 điện cực
 * =============================================================================
 * 
 * Dựa trên schematic: ESP32-DEVKIT-V1 (U1) kết nối 4x CD74HC4067 MUX
 * 
 * MUX U2 (TX)  : Kênh Phát - nhận sóng AC từ DDS, bơm dòng vào điện cực
 * MUX U3 (LA)  : Kênh Thu LA - đo điện áp, đưa về ADAS1000 ngõ LA
 * MUX U4 (LL)  : Kênh Thu LL - đo điện áp, đưa về ADAS1000 ngõ LL
 * MUX U5 (RA)  : Kênh Thu RA - đo điện áp, đưa về ADAS1000 ngõ RA
 * 
 * LƯU Ý QUAN TRỌNG: 
 *   - Mỗi MUX có 4 đường địa chỉ RIÊNG (S0-S3) → điều khiển ĐỘC LẬP
 *   - KHÔNG dùng chung đường địa chỉ để tránh chập que đo
 *   - GPIO2 (RA_S1) là strapping pin, có LED onboard → cẩn thận khi boot
 *   - GPIO15 (LL_S2) là strapping pin → ảnh hưởng UART debug khi boot
 *   - GPIO12 (TX_S1) là strapping pin → ảnh hưởng flash voltage
 * 
 * Nếu chân không khớp với mạch thực tế, chỉ cần sửa giá trị #define bên dưới.
 * =============================================================================
 */

#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// =============================================================================
// SỐ LƯỢNG ĐIỆN CỰC
// =============================================================================
#define NUM_ELECTRODES  16

// =============================================================================
// MUX U2 - KÊNH PHÁT (TX / Injection)
// COM (pin 1) ← DDS_IN (sóng AC từ AD9850 qua Howland Current Pump)
// Chọn kênh E0-E15 để bơm dòng điện vào điện cực tương ứng
// =============================================================================
#define TX_S0   13    // GPIO13 → U2 pin 10 (S0)
#define TX_S1   12    // GPIO12 → U2 pin 11 (S1)
#define TX_S2   14    // GPIO14 → U2 pin 14 (S2)
#define TX_S3   27    // GPIO27 → U2 pin 13 (S3)

// =============================================================================
// MUX U3 - KÊNH THU LA (Left Arm → ADAS1000 LA input)
// COM (pin 1) → OUT_LA → ADAS1000
// =============================================================================
#define LA_S0   26    // GPIO26 → U3 pin 10 (S0)
#define LA_S1   25    // GPIO25 → U3 pin 11 (S1)
#define LA_S2   33    // GPIO33 → U3 pin 14 (S2)
#define LA_S3   32    // GPIO32 → U3 pin 13 (S3)

// =============================================================================
// MUX U4 - KÊNH THU LL (Left Leg → ADAS1000 LL input)
// COM (pin 1) → OUT_LL → ADAS1000
// =============================================================================
#define LL_S0    5    // GPIO5  → U4 pin 10 (S0)
#define LL_S1   18    // GPIO18 → U4 pin 11 (S1)
#define LL_S2   15    // GPIO15 → U4 pin 14 (S2)
#define LL_S3   19    // GPIO19 → U4 pin 13 (S3)

// =============================================================================
// MUX U5 - KÊNH THU RA (Right Arm → ADAS1000 RA input)
// COM (pin 1) → OUT_RA → ADAS1000
// =============================================================================
#define RA_S0    4    // GPIO4  → U5 pin 10 (S0)
#define RA_S1    2    // GPIO2  → U5 pin 11 (S1)  [⚠ Strapping pin + LED]
#define RA_S2   22    // GPIO22 → U5 pin 14 (S2)
#define RA_S3   23    // GPIO23 → U5 pin 13 (S3)

// =============================================================================
// DDS AD9850 - BỘ TẠO TÍN HIỆU SÓNG SIN
// RESET nối cứng GND (không cần code)
// =============================================================================
#define DDS_WCLK  16  // GPIO16 (RX2) → AD9850 W_CLK
#define DDS_FQUD  17  // GPIO17 (TX2) → AD9850 FQ_UD
#define DDS_DATA  21  // GPIO21       → AD9850 Serial DATA

// =============================================================================
// ADAS1000 - ADC ĐO ĐIỆN ÁP VI SAI (giao tiếp SPI)
// Chưa kết nối trực tiếp ESP32 trong schematic này 
// (đi qua bo SDP riêng), nhưng để sẵn cho mở rộng
// =============================================================================
// #define ADAS_CS    SS     // Chip Select (mặc định GPIO5 nếu dùng VSPI)
// #define ADAS_MOSI  23    // MOSI
// #define ADAS_MISO  19    // MISO
// #define ADAS_SCK   18    // Clock
// #define ADAS_DRDY  34    // Data Ready (input only pin)

// =============================================================================
// THỜI GIAN CHUYỂN MẠCH
// CD74HC4067 propagation delay: ~70-100ns typ
// Thêm margin cho ổn định tín hiệu analog
// =============================================================================
#define MUX_SETTLE_US    50   // Thời gian chờ MUX ổn định (microseconds)
#define ADC_SETTLE_US   200   // Thời gian chờ ADC ổn định sau chuyển kênh

#endif // PIN_CONFIG_H
