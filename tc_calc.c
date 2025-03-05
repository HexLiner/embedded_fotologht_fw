#include <stdint.h>


uint16_t tc_cal_1_temp_c, tc_cal_2_temp_c;
uint16_t tc_cal_1_volt_raw, tc_cal_2_volt_raw;


uint16_t tc_calc_c(uint16_t adc_raw) {
    uint16_t tc_cal_temp_delta, adc_value_delta, clbr_delta;
    uint32_t value_c;


    // y = y1 + ((y2 - y1) / (x2 - x1)) * (x - x1)
    // y = y1 + ((y2 - y1) * (x - x1)) / (x2 - x1)
    tc_cal_temp_delta = tc_cal_2_temp_c - tc_cal_1_temp_c;                     // (y2 - y1)
    adc_value_delta = adc_raw - tc_cal_1_volt_raw;                             // (x - x1)
    clbr_delta = tc_cal_2_volt_raw - tc_cal_1_volt_raw;                        // (x2 - x1)
    if (clbr_delta != 0) {
        value_c = tc_cal_temp_delta * adc_value_delta;
        value_c = value_c / clbr_delta;
        value_c += tc_cal_1_temp_c;                                            // + y1
    }

    return (uint16_t)value_c;
}
