#include "systimer.h"
#include <stdint.h>
#include <stdbool.h>

static volatile uint32_t systimer_int_counter_ms = 0;


void systimer_process_ms(void) {
    static uint8_t div_cnt = 0;
    

    div_cnt++;
    if (div_cnt >= SYSTIMER_PROCESS_CALLS_IN_1MS) {
        systimer_int_counter_ms++;
        div_cnt = 0;
    }
}


timer_t systimer_set_ms(uint32_t time_ms) {
    return (systimer_int_counter_ms + time_ms);
}


bool systimer_triggered_ms(timer_t timeout) {
    return (systimer_int_counter_ms >= timeout);
}


void systimer_delay_ms(uint32_t time_ms) {
    timer_t timer;

    timer = systimer_set_ms(time_ms);
    while (!systimer_triggered_ms(timer)) ;
}
