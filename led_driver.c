#include "led_driver.h"
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include "meas.h"
#include "systimer.h"
#include "error_handler.h"
#include "gpio_driver.h"


#define LED_FB_CURRENT_SHOUNT_10_OHM (33)
#define LED_FB_VOLTAGE_100_K (1572)
#define LED_EN (GPIOB_SET(2))
#define LED_DIS (GPIOB_RESET(2))

#define LED_DRIVER_MAX_SETUP_CURRENT_MA (200)   ////
#define LED_DRIVER_MAX_FATAL_CURRENT_MA (300)   ////
#define LED_DRIVER_MAX_FATAL_VOLTAGE_MV (15000) ////


uint8_t led_current_pct;

static bool is_led_err, is_led_en;
static uint16_t led_ocr;

static const uint16_t led_driver_max_fatal_current_raw = ((uint32_t)LED_DRIVER_MAX_FATAL_CURRENT_MA * (uint32_t)ADC_MAX_CODE * (uint32_t)LED_FB_CURRENT_SHOUNT_10_OHM) / ((uint32_t)ADC_REF_MV * 10);
static const uint16_t led_driver_max_fatal_voltage_raw = ((uint32_t)LED_DRIVER_MAX_FATAL_VOLTAGE_MV * (uint32_t)ADC_MAX_CODE * 100) / ((uint32_t)ADC_REF_MV * 1572);


void led_driver_init(void) {
    LED_DIS;

    // Tim 1 init
    #define ICR1_VAL (128)   // TOP   //// 64 ???
    ICR1H = (uint8_t)(ICR1_VAL >> 8);
    ICR1L = (uint8_t)(ICR1_VAL >> 0);
    OCR1AH = 0;
    OCR1AL = 0;
    OCR1BH = 0;
    OCR1BL = 0;
    #define WGM1 (14)           // Fast PWM, TOP = ICR1
    TCCR1A = ((WGM1 & 0b11) << WGM10)  |
             (0 << COM1B0) |    // 0 - OCx disconnected
             (0 << COM1A0);     // Toggle OC1A on Compare Match
    TCCR1B = (0 << ICNC1) |     // Input Capture Noise Canceler
             (0 << ICES1) |     // Input Capture Edge Select
             (((WGM1 & 0b1100) >> 2) << WGM12)  |
             (1 << CS10);       // Clock Select: 0x00-0x05 -> 0/1/8/64/256/1024
    // Interrupt Mask Register
    TIMSK |= (0 << OCIE1B) |    // Output Compare B Match Interrupt
            (0 << OCIE1A) |    // Output Compare A Match Interrupt
            (0 << TOIE1);      // Overflow Interrupt

    is_led_err = false;
    is_led_en = false;
    led_current_pct = 0;
}


void led_driver_process(void) {
    static uint8_t led_current_pct_prev = 0xFF;
    static uint32_t led_current_raw;
    static uint8_t eh_skip;


    if (led_current_pct > 100) led_current_pct = 100;
    if (is_led_err) return;

    if (led_current_pct != led_current_pct_prev) {
        led_current_pct_prev = led_current_pct;

        if (led_current_pct > 0) {
            led_current_raw = LED_DRIVER_MAX_SETUP_CURRENT_MA;
            led_current_raw = led_current_raw * led_current_pct;
            //led_current_raw = led_current_raw / 100;
            led_current_raw = led_current_raw * (uint32_t)ADC_MAX_CODE * (uint32_t)LED_FB_CURRENT_SHOUNT_10_OHM;
            led_current_raw = led_current_raw / ADC_REF_MV;
            led_current_raw = led_current_raw / (10 * 100);

            if (!is_led_en) {
                is_led_en = true;
                eh_skip = 10;
                led_ocr = 0;
                OCR1AH = (uint8_t)led_ocr >> 8;
                OCR1AL = (uint8_t)led_ocr;
                TCCR1A |= (2 << COM1A0); // 0 - OCB connected
                LED_EN;
            }
        }
        else if (is_led_en) {
            is_led_en = false;
            led_ocr = 0;
            OCR1AH = (uint8_t)led_ocr >> 8;
            OCR1AL = (uint8_t)led_ocr;
            TCCR1A &= ~(3 << COM1A0); // 0 - OCB disconnected
            LED_DIS;
        }
    }

    if (meas_adc_data.channel_name.led_current < led_current_raw) {
        if (led_ocr < 0xFFFF) led_ocr++;
    }
    else {
        if (led_ocr > 0) led_ocr--;
    }

    if (eh_skip == 0) {
        if (meas_adc_data.channel_name.led_current > led_driver_max_fatal_current_raw) {
            is_led_err = true;
            led_ocr = 0;
            TCCR1A &= ~(3 << COM1A0); // 0 - OCB disconnected
            LED_DIS;
        }
        if (meas_adc_data.channel_name.led_voltage > led_driver_max_fatal_voltage_raw) {
            is_led_err = true;
            led_ocr = 0;
            TCCR1A &= ~(3 << COM1A0); // 0 - OCB disconnected
            LED_DIS;
        }
    }
    else {
        eh_skip--;
    }
    
    OCR1AH = (uint8_t)led_ocr >> 8;
    OCR1AL = (uint8_t)led_ocr;
}
