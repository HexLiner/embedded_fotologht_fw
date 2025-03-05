#include "heater_driver.h"
#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include "meas.h"
#include "eeprom_driver.h"
#include "device_registers.h"
#include "systimer.h"
#include "error_handler.h"
#include "mcp9804_temp_sensor_driver.h"


#define HEATER_MIN_DELTA_C             (30)
#define HEATER_FAST_HEATERING_DELTA_C  (20)


uint16_t heater_setup_temperature_c;
uint16_t heater_setup_temperature_raw;
uint16_t heater_real_temperature_c;
uint16_t cjs_temperature;

bool is_heater_tc_calibr;
uint16_t heater_cal_tc_t1_meas_raw;
uint16_t heater_cal_tc_t2_meas_raw;
uint16_t heater_ocr_calibr_k;
uint16_t heater_cal_ocr_minimal_ocr;

static const uint16_t heater_cal_tc_t1_c = 100;
static const uint16_t heater_cal_tc_t2_c = 400;
static timer_t heating_process_timer;
static const uint32_t heating_process_timeout_ms = 90000;




void heater_init(void) {
    bool is_incorrect_calibr;


    // Tim 1 init
    #define ICR1_VAL (65535)   // Need for TOP
    ICR1H = (uint8_t)(ICR1_VAL >> 8);
    ICR1L = (uint8_t)(ICR1_VAL >> 0);
    OCR1AH = 0;
    OCR1AL = 0;
    OCR1BH = 0;
    OCR1BL = 0;
    #define WGM1 (14)           // Fast PWM, TOP = ICR1
    TCCR1A = ((WGM1 & 0b11) << WGM10)  |
             (0 << COM1B0) |    // 0 - OCx disconnected
             (0 << COM1A0);     // Toggle OC1A on Compare Match     // set in process
    TCCR1B = (0 << ICNC1) |     // Input Capture Noise Canceler
             (0 << ICES1) |     // Input Capture Edge Select
             (((WGM1 & 0b1100) >> 2) << WGM12)  |
             (2 << CS10);       // Clock Select: 0x00-0x05 -> 0/1/8/64/256/1024
    // Interrupt Mask Register
    TIMSK1 = (0 << OCIE1B) |    // Output Compare B Match Interrupt
             (0 << OCIE1A) |    // Output Compare A Match Interrupt
             (0 << TOIE1);      // Overflow Interrupt

    mcp9804_temp_sensor_driver_init();

    eeprom_driver_read_16(EE_ADDR_CALIBR_TC_T1_MEAS_RAW, &heater_cal_tc_t1_meas_raw);
    eeprom_driver_read_16(EE_ADDR_CALIBR_TC_T2_MEAS_RAW, &heater_cal_tc_t2_meas_raw);
    eeprom_driver_read_16(EE_ADDR_OCR_CALIBR_K, &heater_ocr_calibr_k);
    eeprom_driver_read_16(EE_ADDR_OCR_CALIBR_MINIMAL_OCR, &heater_cal_ocr_minimal_ocr);

    // Check values
    is_incorrect_calibr = false;
    if (heater_cal_tc_t1_meas_raw > 0xF000) is_incorrect_calibr = true;
    if (heater_cal_tc_t2_meas_raw > 0xF000) is_incorrect_calibr = true;
    if (heater_ocr_calibr_k > 1000) is_incorrect_calibr = true;
    if (heater_cal_ocr_minimal_ocr > 0xF000) is_incorrect_calibr = true;

    if (is_incorrect_calibr) {
        heater_cal_tc_t1_meas_raw = 0;
        heater_cal_tc_t2_meas_raw = 1;
        heater_ocr_calibr_k = 1;
        heater_cal_ocr_minimal_ocr = 0;
        eh_state |= EH_STATUS_FLAG_CAL_ERR;
    }

    is_heater_tc_calibr = false;
    heater_setup_temperature_c = 0;
    heater_setup_temperature_raw = 0;
    heater_real_temperature_c = 999;
    cjs_temperature = 25 << 4;
    heating_process_timer = systimer_set_ms(heating_process_timeout_ms);
}


void heater_process(void) {
    static bool is_heater_enabled = false;
    static uint16_t heater_setup_temperature_perv = 0xFFFF;
    uint16_t heater_ocr_value;
    uint16_t heater_minimal_ocr;
    int64_t tmp;
    uint16_t heater_setup_temperature;
    uint16_t heater_temperature_delta_c;


    if (mcp9804_temp_sensor_get_temp(&cjs_temperature)) cjs_temperature = 25 << 4;

    if (is_heater_tc_calibr) {
        heater_setup_temperature = heater_setup_temperature_raw;
        heater_real_temperature_c = meas_adc_data.channel_name.heater_tc;
    }
    else {
        heater_setup_temperature = heater_setup_temperature_c;
        if (heater_setup_temperature > HEATER_MAX_SETUP_TEMP_C) heater_setup_temperature = HEATER_MAX_SETUP_TEMP_C;

        // Convert raw to temperature
        // y = y1 + ((y2 - y1) / (x2 - x1)) * (x - x1)
        // y = y1 + ((y2 - y1) * (x - x1)) / (x2 - x1)
        if (meas_adc_data.channel_name.heater_tc > 5) {
            // (x - x1)
            tmp = meas_adc_data.channel_name.heater_tc;
            ///tmp -= cjs_comp;
            tmp -= heater_cal_tc_t1_meas_raw;
            // * (y2 - y1)) / (x2 - x1)
            tmp = (((heater_cal_tc_t2_c - heater_cal_tc_t1_c) * tmp) / (heater_cal_tc_t2_meas_raw - heater_cal_tc_t1_meas_raw));
            // + y1
            tmp += (heater_cal_tc_t1_c);
            heater_real_temperature_c = (uint16_t)tmp;
        }
        else {
            heater_real_temperature_c = 0;
        }

        if (eh_state != 0) {
            TCCR1A = ((WGM1 & 0b11) << WGM10)  |
                     (0 << COM1B0) |    // 0 - OCx disconnected
                     (0 << COM1A0);     // 0 - OCx disconnected
            is_heater_enabled = false;
            return;
        }
    }

    if (heater_setup_temperature_perv != heater_setup_temperature) {
        heater_setup_temperature_perv = heater_setup_temperature;
        heating_process_timer = systimer_set_ms(heating_process_timeout_ms);
    }


    if (heater_setup_temperature == 0) {
        if (is_heater_enabled) {
            TCCR1A = ((WGM1 & 0b11) << WGM10)  |
                     (0 << COM1B0) |    // 0 - OCx disconnected
                     (0 << COM1A0);     // 0 - OCx disconnected
            is_heater_enabled = false;
        }
    }
    else if (!is_heater_enabled) {
        TCCR1A = ((WGM1 & 0b11) << WGM10)  |
             (0 << COM1B0) |    // 0 - OCx disconnected
             (2 << COM1A0);     // Toggle OC1A on Compare Match
        is_heater_enabled = true;
    }    


    if (heater_real_temperature_c < heater_setup_temperature) {
        heater_temperature_delta_c = heater_setup_temperature - heater_real_temperature_c;
        if (heater_temperature_delta_c > HEATER_MIN_DELTA_C) {
            if (systimer_triggered_ms(heating_process_timer)) eh_state |= EH_STATUS_FLAG_HEATER_ERR;
        }
        else {
            heating_process_timer = systimer_set_ms(heating_process_timeout_ms);
        }

        if (!is_heater_tc_calibr && (heater_temperature_delta_c > HEATER_FAST_HEATERING_DELTA_C)) heater_ocr_value = 0x5FFF;   // fast heating
        else heater_ocr_value = heater_temperature_delta_c * heater_ocr_calibr_k;
        heater_minimal_ocr = heater_cal_ocr_minimal_ocr * heater_setup_temperature;
        if (heater_ocr_value < heater_minimal_ocr) heater_ocr_value = heater_minimal_ocr;
    }
    else {
        heater_ocr_value = 0;
    }

    OCR1AH = (uint8_t)(heater_ocr_value >> 8);
    OCR1AL = (uint8_t)(heater_ocr_value >> 0);
}




    // Filters
/*
    static uint16_t heater_tc_meas_buff = 0;
    static uint8_t heater_tc_meas_buff_ctr = 0;
    heater_tc_meas_buff += meas_adc_data.channel_name.heater_tc;
    heater_tc_meas_buff_ctr++;
    if (heater_tc_meas_buff_ctr < 8) return;
    heater_real_temperature_c = heater_tc_meas_buff >> 3;
    heater_tc_meas_buff = 0;
    heater_tc_meas_buff_ctr = 0;
*/
/*
    static uint16_t heater_tc_meas_max = 0;
    static uint16_t heater_tc_meas_premax = 0;
    static uint16_t heater_tc_meas_premin = 0xFFFF;
    static uint16_t heater_tc_meas_min = 0xFFFF;
    heater_tc_meas_buff += meas_adc_data.channel_name.heater_tc;
    heater_tc_meas_buff_ctr++;
    if ((meas_adc_data.channel_name.heater_tc > heater_tc_meas_premax) && (meas_adc_data.channel_name.heater_tc > heater_tc_meas_max)) heater_tc_meas_max = meas_adc_data.channel_name.heater_tc;
    if ((meas_adc_data.channel_name.heater_tc > heater_tc_meas_premax) && (meas_adc_data.channel_name.heater_tc < heater_tc_meas_max)) heater_tc_meas_premax = meas_adc_data.channel_name.heater_tc;
    if ((meas_adc_data.channel_name.heater_tc < heater_tc_meas_premin) && (meas_adc_data.channel_name.heater_tc < heater_tc_meas_min)) heater_tc_meas_min = meas_adc_data.channel_name.heater_tc;
    if ((meas_adc_data.channel_name.heater_tc < heater_tc_meas_premin) && (meas_adc_data.channel_name.heater_tc > heater_tc_meas_min)) heater_tc_meas_premin = meas_adc_data.channel_name.heater_tc;
    if (heater_tc_meas_buff_ctr < 8) return;
    heater_tc_meas_buff -= heater_tc_meas_max;
    heater_tc_meas_buff -= heater_tc_meas_premax;
    heater_tc_meas_buff -= heater_tc_meas_premin;
    heater_tc_meas_buff -= heater_tc_meas_min;
    heater_real_temperature_c = heater_tc_meas_buff >> 2;
    heater_tc_meas_buff = 0;
    heater_tc_meas_buff_ctr = 0;
    heater_tc_meas_max = 0;
    heater_tc_meas_premax = 0;
    heater_tc_meas_premin = 0xFFFF;
    heater_tc_meas_min = 0xFFFF;
*/
