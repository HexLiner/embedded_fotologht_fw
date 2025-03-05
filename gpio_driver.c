#include "gpio_driver.h"
#include <avr/io.h>


#define GPIO_INPUT  (0)
#define GPIO_OUTPUT (1)


void gpio_init(void) {
    // Port B initialization
    DDRB = (GPIO_INPUT  << DDB7) |
           (GPIO_INPUT  << DDB6) |
           (GPIO_INPUT  << DDB5) |
           (GPIO_INPUT  << DDB4) |
           (GPIO_OUTPUT << DDB3) |
           (GPIO_OUTPUT << DDB2) |
           (GPIO_OUTPUT << DDB1) |
           (GPIO_INPUT  << DDB0);

    PORTB = (0 << PORTB7) |
            (0 << PORTB6) |
            (0 << PORTB5) |
            (0 << PORTB4) |
            (0 << PORTB3) |
            (0 << PORTB2) |
            (0 << PORTB1) |
            (0 << PORTB0);

    // Port C initialization
    DDRC = (GPIO_INPUT  << DDC6) |
           (GPIO_OUTPUT << DDC5) |
           (GPIO_OUTPUT << DDC4) |
           (GPIO_OUTPUT << DDC3) |
           (GPIO_OUTPUT << DDC2) |
           (GPIO_OUTPUT << DDC1) |
           (GPIO_INPUT  << DDC0);

    PORTC = (0 << PORTC6) |
            (0 << PORTC5) |
            (0 << PORTC4) |
            (0 << PORTC3) |
            (0 << PORTC2) |
            (0 << PORTC1) |
            (0 << PORTC0);

    // Port D initialization
    DDRD = (GPIO_OUTPUT << DDD7) |
           (GPIO_INPUT  << DDD6) |
           (GPIO_INPUT  << DDD5) |
           (GPIO_OUTPUT << DDD4) |
           (GPIO_INPUT  << DDD3) |
           (GPIO_OUTPUT << DDD2) |
           (GPIO_INPUT  << DDD1) |
           (GPIO_INPUT  << DDD0);

    PORTD = (0 << PORTD7) |
            (0 << PORTD6) |
            (0 << PORTD5) |
            (0 << PORTD4) |
            (0 << PORTD3) |
            (0 << PORTD2) |
            (0 << PORTD1) |
            (0 << PORTD0);
}
