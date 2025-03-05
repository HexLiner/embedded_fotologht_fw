#include <stdint.h>
#include <stdbool.h>
#include "gpio_driver.h"


#ifndef _POWER_CONTROL_H_
#define _POWER_CONTROL_H_


extern bool power_control_is_device_enabled;
#define POWER_LOCK_EN (GPIOC_SET(5))   ////
#define POWER_LOCK_DIS (GPIOC_RESET(5))   ////


extern void power_control_init(void);
extern void power_control_process(void);


#endif   // _POWER_CONTROL_H_
