#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "gpio_driver.h"
#include "systimer.h"
#include "cli.h"
#include "encoder_driver.h"
#include "meas.h"
#include "char1602.h"
#include "led_driver.h"
#include "menu.h"



#define CLI_ENABLED             (1)




int main(void) {
    bool is_meas_ready;


    // Tim 2 init for systimer
    #ifdef __AVR_ATmega8__
    TCCR2 = (2 << CS20);  // Clock Select: 0x00-0x05 -> 0/1/8/32/64/128/256/1024
    TIMSK |= (1 << TOIE2);  // TOIE2 irq en 
    #else
    TCCR2A = 0;
    TCCR2B = (2 << CS20);  // Clock Select: 0x00-0x05 -> 0/1/8/32/64/128/256/1024
    TIMSK2 = (1 << TOIE2);  // TOIE2 irq en 
    #endif

    sei();   // global IRQ enable

    gpio_init();
    encoder_init();
    meas_init();
    led_driver_init();
    lcd1602_init();
    menu_init();
    #if (CLI_ENABLED != 0)
    cli_init();
    #endif


/*
    uint8_t digit_string[6];
    digit_string[5] = '\0';

    uint32_t voltage_mv, curr_ma;
*/


    while(1) {
        #if (CLI_ENABLED != 0)
        cli_process();
        drvice_registers_proc();
        #endif
        meas_process();
        encoder_process();

        is_meas_ready = meas_is_data_ready();
        if (is_meas_ready) {
            led_driver_process();
            /*
            if (systimer_triggered_ms(device_state_timer)) {
                device_state_timer = systimer_set_ms(400);

                voltage_mv = meas_adc_data.channel_name.led_voltage;
                voltage_mv = voltage_mv * ADC_REF_MV;
                voltage_mv = voltage_mv * 15;
                voltage_mv = voltage_mv / ((uint32_t)ADC_MAX_CODE);

                curr_ma = meas_adc_data.channel_name.led_current;
                curr_ma = curr_ma * ADC_REF_MV * 33;
                curr_ma = curr_ma / ADC_MAX_CODE;
                curr_ma = curr_ma / 100;

                lcd1602_move_coursor(0, 0);
                dig_to_string((uint16_t)curr_ma, digit_string);
                lcd1602_print_str("I=");
                lcd1602_print_str(digit_string);
                dig_to_string((uint16_t)voltage_mv, digit_string);
                lcd1602_print_str(" V=");
                lcd1602_print_str(digit_string);
            }
            */
        }

        menu_process();
    }

}



ISR(TIMER2_OVF_vect) {
    systimer_process_ms();
}
