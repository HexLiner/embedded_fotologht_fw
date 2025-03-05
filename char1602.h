#ifndef _LCD1602_H_
#define _LCD1602_H_

#include <stdint.h>


extern void lcd1602_init();
extern void lcd1602_clear();
extern void lcd1602_move_coursor(uint8_t x, uint8_t y);
extern void lcd1602_print_char(char data);
extern void lcd1602_print_str(const char *data);
extern void lcd1602_coursor_on(void);
extern void lcd1602_coursor_off(void);

#endif // _LCD1602_H_