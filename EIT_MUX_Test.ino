/**
 * =============================================================================
 * EIT_MUX_Test.ino - Test điều khiển 4x CD74HC4067 MUX cho hệ thống EIT
 * =============================================================================
 * 
 * Phần cứng:
 *   - ESP32-DEVKIT-V1 (MCU trung tâm)
 *   - U2: CD74HC4067 - MUX TX (Kênh Phát, COM ← DDS_IN)
 *   - U3: CD74HC4067 - MUX LA (Kênh Thu, COM → OUT_LA → ADAS1000)
 *   - U4: CD74HC4067 - MUX LL (Kênh Thu, COM → OUT_LL → ADAS1000)
 *   - U5: CD74HC4067 - MUX RA (Kênh Thu, COM → OUT_RA → ADAS1000)
 * 
 * Chức năng test:
 *   1. Quét toàn bộ kênh (0-15) trên tất cả MUX
 *   2. Chọn kênh thủ công trên từng MUX
 *   3. Mô phỏng chu trình quét EIT đầy đủ
 *   4. Xác minh trạng thái GPIO (readback)
 *   5. Giữ cấu hình cố định để đo bằng đồng hồ vạn năng
 *   6. Test tốc độ chuyển kênh
 * 
 * Cách dùng:
 *   1. Upload code lên ESP32
 *   2. Mở Serial Monitor (115200 baud)
 *   3. Chọn chức năng test từ menu
 *   4. Kiểm tra bằng đồng hồ vạn năng / oscilloscope trên các chân MUX
 * 
 * =============================================================================
 */

#include "PinConfig.h"
#include "MUX_Control.h"
#include "AD9850.h"

// =============================================================================
// KHỞI TẠO 4 ĐỐI TƯỢNG MUX
// Mỗi MUX có 4 chân địa chỉ riêng biệt, điều khiển hoàn toàn độc lập
// =============================================================================
MUX_Control muxTX(TX_S0, TX_S1, TX_S2, TX_S3, "TX");   // U2 - Kênh Phát
MUX_Control muxLA(LA_S0, LA_S1, LA_S2, LA_S3, "LA");   // U3 - Kênh Thu LA
MUX_Control muxLL(LL_S0, LL_S1, LL_S2, LL_S3, "LL");   // U4 - Kênh Thu LL
MUX_Control muxRA(RA_S0, RA_S1, RA_S2, RA_S3, "RA");   // U5 - Kênh Thu RA

// Mảng con trỏ để duyệt thuận tiện
MUX_Control* allMux[] = { &muxTX, &muxLA, &muxLL, &muxRA };
const uint8_t NUM_MUX = 4;

// DDS AD9850 - Bộ tạo sóng Sin
AD9850 dds(DDS_DATA, DDS_WCLK, DDS_FQUD);

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    delay(500); // Đợi Serial Monitor kết nối
    
    Serial.println();
    Serial.println("╔══════════════════════════════════════════════════════════╗");
    Serial.println("║   EIT MUX TEST - He thong 16 dien cuc                  ║");
    Serial.println("║   ESP32 + 4x CD74HC4067                                ║");
    Serial.println("╚══════════════════════════════════════════════════════════╝");
    Serial.println();
    
    // Khởi tạo tất cả MUX (cấu hình GPIO OUTPUT, đặt kênh 0)
    Serial.println("[INIT] Khoi tao 4 MUX...");
    for (uint8_t i = 0; i < NUM_MUX; i++) {
        allMux[i]->begin();
        Serial.printf("  ✓ %s MUX da khoi tao - Kenh 0 (E1)\n", allMux[i]->getName());
    }
    
    Serial.println();
    printPinMap();
    
    // Khởi tạo DDS AD9850
    Serial.println();
    Serial.println("[INIT] Khoi tao DDS AD9850...");
    dds.begin();
    
    Serial.println();
    printMenu();
}

// =============================================================================
// LOOP - Menu tương tác qua Serial
// =============================================================================
void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        // Xóa buffer còn lại
        while (Serial.available()) Serial.read();
        
        Serial.println();
        
        switch (cmd) {
            case '1': testSweepAll();       break;
            case '2': testSweepSingle();    break;
            case '3': testManualSelect();   break;
            case '4': testEITScanPattern(); break;
            case '5': testVerifyGPIO();     break;
            case '6': testHoldConfig();     break;
            case '7': testSwitchSpeed();    break;
            case '8': testIndependence();   break;
            case '9': testDDStoElectrodes(); break;
            case '0': printMenu();          break;
            default:
                Serial.printf("Lenh '%c' khong hop le. Nhap '0' de xem menu.\n", cmd);
                break;
        }
    }
}

// =============================================================================
// IN MENU
// =============================================================================
void printMenu() {
    Serial.println("┌──────────────────────────────────────────┐");
    Serial.println("│           MENU TEST MUX EIT              │");
    Serial.println("├──────────────────────────────────────────┤");
    Serial.println("│  1 - Quet toan bo kenh (tat ca MUX)     │");
    Serial.println("│  2 - Quet kenh tren 1 MUX cu the        │");
    Serial.println("│  3 - Chon kenh thu cong                 │");
    Serial.println("│  4 - Mo phong chu trinh quet EIT         │");
    Serial.println("│  5 - Xac minh trang thai GPIO           │");
    Serial.println("│  6 - Giu cau hinh de do (multimeter)    │");
    Serial.println("│  7 - Test toc do chuyen kenh            │");
    Serial.println("│  8 - Test tinh doc lap cua MUX          │");
    Serial.println("│  9 - DDS phat song ra 16 dien cuc       │");
    Serial.println("│  0 - Hien thi menu nay                  │");
    Serial.println("└──────────────────────────────────────────┘");
    Serial.println("Nhap so (0-8) va nhan Enter:");
}

// =============================================================================
// IN SƠ ĐỒ CHÂN
// =============================================================================
void printPinMap() {
    Serial.println("--- SO DO CHAN ESP32 → MUX ---");
    Serial.println("MUX TX (U2 - Phat):  S0=GPIO13, S1=GPIO12, S2=GPIO14, S3=GPIO27");
    Serial.println("MUX LA (U3 - Thu):   S0=GPIO26, S1=GPIO25, S2=GPIO33, S3=GPIO32");
    Serial.println("MUX LL (U4 - Thu):   S0=GPIO5,  S1=GPIO18, S2=GPIO15, S3=GPIO19");
    Serial.println("MUX RA (U5 - Thu):   S0=GPIO4,  S1=GPIO2,  S2=GPIO22, S3=GPIO23");
    Serial.println("DDS:                 WCLK=GPIO16, FQUD=GPIO17, DATA=GPIO21");
}

// =============================================================================
// TEST 1: Quét toàn bộ kênh trên tất cả MUX
// Mỗi MUX lần lượt chuyển từ kênh 0 → 15, xác minh GPIO sau mỗi bước
// =============================================================================
void testSweepAll() {
    Serial.println("=== TEST 1: QUET TOAN BO KENH ===");
    Serial.println("Moi MUX se chuyen tu kenh 0 (E1) den kenh 15 (E16)");
    Serial.println("Dung dong ho van nang do chan COM cua tung MUX de xac nhan.");
    Serial.println();
    
    uint8_t errorCount = 0;
    
    for (uint8_t i = 0; i < NUM_MUX; i++) {
        Serial.printf("--- %s MUX: Bat dau quet ---\n", allMux[i]->getName());
        
        for (uint8_t ch = 0; ch < NUM_ELECTRODES; ch++) {
            allMux[i]->selectChannel(ch);
            delayMicroseconds(MUX_SETTLE_US);
            
            // Xác minh GPIO readback
            if (!allMux[i]->verifyPinState()) {
                errorCount++;
            }
            allMux[i]->printState();
            delay(300); // Chậm để quan sát trên oscilloscope/LED
        }
        Serial.println();
    }
    
    if (errorCount == 0) {
        Serial.println(">>> KET QUA: TAT CA KENH CHUYEN DUNG! <<<");
    } else {
        Serial.printf(">>> CANH BAO: Co %d loi GPIO readback! <<<\n", errorCount);
    }
    Serial.println();
    printMenu();
}

// =============================================================================
// TEST 2: Quét kênh trên 1 MUX cụ thể
// =============================================================================
void testSweepSingle() {
    Serial.println("=== TEST 2: QUET 1 MUX CU THE ===");
    Serial.println("Chon MUX: 1=TX, 2=LA, 3=LL, 4=RA");
    
    while (!Serial.available()) { delay(10); }
    char c = Serial.read();
    while (Serial.available()) Serial.read();
    
    uint8_t idx = c - '1';
    if (idx >= NUM_MUX) {
        Serial.println("MUX khong hop le!");
        return;
    }
    
    MUX_Control* mux = allMux[idx];
    Serial.printf("Quet %s MUX (U%d)...\n\n", mux->getName(), idx + 2);
    
    for (uint8_t ch = 0; ch < NUM_ELECTRODES; ch++) {
        mux->selectChannel(ch);
        delayMicroseconds(MUX_SETTLE_US);
        mux->printState();
        mux->verifyPinState();
        delay(500);
    }
    
    Serial.println("\nHoan thanh!");
    printMenu();
}

// =============================================================================
// TEST 3: Chọn kênh thủ công
// Người dùng nhập MUX và kênh muốn chọn
// =============================================================================
void testManualSelect() {
    Serial.println("=== TEST 3: CHON KENH THU CONG ===");
    
    // Chọn MUX
    Serial.println("Chon MUX: 1=TX, 2=LA, 3=LL, 4=RA (hoac 5=Tat ca)");
    while (!Serial.available()) { delay(10); }
    char muxChar = Serial.read();
    while (Serial.available()) Serial.read();
    
    // Chọn kênh
    Serial.println("Nhap so kenh (0-15):");
    while (!Serial.available()) { delay(10); }
    String chStr = Serial.readStringUntil('\n');
    chStr.trim();
    uint8_t ch = chStr.toInt();
    
    if (ch > 15) {
        Serial.println("Kenh khong hop le (0-15)!");
        return;
    }
    
    if (muxChar == '5') {
        // Đặt tất cả MUX cùng kênh
        Serial.printf("Dat TAT CA MUX ve kenh %d (E%d):\n", ch, ch + 1);
        for (uint8_t i = 0; i < NUM_MUX; i++) {
            allMux[i]->selectChannel(ch);
            delayMicroseconds(MUX_SETTLE_US);
            allMux[i]->printState();
        }
    } else {
        uint8_t idx = muxChar - '1';
        if (idx >= NUM_MUX) {
            Serial.println("MUX khong hop le!");
            return;
        }
        allMux[idx]->selectChannel(ch);
        delayMicroseconds(MUX_SETTLE_US);
        allMux[idx]->printState();
    }
    
    Serial.println("\nCau hinh da dat. Co the do bang dong ho van nang.");
    printMenu();
}

// =============================================================================
// TEST 4: Mô phỏng chu trình quét EIT đầy đủ
// 
// Quy trình quét EIT:
//   - Vòng ngoài: TX MUX chọn điện cực bơm dòng (0 → 15)
//   - Vòng trong: 3 RX MUX (LA, LL, RA) quét đồng thời qua các điện cực đo
//   - Bỏ qua điện cực đang bơm dòng
//   - 3 kênh RX đo đồng thời → 5 bước mỗi vị trí TX (15 điện cực ÷ 3)
//   - Tổng: 16 TX × 5 bước = 80 bước đo mỗi frame
// =============================================================================
void testEITScanPattern() {
    Serial.println("=== TEST 4: MO PHONG CHU TRINH QUET EIT ===");
    Serial.println("16 dien cuc, phuong phap quet (Excitation Scan)");
    Serial.println("TX: Chuyen dien cuc phát | LA,LL,RA: Quet dien cuc thu");
    Serial.println();
    
    unsigned long startTime = micros();
    uint16_t totalSteps = 0;
    
    // Vòng lặp kích thích: TX dịch qua 16 điện cực
    for (uint8_t tx_ch = 0; tx_ch < NUM_ELECTRODES; tx_ch++) {
        
        // Bước 1: Đặt MUX TX → điện cực bơm dòng
        muxTX.selectChannelFast(tx_ch);
        delayMicroseconds(MUX_SETTLE_US);
        
        Serial.printf("TX=E%d | ", tx_ch + 1);
        
        // Xây dựng danh sách điện cực đo (bỏ qua điện cực TX)
        uint8_t rxList[15];
        uint8_t rxCount = 0;
        for (uint8_t i = 0; i < NUM_ELECTRODES; i++) {
            if (i != tx_ch) {
                rxList[rxCount++] = i;
            }
        }
        
        // Bước 2: Quét RX - mỗi bước đo 3 điện cực đồng thời
        for (uint8_t step = 0; step < rxCount; step += 3) {
            uint8_t la_ch = rxList[step];
            uint8_t ll_ch = (step + 1 < rxCount) ? rxList[step + 1] : rxList[step];
            uint8_t ra_ch = (step + 2 < rxCount) ? rxList[step + 2] : rxList[step];
            
            // Chuyển 3 MUX RX đồng thời
            muxLA.selectChannelFast(la_ch);
            muxLL.selectChannelFast(ll_ch);
            muxRA.selectChannelFast(ra_ch);
            delayMicroseconds(MUX_SETTLE_US);
            
            // Chờ ADC ổn định (mô phỏng đọc ADAS1000)
            delayMicroseconds(ADC_SETTLE_US);
            
            Serial.printf("[LA=E%d,LL=E%d,RA=E%d] ", la_ch+1, ll_ch+1, ra_ch+1);
            totalSteps++;
        }
        Serial.println();
    }
    
    unsigned long elapsed = micros() - startTime;
    
    Serial.println();
    Serial.println("--- THONG KE QUET ---");
    Serial.printf("Tong so buoc do:      %d\n", totalSteps);
    Serial.printf("Thoi gian 1 frame:    %lu us (%.2f ms)\n", elapsed, elapsed / 1000.0);
    Serial.printf("Thoi gian trung binh: %.1f us/buoc\n", (float)elapsed / totalSteps);
    Serial.printf("Toc do frame:         %.1f frames/giay\n", 1000000.0 / elapsed);
    Serial.println();
    printMenu();
}

// =============================================================================
// TEST 5: Xác minh trạng thái GPIO
// Đọc ngược tất cả chân và so sánh với kênh đã đặt
// =============================================================================
void testVerifyGPIO() {
    Serial.println("=== TEST 5: XAC MINH TRANG THAI GPIO ===");
    Serial.println("Doc nguoc trang thai GPIO cua tat ca MUX:\n");
    
    uint8_t totalErrors = 0;
    
    for (uint8_t i = 0; i < NUM_MUX; i++) {
        allMux[i]->printState();
        if (!allMux[i]->verifyPinState()) {
            totalErrors++;
        }
    }
    
    Serial.println();
    if (totalErrors == 0) {
        Serial.println(">>> TAT CA GPIO DUNG TRANG THAI! <<<");
    } else {
        Serial.printf(">>> PHAT HIEN %d MUX CO LOI GPIO! <<<\n", totalErrors);
        Serial.println("    Kiem tra: day noi, han thiêc, short circuit");
    }
    
    // In trạng thái thô của từng GPIO
    Serial.println("\n--- TRANG THAI THO GPIO ---");
    uint8_t pins[4];
    for (uint8_t i = 0; i < NUM_MUX; i++) {
        allMux[i]->getPins(pins);
        Serial.printf("  %s: ", allMux[i]->getName());
        for (uint8_t p = 0; p < 4; p++) {
            Serial.printf("GPIO%d=%d  ", pins[p], digitalRead(pins[p]));
        }
        Serial.println();
    }
    
    Serial.println();
    printMenu();
}

// =============================================================================
// TEST 6: Giữ cấu hình cố định để đo bằng multimeter
// Đặt các MUX ở kênh cố định, giữ nguyên cho đến khi nhấn phím
// =============================================================================
void testHoldConfig() {
    Serial.println("=== TEST 6: GIU CAU HINH CO DINH ===");
    Serial.println("Dat TX=E1, LA=E3, LL=E4, RA=E5");
    Serial.println("(Mo phong: bom dong E1, do dien ap E3/E4/E5)");
    Serial.println();
    
    muxTX.selectChannel(0);   // E1
    muxLA.selectChannel(2);   // E3
    muxLL.selectChannel(3);   // E4
    muxRA.selectChannel(4);   // E5
    
    delayMicroseconds(MUX_SETTLE_US);
    
    Serial.println("Trang thai hien tai:");
    for (uint8_t i = 0; i < NUM_MUX; i++) {
        allMux[i]->printState();
    }
    
    Serial.println();
    Serial.println("Cac phep do kiem tra:");
    Serial.println("  1. Do dien tro U2 COM → E0: phai gan 0 ohm (MUX on-resistance ~70 ohm)");
    Serial.println("  2. Do dien tro U2 COM → E1: phai vo cung (MUX off)");
    Serial.println("  3. Do dien tro U3 COM → E2: phai gan 0 ohm");
    Serial.println("  4. Tuong tu cho U4 (E3), U5 (E4)");
    Serial.println();
    Serial.println("Nhan phim bat ky de thoat...");
    
    // Giữ cấu hình cho đến khi user nhấn phím
    while (!Serial.available()) {
        delay(100);
    }
    while (Serial.available()) Serial.read();
    
    Serial.println("Da thoat che do giu.");
    printMenu();
}

// =============================================================================
// TEST 7: Đo tốc độ chuyển kênh
// Chuyển kênh nhiều lần và đo thời gian trung bình
// =============================================================================
void testSwitchSpeed() {
    Serial.println("=== TEST 7: TEST TOC DO CHUYEN KENH ===");
    
    const uint32_t iterations = 10000;
    
    // Test 1 MUX
    Serial.printf("Chuyen kenh %lu lan tren TX MUX...\n", iterations);
    unsigned long start = micros();
    for (uint32_t i = 0; i < iterations; i++) {
        muxTX.selectChannelFast(i & 0x0F); // Cycle 0-15
    }
    unsigned long elapsed1 = micros() - start;
    
    // Test 4 MUX đồng thời
    Serial.printf("Chuyen kenh %lu lan tren 4 MUX dong thoi...\n", iterations);
    start = micros();
    for (uint32_t i = 0; i < iterations; i++) {
        uint8_t ch = i & 0x0F;
        muxTX.selectChannelFast(ch);
        muxLA.selectChannelFast(ch);
        muxLL.selectChannelFast(ch);
        muxRA.selectChannelFast(ch);
    }
    unsigned long elapsed4 = micros() - start;
    
    Serial.println();
    Serial.println("--- KET QUA ---");
    Serial.printf("1 MUX:  %lu us tong / %.2f us moi lan chuyen\n", 
                   elapsed1, (float)elapsed1 / iterations);
    Serial.printf("4 MUX:  %lu us tong / %.2f us moi lan chuyen\n", 
                   elapsed4, (float)elapsed4 / iterations);
    Serial.printf("Toc do quet toi da (1 MUX): %.0f kenh/giay\n", 
                   1000000.0 / ((float)elapsed1 / iterations));
    Serial.printf("Toc do quet toi da (4 MUX): %.0f bo/giay\n", 
                   1000000.0 / ((float)elapsed4 / iterations));
    
    // Ước tính frame rate EIT
    float timePerStep = (float)elapsed4 / iterations; // 4 MUX switch time
    float settleTime = MUX_SETTLE_US + ADC_SETTLE_US;
    float stepTime = timePerStep + settleTime;
    float frameTime = stepTime * 80; // 80 steps per frame
    Serial.printf("\nUoc tinh EIT frame rate:\n");
    Serial.printf("  Thoi gian moi buoc: %.1f us (switch) + %d us (settle) = %.1f us\n",
                   timePerStep, MUX_SETTLE_US + ADC_SETTLE_US, stepTime);
    Serial.printf("  Thoi gian 1 frame (80 buoc): %.1f ms\n", frameTime / 1000.0);
    Serial.printf("  Frame rate toi da: %.1f fps\n", 1000000.0 / frameTime);
    
    Serial.println();
    printMenu();
}

// =============================================================================
// TEST 8: Test tính độc lập của các MUX
// Đặt mỗi MUX ở kênh khác nhau, xác minh không ảnh hưởng lẫn nhau
// =============================================================================
void testIndependence() {
    Serial.println("=== TEST 8: TEST TINH DOC LAP CUA MUX ===");
    Serial.println("Dat moi MUX o kenh khac nhau, xac minh khong anh huong nhau.\n");
    
    // Bộ test: mỗi dòng là [TX_ch, LA_ch, LL_ch, RA_ch]
    const uint8_t testCases[][4] = {
        { 0,  5, 10, 15},  // Các kênh cách xa nhau
        {15,  0,  5, 10},  // Đảo thứ tự
        { 1,  1,  1,  1},  // Cùng kênh (hợp lệ vì MUX độc lập)
        { 3,  7, 11,  2},  // Ngẫu nhiên
        { 0, 15,  0, 15},  // Xen kẽ min/max
    };
    const uint8_t numTests = sizeof(testCases) / sizeof(testCases[0]);
    
    uint8_t passCount = 0;
    
    for (uint8_t t = 0; t < numTests; t++) {
        Serial.printf("Test %d/%d: TX=E%d, LA=E%d, LL=E%d, RA=E%d\n",
            t + 1, numTests,
            testCases[t][0] + 1, testCases[t][1] + 1,
            testCases[t][2] + 1, testCases[t][3] + 1
        );
        
        // Đặt từng MUX
        muxTX.selectChannel(testCases[t][0]);
        muxLA.selectChannel(testCases[t][1]);
        muxLL.selectChannel(testCases[t][2]);
        muxRA.selectChannel(testCases[t][3]);
        
        delayMicroseconds(MUX_SETTLE_US);
        
        // Xác minh từng MUX
        bool allOK = true;
        for (uint8_t i = 0; i < NUM_MUX; i++) {
            if (!allMux[i]->verifyPinState()) {
                allOK = false;
                Serial.printf("  LOI: %s MUX khong dung kenh!\n", allMux[i]->getName());
            }
        }
        
        // Kiểm tra chéo: kênh đã đặt có đúng không
        bool crossCheck = true;
        crossCheck &= (muxTX.getCurrentChannel() == testCases[t][0]);
        crossCheck &= (muxLA.getCurrentChannel() == testCases[t][1]);
        crossCheck &= (muxLL.getCurrentChannel() == testCases[t][2]);
        crossCheck &= (muxRA.getCurrentChannel() == testCases[t][3]);
        
        if (allOK && crossCheck) {
            Serial.println("  => PASS");
            passCount++;
        } else {
            Serial.println("  => FAIL!");
        }
    }
    
    Serial.println();
    Serial.printf("KET QUA: %d/%d test PASS\n", passCount, numTests);
    
    if (passCount == numTests) {
        Serial.println(">>> CAC MUX HOAT DONG DOC LAP HOAN TOAN! <<<");
    } else {
        Serial.println(">>> CO LOI! Kiem tra day noi va short circuit. <<<");
    }
    
    Serial.println();
    printMenu();
}

// =============================================================================
// TEST 9: DDS AD9850 phát sóng Sin ra từng điện cực qua MUX TX
//
// Luồng tín hiệu: AD9850 → DDS_IN → MUX U2 COM → E(x) → Điện cực
//
// Cách kiểm tra:
//   - Dùng oscilloscope đo trên chân E(x) tương ứng của J1 connector
//   - Phải thấy sóng Sin ở tần số đã chọn (mặc định 50kHz)
//   - Nếu không có oscilloscope: dùng đồng hồ vạn năng chế độ AC (VAC)
//     sẽ đo được điện áp AC (giá trị RMS)
// =============================================================================
void testDDStoElectrodes() {
    Serial.println("=== TEST 9: DDS PHAT SONG RA 16 DIEN CUC ===");
    Serial.println();
    Serial.println("Chon che do:");
    Serial.println("  1 - Phat 50kHz, quet tu E1 den E16 (dung 3 giay moi kenh)");
    Serial.println("  2 - Phat tan so tuy chon, giu 1 kenh de do");
    Serial.println("  3 - Phat 50kHz, quet nhanh lien tuc (500ms/kenh)");

    while (!Serial.available()) { delay(10); }
    char mode = Serial.read();
    while (Serial.available()) Serial.read();

    switch (mode) {
        case '1': testDDS_SweepSlow();  break;
        case '2': testDDS_HoldOne();    break;
        case '3': testDDS_SweepFast();  break;
        default:
            Serial.println("Che do khong hop le!");
            printMenu();
            return;
    }
}

// --- Chế độ 1: Quét chậm 50kHz qua 16 điện cực ---
void testDDS_SweepSlow() {
    const uint32_t freq = 50000; // 50kHz
    
    Serial.printf("\nBat DDS tai %lu Hz (%.1f kHz)...\n", freq, freq / 1000.0);
    dds.setFrequency(freq);
    dds.printStatus();
    Serial.println();
    
    Serial.println("Bat dau quet: MUX TX chuyen kenh moi 3 giay.");
    Serial.println("Dung oscilloscope do tren chan E(x) cua J1.");
    Serial.println("Nhan phim bat ky de dung som.\n");

    for (uint8_t ch = 0; ch < NUM_ELECTRODES; ch++) {
        muxTX.selectChannel(ch);
        delayMicroseconds(MUX_SETTLE_US);

        Serial.printf(">>> TX -> Kenh %d (E%d) | Dang phat song %lu Hz...\n",
                       ch, ch + 1, freq);

        // Giữ 3 giây, kiểm tra Serial để thoát sớm
        for (int t = 0; t < 30; t++) {
            delay(100);
            if (Serial.available()) {
                while (Serial.available()) Serial.read();
                Serial.println("\nDung quet som.");
                dds.disable();
                Serial.println("DDS da TAT.");
                printMenu();
                return;
            }
        }
    }

    Serial.println("\nTat DDS...");
    dds.disable();
    Serial.println("Hoan thanh! Tat ca 16 kenh da duoc quet.");
    Serial.println();
    printMenu();
}

// --- Chế độ 2: Giữ 1 kênh, tần số tùy chọn ---
void testDDS_HoldOne() {
    Serial.println("\nNhap tan so (Hz), vi du 50000:");
    while (!Serial.available()) { delay(10); }
    String freqStr = Serial.readStringUntil('\n');
    freqStr.trim();
    uint32_t freq = freqStr.toInt();
    
    if (freq == 0 || freq > 40000000UL) {
        Serial.println("Tan so khong hop le (1 - 40000000 Hz)!");
        printMenu();
        return;
    }

    Serial.println("Nhap kenh (0-15), vi du 0 = E1:");
    while (!Serial.available()) { delay(10); }
    String chStr = Serial.readStringUntil('\n');
    chStr.trim();
    uint8_t ch = chStr.toInt();
    
    if (ch > 15) {
        Serial.println("Kenh khong hop le (0-15)!");
        printMenu();
        return;
    }

    // Đặt MUX TX
    muxTX.selectChannel(ch);
    delayMicroseconds(MUX_SETTLE_US);

    // Bật DDS
    dds.setFrequency(freq);

    Serial.println();
    Serial.println("================================================");
    Serial.printf( "  DDS: %lu Hz (%.2f kHz)\n", freq, freq / 1000.0);
    Serial.printf( "  MUX TX -> Kenh %d (E%d)\n", ch, ch + 1);
    Serial.println();
    Serial.println("  Song Sin dang phat ra dien cuc nay!");
    Serial.println("  Do bang oscilloscope tren chan E tuong ung.");
    Serial.println();
    Serial.println("  Nhan phim bat ky de TAT va thoat.");
    Serial.println("================================================");

    // Giữ cho đến khi nhấn phím
    while (!Serial.available()) {
        delay(100);
    }
    while (Serial.available()) Serial.read();

    dds.disable();
    Serial.println("\nDDS da TAT.");
    printMenu();
}

// --- Chế độ 3: Quét nhanh liên tục ---
void testDDS_SweepFast() {
    const uint32_t freq = 50000;
    
    Serial.printf("\nBat DDS tai %lu Hz, quet nhanh 500ms/kenh.\n", freq);
    Serial.println("Nhan phim bat ky de dung.\n");
    
    dds.setFrequency(freq);

    uint8_t ch = 0;
    while (true) {
        muxTX.selectChannelFast(ch);
        delayMicroseconds(MUX_SETTLE_US);
        
        Serial.printf("E%d ", ch + 1);
        if (ch == 15) Serial.println(); // Xuống dòng sau E16
        
        // 500ms delay, kiểm tra thoát
        for (int t = 0; t < 5; t++) {
            delay(100);
            if (Serial.available()) {
                while (Serial.available()) Serial.read();
                dds.disable();
                Serial.println("\n\nDDS da TAT. Hoan thanh.");
                printMenu();
                return;
            }
        }
        
        ch = (ch + 1) & 0x0F; // Quay vòng 0-15
    }
}
