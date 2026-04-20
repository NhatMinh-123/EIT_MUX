/**
 * =============================================================================
 * MUX_Control.h - Lớp điều khiển IC CD74HC4067 (16-channel Analog MUX)
 * =============================================================================
 * 
 * Mỗi đối tượng MUX_Control quản lý 4 chân địa chỉ (S0-S3) độc lập.
 * Hỗ trợ chọn kênh 0-15 tương ứng với điện cực E1-E16.
 * 
 * CD74HC4067 Truth Table:
 *   S3 S2 S1 S0 | Kênh
 *    0  0  0  0 |  E0  (Điện cực 1)
 *    0  0  0  1 |  E1  (Điện cực 2)
 *    0  0  1  0 |  E2  (Điện cực 3)
 *    ...
 *    1  1  1  1 |  E15 (Điện cực 16)
 * 
 * =============================================================================
 */

#ifndef MUX_CONTROL_H
#define MUX_CONTROL_H

#include <Arduino.h>

class MUX_Control {
public:
    /**
     * Constructor - Tạo đối tượng điều khiển MUX
     * @param s0  GPIO cho chân S0 (LSB địa chỉ)
     * @param s1  GPIO cho chân S1
     * @param s2  GPIO cho chân S2
     * @param s3  GPIO cho chân S3 (MSB địa chỉ)
     * @param name Tên MUX để hiển thị debug (vd: "TX", "LA", "LL", "RA")
     */
    MUX_Control(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, const char* name)
        : _pinS0(s0), _pinS1(s1), _pinS2(s2), _pinS3(s3)
        , _name(name), _currentChannel(0)
    {
    }

    /**
     * Khởi tạo - Cấu hình các chân GPIO thành OUTPUT và đặt kênh 0
     * Gọi trong setup()
     */
    void begin() {
        pinMode(_pinS0, OUTPUT);
        pinMode(_pinS1, OUTPUT);
        pinMode(_pinS2, OUTPUT);
        pinMode(_pinS3, OUTPUT);
        selectChannel(0);
    }

    /**
     * Chọn kênh MUX (0-15)
     * @param ch  Số kênh (0 = E0/Điện cực 1, ... 15 = E15/Điện cực 16)
     * 
     * Nguyên lý: Ghi 4 bit nhị phân của số kênh ra 4 chân S0-S3
     *   ch = 5 (0101b) → S0=1, S1=0, S2=1, S3=0
     */
    void selectChannel(uint8_t ch) {
        if (ch > 15) {
            Serial.printf("[%s] LOI: Kenh %d khong hop le (0-15)!\n", _name, ch);
            return;
        }
        
        // Ghi trực tiếp từng bit vào GPIO
        // Bit 0 → S0, Bit 1 → S1, Bit 2 → S2, Bit 3 → S3
        digitalWrite(_pinS0, (ch >> 0) & 0x01);
        digitalWrite(_pinS1, (ch >> 1) & 0x01);
        digitalWrite(_pinS2, (ch >> 2) & 0x01);
        digitalWrite(_pinS3, (ch >> 3) & 0x01);
        
        _currentChannel = ch;
    }

    /**
     * Chọn kênh nhanh - không kiểm tra giới hạn, dùng thanh ghi GPIO trực tiếp
     * Dùng trong vòng lặp quét EIT cần tốc độ cao
     * @param ch  Số kênh (0-15), PHẢI đảm bảo hợp lệ trước khi gọi
     */
    void selectChannelFast(uint8_t ch) {
        // Dùng digitalWriteFast hoặc thanh ghi trực tiếp nếu cần tối ưu
        // Hiện tại dùng digitalWrite để tương thích
        digitalWrite(_pinS0, (ch >> 0) & 0x01);
        digitalWrite(_pinS1, (ch >> 1) & 0x01);
        digitalWrite(_pinS2, (ch >> 2) & 0x01);
        digitalWrite(_pinS3, (ch >> 3) & 0x01);
        _currentChannel = ch;
    }

    /**
     * Lấy kênh hiện tại
     */
    uint8_t getCurrentChannel() const {
        return _currentChannel;
    }

    /**
     * Lấy tên MUX
     */
    const char* getName() const {
        return _name;
    }

    /**
     * In trạng thái hiện tại ra Serial
     */
    void printState() const {
        uint8_t ch = _currentChannel;
        Serial.printf("  [%s] Kenh: %2d (E%d) | S3=%d S2=%d S1=%d S0=%d | GPIO: S0=%d S1=%d S2=%d S3=%d\n",
            _name, ch, ch + 1,
            (ch >> 3) & 1, (ch >> 2) & 1, (ch >> 1) & 1, (ch >> 0) & 1,
            _pinS0, _pinS1, _pinS2, _pinS3
        );
    }

    /**
     * Đọc ngược trạng thái GPIO thực tế để xác nhận
     * So sánh giá trị mong đợi với giá trị đọc được
     * @return true nếu tất cả chân khớp, false nếu có lỗi
     */
    bool verifyPinState() const {
        uint8_t ch = _currentChannel;
        bool s0_expected = (ch >> 0) & 0x01;
        bool s1_expected = (ch >> 1) & 0x01;
        bool s2_expected = (ch >> 2) & 0x01;
        bool s3_expected = (ch >> 3) & 0x01;

        bool s0_actual = digitalRead(_pinS0);
        bool s1_actual = digitalRead(_pinS1);
        bool s2_actual = digitalRead(_pinS2);
        bool s3_actual = digitalRead(_pinS3);

        bool ok = (s0_expected == s0_actual) && 
                  (s1_expected == s1_actual) &&
                  (s2_expected == s2_actual) && 
                  (s3_expected == s3_actual);

        if (!ok) {
            Serial.printf("  [%s] CANH BAO! Kenh %d - Mong doi: %d%d%d%d | Thuc te: %d%d%d%d\n",
                _name, ch,
                s3_expected, s2_expected, s1_expected, s0_expected,
                s3_actual, s2_actual, s1_actual, s0_actual
            );
        }
        return ok;
    }

    /**
     * Lấy mảng chân GPIO [S0, S1, S2, S3]
     */
    void getPins(uint8_t pins[4]) const {
        pins[0] = _pinS0;
        pins[1] = _pinS1;
        pins[2] = _pinS2;
        pins[3] = _pinS3;
    }

private:
    uint8_t _pinS0, _pinS1, _pinS2, _pinS3;
    const char* _name;
    uint8_t _currentChannel;
};

#endif // MUX_CONTROL_H
