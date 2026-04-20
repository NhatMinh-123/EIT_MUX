/**
 * =============================================================================
 * AD9850.h - Driver điều khiển IC DDS AD9850 (Serial mode)
 * =============================================================================
 * 
 * AD9850: Bộ tổng hợp tần số trực tiếp (DDS), tạo sóng Sin 0-40MHz
 * Giao tiếp: Serial (bit-bang qua 3 dây: DATA, W_CLK, FQ_UD)
 * 
 * Cách hoạt động:
 *   1. Gửi 40 bit (8 bit control + 32 bit tuning word) qua DATA + W_CLK
 *   2. Pulse FQ_UD để cập nhật tần số đầu ra
 *   
 * Công thức tần số:
 *   f_out = (tuning_word × f_clk) / 2^32
 *   tuning_word = (f_out × 2^32) / f_clk
 *   
 *   Với f_clk = 125MHz (thạch anh mặc định AD9850):
 *     50kHz → tuning_word = 1,717,987
 *     10kHz → tuning_word = 343,597
 * 
 * Chân RESET: Nối cứng GND trên PCB (không cần điều khiển bằng code)
 * =============================================================================
 */

#ifndef AD9850_H
#define AD9850_H

#include <Arduino.h>

class AD9850 {
public:
    /**
     * Constructor
     * @param dataPin   GPIO nối tới AD9850 D7 (Serial Data)
     * @param wClkPin   GPIO nối tới AD9850 W_CLK
     * @param fqUdPin   GPIO nối tới AD9850 FQ_UD
     * @param refClk    Tần số thạch anh (mặc định 125MHz)
     */
    AD9850(uint8_t dataPin, uint8_t wClkPin, uint8_t fqUdPin, 
           uint32_t refClk = 125000000UL)
        : _dataPin(dataPin), _wClkPin(wClkPin), _fqUdPin(fqUdPin)
        , _refClk(refClk), _currentFreq(0), _enabled(false)
    {
    }

    /**
     * Khởi tạo - cấu hình GPIO và reset AD9850 vào Serial mode
     */
    void begin() {
        pinMode(_dataPin, OUTPUT);
        pinMode(_wClkPin, OUTPUT);
        pinMode(_fqUdPin, OUTPUT);

        digitalWrite(_dataPin, LOW);
        digitalWrite(_wClkPin, LOW);
        digitalWrite(_fqUdPin, LOW);

        // Khởi tạo AD9850 vào chế độ Serial:
        // Pulse W_CLK rồi FQ_UD để reset state machine nội bộ
        pulsePin(_wClkPin);
        pulsePin(_fqUdPin);

        // Đặt output = 0Hz (tắt tín hiệu)
        setFrequency(0);
        
        Serial.println("[DDS] AD9850 da khoi tao (Serial mode)");
    }

    /**
     * Đặt tần số đầu ra
     * @param freqHz  Tần số mong muốn (Hz), 0 = tắt output
     * 
     * Gửi 40 bit = 32 bit tuning word (LSB first) + 8 bit control (0x00)
     */
    void setFrequency(uint32_t freqHz) {
        // Tính tuning word: tw = (freq * 2^32) / refClk
        // Dùng phép tính 64-bit để tránh tràn số
        uint32_t tuningWord = (uint32_t)((double)freqHz * 4294967296.0 / _refClk);

        // Gửi 32 bit tuning word (LSB first)
        for (uint8_t i = 0; i < 32; i++) {
            digitalWrite(_dataPin, (tuningWord >> i) & 0x01);
            pulsePin(_wClkPin);
        }

        // Gửi 8 bit control = 0x00 (Phase = 0, Power-down = OFF)
        // Bit 2 = 1 sẽ power-down (tắt output)
        uint8_t controlByte = (freqHz == 0) ? 0x04 : 0x00;
        for (uint8_t i = 0; i < 8; i++) {
            digitalWrite(_dataPin, (controlByte >> i) & 0x01);
            pulsePin(_wClkPin);
        }

        // Pulse FQ_UD để cập nhật tần số
        pulsePin(_fqUdPin);

        _currentFreq = freqHz;
        _enabled = (freqHz > 0);
    }

    /**
     * Bật tín hiệu ở tần số đã đặt trước đó
     */
    void enable() {
        if (_currentFreq > 0) {
            setFrequency(_currentFreq);
        }
    }

    /**
     * Tắt tín hiệu (power-down AD9850)
     */
    void disable() {
        // Gửi control byte với bit power-down = 1
        for (uint8_t i = 0; i < 32; i++) {
            digitalWrite(_dataPin, LOW);
            pulsePin(_wClkPin);
        }
        // Control byte = 0x04 (bit 2 = power-down)
        uint8_t controlByte = 0x04;
        for (uint8_t i = 0; i < 8; i++) {
            digitalWrite(_dataPin, (controlByte >> i) & 0x01);
            pulsePin(_wClkPin);
        }
        pulsePin(_fqUdPin);
        _enabled = false;
    }

    /**
     * Lấy tần số hiện tại
     */
    uint32_t getFrequency() const { return _currentFreq; }
    
    /**
     * Kiểm tra DDS đang bật hay tắt
     */
    bool isEnabled() const { return _enabled; }

    /**
     * In thông tin trạng thái
     */
    void printStatus() const {
        Serial.printf("[DDS] Trang thai: %s | Tan so: %lu Hz (%.1f kHz)\n",
            _enabled ? "BAT" : "TAT",
            _currentFreq, _currentFreq / 1000.0);
        Serial.printf("[DDS] Ref Clock: %lu MHz | GPIO: DATA=%d, WCLK=%d, FQUD=%d\n",
            _refClk / 1000000UL, _dataPin, _wClkPin, _fqUdPin);
    }

private:
    uint8_t _dataPin, _wClkPin, _fqUdPin;
    uint32_t _refClk;
    uint32_t _currentFreq;
    bool _enabled;

    /**
     * Tạo xung LOW→HIGH→LOW trên 1 chân
     */
    void pulsePin(uint8_t pin) {
        digitalWrite(pin, LOW);
        digitalWrite(pin, HIGH);
        digitalWrite(pin, LOW);
    }
};

#endif // AD9850_H
