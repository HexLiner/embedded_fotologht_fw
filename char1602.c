#include "char1602.h"
#include <stdint.h>
#include <stdbool.h>
#include "gpio_driver.h"
#include "systimer.h"
#include <util/delay.h>


// Drive pins
#define LCD1602_RS_SET   (GPIOD_SET(4))
#define LCD1602_RS_RESET (GPIOD_RESET(4))
#define LCD1602_RW_SET   (GPIOD_SET(2))
#define LCD1602_RW_RESET (GPIOD_RESET(2))
#define LCD1602_E_SET    (GPIOC_SET(1))
#define LCD1602_E_RESET  (GPIOC_RESET(1))


// Select parallel interface type
#define FOUR_LINES_ON_PORT_CONSISTENTLY_UP_TO_DOWN
//// other interface type...


// Interface pins
#ifdef FOUR_LINES_ON_PORT_CONSISTENTLY_UP_TO_DOWN
    #define INTERF_D4_SET      (GPIOC_SET(2))
    #define INTERF_D4_RESET    (GPIOC_RESET(2))
    #define INTERF_D4_GET      (GPIOC_GET(2))
    #define INTERF_D4_IN_MODE  (GPIOC_MODE_INPUT(2))
    #define INTERF_D4_OUT_MODE (GPIOC_MODE_OUTPUT(2))
    #define INTERF_D5_SET      (GPIOC_SET(3))
    #define INTERF_D5_RESET    (GPIOC_RESET(3))
    #define INTERF_D5_GET      (GPIOC_GET(3))
    #define INTERF_D5_IN_MODE  (GPIOC_MODE_INPUT(3))
    #define INTERF_D5_OUT_MODE (GPIOC_MODE_OUTPUT(3))
    #define INTERF_D6_SET      (GPIOC_SET(4))
    #define INTERF_D6_RESET    (GPIOC_RESET(4))
    #define INTERF_D6_GET      (GPIOC_GET(4))
    #define INTERF_D6_IN_MODE  (GPIOC_MODE_INPUT(4))
    #define INTERF_D6_OUT_MODE (GPIOC_MODE_OUTPUT(4))
    #define INTERF_D7_SET      (GPIOC_SET(5))
    #define INTERF_D7_RESET    (GPIOC_RESET(5))
    #define INTERF_D7_GET      (GPIOC_GET(5))
    #define INTERF_D7_IN_MODE  (GPIOC_MODE_INPUT(5))
    #define INTERF_D7_OUT_MODE (GPIOC_MODE_OUTPUT(5))
#endif
//// other interface type...


#define CLEAR_DISPLAY                    (0x01)
#define RETURN_HOME                      (0x02)
#define ENTRY_MODE_SET                   (0x04)
    #define ENTRY_MODE_SET_I_D_POS       (1)    // I/D=1: Increment Mode; I/D=0: Decrement Mode
    #define ENTRY_MODE_SET_SH_POS        (0)    // S=1: Shift
#define DISPLAY_CONTROL                  (0x08)      
    #define DISPLAY_CONTROL_D_POS        (2)    // display on/off
    #define DISPLAY_CONTROL_C_POS        (1)    // cursor on/off
    #define DISPLAY_CONTROL_B_POS        (0)    // blink on/off
#define CURSOR_DISPLAY_SHIFT             (0x10)
    #define CURSOR_DISPLAY_SHIFT_S_C_POS (3)    // S/C=1: Display Shift; S/C=0: Cursor Shift
    #define CURSOR_DISPLAY_SHIFT_R_L_POS (2)    // R/L=1: Right Shift; R/L=0: Left Shift
#define FUNCTION_SET                     (0x20)
    #define FUNCTION_SET_DL_POS          (4)    // DL=1: 8D   DL=0: 4D
    #define FUNCTION_SET_N_POS           (3)    // N=1: 2R     N=0: 1R
    #define FUNCTION_SET_F_POS           (2)    // F=1: 5x10 Style;     F=0: 5x7 Style
#define SET_GRAM_ADDR                    (0x40)
#define SET_DDRAM_ADDR                   (0x80)


#define LCD1602_STROB() LCD1602_E_SET;   \
                        _delay_us(20);   \
                        LCD1602_E_RESET; \


static void lcd1602_init_write(uint8_t data);
static void lcd1602_write(uint8_t data, bool is_data);
static bool lcd1602_is_busy(void);
static void lcd1602_set_data_input_mode(void);
static void lcd1602_set_data_output_mode(void);



void lcd1602_init() {
    LCD1602_RS_RESET;
    LCD1602_RW_RESET;
    INTERF_D4_RESET,
    INTERF_D5_RESET,
    INTERF_D6_RESET,
    INTERF_D7_RESET,
    LCD1602_E_RESET;

    systimer_delay_ms(50);

    LCD1602_STROB();
    LCD1602_STROB();
    LCD1602_STROB();

    //lcd1602_write(RETURN_HOME, 0);
    INTERF_D5_SET,
    LCD1602_STROB();

    systimer_delay_ms(15);
    lcd1602_init_write(FUNCTION_SET | (0 << FUNCTION_SET_DL_POS) | (1 << FUNCTION_SET_N_POS) | (0 << FUNCTION_SET_F_POS));
    systimer_delay_ms(15);
    lcd1602_init_write(FUNCTION_SET | (0 << FUNCTION_SET_DL_POS) | (1 << FUNCTION_SET_N_POS) | (0 << FUNCTION_SET_F_POS));
    systimer_delay_ms(15);
    lcd1602_init_write(DISPLAY_CONTROL | (1 << DISPLAY_CONTROL_D_POS) | (0 << DISPLAY_CONTROL_C_POS) | (0 << DISPLAY_CONTROL_B_POS));
    systimer_delay_ms(15);
    lcd1602_init_write(DISPLAY_CONTROL | (1 << DISPLAY_CONTROL_D_POS) | (0 << DISPLAY_CONTROL_C_POS) | (0 << DISPLAY_CONTROL_B_POS));
    systimer_delay_ms(15);
    lcd1602_init_write(CLEAR_DISPLAY);
    systimer_delay_ms(15);
    lcd1602_init_write(ENTRY_MODE_SET | (1 << ENTRY_MODE_SET_I_D_POS) | (0 << ENTRY_MODE_SET_SH_POS));
    systimer_delay_ms(15);
}


void lcd1602_clear() {
    lcd1602_write(CLEAR_DISPLAY, 0);
}


void lcd1602_move_coursor(uint8_t x, uint8_t y) {
    uint8_t address;


    address = (y * 0x40) + x;
    lcd1602_write(SET_DDRAM_ADDR | address, 0);
    _delay_us(50);
}


void lcd1602_print_char(char data) {
    lcd1602_write(data, 1);
}


void lcd1602_print_str(const char *data) {
    uint8_t i;


    for (i = 0; data[i] != '\0'; i++) {
        lcd1602_write(data[i], 1);
    }       
}


void lcd1602_coursor_on(void) {
    lcd1602_write(DISPLAY_CONTROL | (1 << DISPLAY_CONTROL_D_POS) | (1 << DISPLAY_CONTROL_C_POS) | (1 << DISPLAY_CONTROL_B_POS), 0);
}


void lcd1602_coursor_off(void) {
    lcd1602_write(DISPLAY_CONTROL | (1 << DISPLAY_CONTROL_D_POS) | (0 << DISPLAY_CONTROL_C_POS) | (0 << DISPLAY_CONTROL_B_POS), 0);
}




static void lcd1602_init_write(uint8_t data) {
    if (data & (1 << 4)) INTERF_D4_SET;
    else INTERF_D4_RESET;
    if (data & (1 << 5)) INTERF_D5_SET;
    else INTERF_D5_RESET;
    if (data & (1 << 6)) INTERF_D6_SET;
    else INTERF_D6_RESET;
    if (data & (1 << 7)) INTERF_D7_SET;
    else INTERF_D7_RESET;
    LCD1602_STROB();

    if (data & (1 << 0)) INTERF_D4_SET;
    else INTERF_D4_RESET;
    if (data & (1 << 1)) INTERF_D5_SET;
    else INTERF_D5_RESET;
    if (data & (1 << 2)) INTERF_D6_SET;
    else INTERF_D6_RESET;
    if (data & (1 << 3)) INTERF_D7_SET;
    else INTERF_D7_RESET;
    LCD1602_STROB();
}


static void lcd1602_write(uint8_t data, bool is_data) {
    lcd1602_set_data_input_mode();
    while (lcd1602_is_busy()) ;
    lcd1602_set_data_output_mode();

    if (is_data) {
        LCD1602_RS_SET;
        _delay_us(20);
    }

    lcd1602_init_write(data);

    if (is_data) {
        LCD1602_RS_RESET;
        _delay_us(20);
    }
}


static bool lcd1602_is_busy(void) {
    bool is_busy = false;


    LCD1602_STROB();
    is_busy = INTERF_D7_GET;
    LCD1602_STROB();
    return is_busy;
}


static void lcd1602_set_data_input_mode(void) {
    LCD1602_RW_SET;
    _delay_us(20);
    INTERF_D7_IN_MODE;
    INTERF_D6_IN_MODE;
    INTERF_D5_IN_MODE;
    INTERF_D4_IN_MODE;
}


static void lcd1602_set_data_output_mode(void) {
    LCD1602_RW_RESET;
    _delay_us(20);
    INTERF_D7_OUT_MODE;
    INTERF_D6_OUT_MODE;
    INTERF_D5_OUT_MODE;
    INTERF_D4_OUT_MODE;
}
