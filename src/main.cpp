#include <M5EPD.h>
#include <WiFi.h>

#include "Free_Fonts.h"
#include "util/stringutil.hpp"

RTC_DATA_ATTR bool started = false;

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
    gpio_pullup_en(GPIO_NUM_36);
    //esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW); // TOUCH_INT
    esp_light_sleep_start();
}

void loop() {
    static int i = 0;
    light_sleep_with_touch_wakeup();
    log_i("Wakeup %d cause %s", ++i, WAKEUP_REASON_STRINGS[esp_sleep_get_wakeup_cause()]);
}
