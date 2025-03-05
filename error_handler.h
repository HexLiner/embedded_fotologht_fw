#include <stdint.h>


#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H


#define EH_STATUS_FLAG_FW_ERR                   (1 << 0)
#define EH_STATUS_FLAG_LCD_DCDC_OVERVOLTAGE_ERR (1 << 1)
#define EH_STATUS_FLAG_LCD_DCDC_OVERCURRENT_ERR (1 << 2)


extern uint8_t eh_state;


#endif   // ERROR_HANDLING_H
