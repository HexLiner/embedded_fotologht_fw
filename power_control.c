#include <stdint.h>
#include <stdbool.h>
#include "power_control.h"
#include "systimer.h"
#include "gpio_driver.h"


#define POWER_SENSE_PIN_STATE (GPIOC_GET(4))   ////

bool power_control_is_device_enabled = true;
static timer_t power_sense_timer;




void power_control_init(void) {
    POWER_LOCK_DIS;
    power_sense_timer = systimet_set_ms(1000);
}


void power_control_process(void) {
    static bool power_sense_buff = false;

    power_sense_buff |= POWER_SENSE_PIN_STATE;
    if (systimer_triggered_ms(power_sense_timer)) {
        power_sense_timer = systimet_set_ms(1000);
        power_control_is_device_enabled = power_sense_buff;
        power_sense_buff = false;
    }
}
