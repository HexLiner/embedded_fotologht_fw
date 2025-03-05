#ifndef _SEG_LED_DRIVER_H_
#define _SEG_LED_DRIVER_H_

#include <stdint.h>


extern void seg_led_init(void);
extern void seg_led_process(void);

extern void seg_led_blink_en(void);
extern void seg_led_blink_dis(void);

extern void seg_led_print_dig(uint16_t digit);
extern void seg_led_clear_all_fn(void);
extern void seg_led_set_fn_fun(void);
extern void seg_led_set_fn_param(void);
extern void seg_led_set_fn_heat(void);
extern void seg_led_clr(void);

extern void seg_led_print_error(uint8_t error_code);
extern void seg_led_print_parameter(uint8_t parameter_code);
extern void seg_led_print_intro(void);
extern void seg_led_print_cool(void);
extern void seg_led_print_off(void);
extern void seg_led_print_sleep(void);
extern void seg_led_print_cli(void);


#endif   // _SEG_LED_DRIVER_H_