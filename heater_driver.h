#ifndef _HEATER_DRIVER_H_
#define _HEATER_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include "eeprom_driver.h"


#define HEATER_MAX_SETUP_TEMP_C (500)


extern uint16_t heater_setup_temperature_c;
extern uint16_t heater_setup_temperature_raw;
extern uint16_t heater_real_temperature_c;
extern uint16_t cjs_temperature;

extern bool is_heater_tc_calibr;
extern uint16_t heater_cal_tc_t1_meas_raw;
extern uint16_t heater_cal_tc_t2_meas_raw;
extern uint16_t heater_ocr_calibr_k;
extern uint16_t heater_cal_ocr_minimal_ocr;


extern void heater_init(void);
extern void heater_process(void);


#endif   // _HEATER_DRIVER_H_
