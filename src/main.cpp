#include <M5EPD.h>
#include <WiFi.h>

#include "Free_Fonts.h"
#include "util/stringutil.hpp"

RTC_DATA_ATTR bool started = false;

extern const uint8_t _binary_binres_images_test_bin_start[];
extern const uint8_t _binary_binres_images_test_bin_end[];

const char* RESET_REASON_STRINGS[] = {
    "UNKNOWN",
    "POWERON",    //!< Reset due to power-on event
    "EXTERNAL",        //!< Reset by external pin (not applicable for ESP32)
    "SOFTWARE",         //!< Software reset via esp_restart
    "PANIC",      //!< Software reset due to exception/panic
    "INT WATCHDOG",    //!< Reset (software or hardware) due to interrupt watchdog
    "TASK WATCHDOG",   //!< Reset due to task watchdog
    "OTHER WATCHDOG",        //!< Reset due to other watchdogs
    "DEEPSLEEP",  //!< Reset after exiting deep sleep mode
    "BROWNOUT",   //!< Brownout reset (software or hardware)
    "SDIO",       //!< Reset over SDIO
};

const char* WAKEUP_REASON_STRINGS[] = {
    "UNDEFINED",    //!< In case of deep sleep, reset was not caused by exit from deep sleep
    "INVALID",          //!< Not a wakeup cause, used to disable all wakeup sources with esp_sleep_disable_wakeup_source
    "RTC_IO",         //!< Wakeup caused by external signal using RTC_IO
    "RTC_CNTL",         //!< Wakeup caused by external signal using RTC_CNTL
    "TIMER",        //!< Wakeup caused by timer
    "TOUCHPAD",     //!< Wakeup caused by touchpad
    "ULP",          //!< Wakeup caused by ULP program
    "GPIO",         //!< Wakeup caused by GPIO (light sleep only)
    "UART",         //!< Wakeup caused by UART (light sleep only)
    "WIFI",              //!< Wakeup caused by WIFI (light sleep only)
    "COCPU INT",             //!< Wakeup caused by COCPU int
    "COCPU TRAP",   //!< Wakeup caused by COCPU crash
    "BT",           //!< Wakeup caused by BT (light sleep only)
};

class RemCanvas : public M5EPD_Canvas {
public:
    RemCanvas(M5EPD_Driver *driver) : M5EPD_Canvas(driver), driver(driver) {}

    using M5EPD_Canvas::pushCanvas;

    void pushCanvas(int32_t x, int32_t y, int32_t partX, int32_t partY, int32_t partW, int32_t partH, m5epd_update_mode_t mode) {
        int32_t roundedX = partX & ~3;
        int32_t roundedY = partY & ~3;

        partW += partX - roundedX;
        partH += partY - roundedY;
        
        partW = (partW + 3) & ~3;
        partH = (partH + 3) & ~3;

        partX = roundedX;
        partY = roundedY;
        
        uint8_t* buf = (uint8_t*)malloc((partW * partH + 1) / 2);

        auto y2 = partY + partH;
        for (int32_t ny = 0; ny < partH; ++ny) {
            // memset(&buf[ny * partW/2], ny&4 ? 0xFF : 0x00, partW/2);
            memcpy(&buf[ny * partW/2], &_img8[(partY + ny) * (_iwidth/2) + partX/2], partW/2);
        }

        driver->WritePartGram4bpp(x + partX, y + partY, partW, partH, buf);
        driver->UpdateArea(x + partX, y + partY, partW, partH, mode);

        free(buf);
    }

private:
    M5EPD_Driver *driver;
};

RemCanvas drawCanvas(&M5.EPD);

void testPrintText() {
    M5EPD_Canvas Info(&M5.EPD);
    Info.createCanvas(540, 50);
    Info.setFreeFont(FF18);
    Info.setTextDatum(CC_DATUM);
    Info.setTextColor(15);

    M5.EPD.UpdateFull(UPDATE_MODE_GC16);

    Info.fillCanvas(0);
    Info.drawString(string_format("Hello world %.2fV", 1e-3f * M5.getBatteryVoltage()).c_str(), 270, 20);
    Info.pushCanvas(0, 960/2 - 50/2, UPDATE_MODE_DU);
}

void first_boot() {

}

void setup() {
    disableLoopWDT();

    log_i("Reset reason: %s", RESET_REASON_STRINGS[esp_reset_reason()]);

    M5.begin(true, false, false, true, false);
    
    M5.EPD.SetRotation(M5EPD_Driver::ROTATE_90);
    M5.TP.SetRotation(GT911::ROTATE_90);

    M5.EPD.Clear(true);

    drawCanvas.createCanvas(540, 960);
    drawCanvas.fillCanvas(0);
    
    log_i("Pointer lol %p - %p = %u", _binary_binres_images_test_bin_start, _binary_binres_images_test_bin_end, (unsigned)(_binary_binres_images_test_bin_end - _binary_binres_images_test_bin_start));
    drawCanvas.pushImage(540/2-128/2, 200, 128, 128, _binary_binres_images_test_bin_start);
    drawCanvas.pushCanvas(0, 0, UPDATE_MODE_GC16);

    testPrintText();

    log_d("done");
}

void deep_sleep_with_touch_wakeup() {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW); // TOUCH_INT
    gpio_hold_en(GPIO_NUM_2); // M5EPD_MAIN_PWR_PIN
    gpio_deep_sleep_hold_en();
    esp_deep_sleep_start();
}

void light_sleep_with_touch_wakeup() {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW); // TOUCH_INT
    esp_light_sleep_start();
}

void loop() {
    log_d("wake");
    static bool wasFingerUp = true;

    M5.TP.update();

    uint16_t prevX = 0xFFFF, prevY = 0xFFFF, prevS = 0xFFFF;

    while (M5.TP.getFingerNum()) {
        auto x = M5.TP.readFingerX(0);
        auto y = M5.TP.readFingerY(0);
        auto s = M5.TP.readFingerSize(0);

        if (prevX != x || prevY != y || prevS != s) {
            log_d("CIRKELTJE %u %u %u", (unsigned) x, (unsigned) y, (unsigned) s);
            drawCanvas.fillCircle(x, y, s/2, 15);
            drawCanvas.pushCanvas(0, 0, x-s/2, y-s/2, s, s, UPDATE_MODE_DU);
        }

        prevX = x;
        prevY = y;
        prevS = s;

        M5.TP.update();
    }

    // M5.disableEPDPower();
    light_sleep_with_touch_wakeup();
    // M5.enableEPDPower();
    // delay(1000);
    // M5.EPD.begin(M5EPD_SCK_PIN, M5EPD_MOSI_PIN, M5EPD_MISO_PIN, M5EPD_CS_PIN, M5EPD_BUSY_PIN);
}
