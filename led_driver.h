#ifndef _LED_DRIVER_H_
#define _LED_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>


extern uint8_t led_current_pct;


extern void led_driver_init(void);
extern void led_driver_process(void);


#endif   // _LED_DRIVER_H_
