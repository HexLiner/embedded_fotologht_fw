#ifndef _MEASUREMENTS_H_
#define _MEASUREMENTS_H_


#include <stdint.h>
#include <stdbool.h>


#define ADC_REF_MV   (2510)
#define ADC_MAX_CODE (1024)


typedef union {
    struct {
        uint16_t led_voltage;
        uint16_t led_current;
    } channel_name;
    uint16_t channel_index[2];
} meas_adc_data_t;

extern meas_adc_data_t meas_adc_data;


extern void meas_init(void);
extern bool meas_is_data_ready(void);
extern void meas_process(void);


#endif    // _MEASUREMENTS_H_
