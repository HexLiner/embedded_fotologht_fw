#ifndef _ENCODER_DRIVER_H_
#define _ENCODER_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>


extern void encoder_init(void);
extern void encoder_process(void);

extern void encoder_clear_all_events(void);
extern int8_t encoder_get_step(void);
extern bool encoder_is_press_event(void);
extern bool encoder_is_double_press_event(void);
extern bool encoder_is_long_press_event(void);


#endif   // _ENCODER_DRIVER_H_
