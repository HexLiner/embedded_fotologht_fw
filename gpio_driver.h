#include <avr/io.h>


#ifndef _GPIO_DRIVER_H_
#define _GPIO_DRIVER_H_


#define GPIOB_SET(pin_n) PORTB |= 1 << (pin_n)
#define GPIOB_SET_GR(pins_msk) PORTB |= (pins_msk)
#define GPIOB_RESET(pin_n) PORTB &= ~(1 << (pin_n))
#define GPIOB_RESET_GR(pins_msk) PORTB &= ~(pins_msk)
#define GPIOB_GET(pin_n) (PINB >> (pin_n)) & 0x01
#define GPIOB_GET_GR(pins_msk) PINB & (pins_msk)
#define GPIOB_MODE_OUTPUT(pin_n) DDRB |= (1 << pin_n)
#define GPIOB_MODE_INPUT(pin_n) DDRB &= ~(1 << pin_n)

#define GPIOC_SET(pin_n) PORTC |= 1 << (pin_n)
#define GPIOC_SET_GR(pins_msk) PORTC |= (pins_msk)
#define GPIOC_RESET(pin_n) PORTC &= ~(1 << (pin_n))
#define GPIOC_RESET_GR(pins_msk) PORTC &= ~(pins_msk)
#define GPIOC_GET(pin_n) (PINC >> (pin_n)) & 0x01
#define GPIOC_GET_GR(pins_msk) PINC & (pins_msk)
#define GPIOC_MODE_OUTPUT(pin_n) DDRC |= (1 << pin_n)
#define GPIOC_MODE_INPUT(pin_n) DDRC &= ~(1 << pin_n)

#define GPIOD_SET(pin_n) PORTD |= 1 << (pin_n)
#define GPIOD_SET_GR(pins_msk) PORTD |= (pins_msk)
#define GPIOD_RESET(pin_n) PORTD &= ~(1 << (pin_n))
#define GPIOD_RESET_GR(pins_msk) PORTD &= ~(pins_msk)
#define GPIOD_GET(pin_n) (PIND >> (pin_n)) & 0x01
#define GPIOD_GET_GR(pins_msk) PIND & (pins_msk)
#define GPIOD_MODE_OUTPUT(pin_n) DDRD |= (1 << pin_n)
#define GPIOD_MODE_INPUT(pin_n) DDRD &= ~(1 << pin_n)


extern void gpio_init(void);


#endif  // _GPIO_DRIVER_H_
