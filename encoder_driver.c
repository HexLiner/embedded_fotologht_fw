
#include <stdint.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include "encoder_driver.h"
#include "systimer.h"
#include "gpio_driver.h"


#define ENC_BTN_PIN_STATE   (GPIOD_GET(5))
#define ENC_CODE_PIN_STATE  (GPIOD_GET(6))
#define ENC_WAIT_TIMEOUT_EN (1)
#define ENC_WAIT_TIMEOUT_MS (100)
#define ENC_ACC_TIMEOUT_MS  (150)
#define ENC_ACC_STEP        (10)

#define BUTTON_DEBOUNCE_TIME_MS     (100)
#define BUTTON_DOUBLE_PRESS_TIME_MS (700)
#define BUTTON_LONG_PRESS_TIME_MS   (3000)


typedef enum {
    ENC_BT_HANDLER_STATE_UP,
    ENC_BT_HANDLER_STATE_UP_DEBOUNCE,
    ENC_BT_HANDLER_STATE_DOWN,
    ENC_BT_HANDLER_STATE_LONG_DOWN,
    ENC_BT_HANDLER_STATE_DOWN_DEBOUNCE,
} enc_btn_handler_state_t;

typedef enum {
    ENC_STATE_IDLE = 0,
    ENC_STATE_EN,
    ENC_STATE_DEC,
    ENC_STATE_DEBOUNCE,
} enc_state_t;

typedef enum {
    ENC_BTN_EVENT_NULL = 0,
    ENC_BTN_EVENT_PRESS,
    ENC_BTN_EVENT_DOUBLE_PRESS,
    ENC_BTN_EVENT_LONG_PRESS,
} enc_btn_event_t;


static enc_btn_handler_state_t enc_btn_handler_state;
static timer_t enc_btn_debounce_timer = 0;
static timer_t enc_btn_double_press_timer = 0;
static timer_t enc_btn_long_press_timer = 0;
static enc_btn_event_t enc_btn_event;
static bool is_double_click_proc;

volatile enc_state_t enc_state;
static timer_t enc_acceleration_timer = 0;
#if (ENC_WAIT_TIMEOUT_EN == 1)
static timer_t enc_wait_timer;
#endif
static int8_t enc_step;



void encoder_init(void) {
    enc_btn_handler_state = ENC_BT_HANDLER_STATE_UP;
    enc_btn_event = ENC_BTN_EVENT_NULL;
    is_double_click_proc = false;

    enc_state = ENC_STATE_IDLE;
    enc_step = 0;

    #ifdef __AVR_ATmega8__
    MCUCR |= (0b11 << ISC10) |   // INT1 Sense Control: The rising edge of INT1 generates an interrupt request.
             (0b00 << ISC00);    // INT0 Sense Control: The rising edge of INT0 generates an interrupt request.
    GIFR = (0 << INTF1) |       // External Interrupt Flag Register
           (0 << INTF0);
    GICR = (1 << INT1) |       // External Interrupt Mask Register
           (0 << INT0);
    #else
    EICRA = (0b11 << ISC10) |   // INT1 Sense Control: The rising edge of INT1 generates an interrupt request.
            (0b00 << ISC00);    // INT0 Sense Control: The rising edge of INT0 generates an interrupt request.
    EIFR = (0 << INTF1) |       // External Interrupt Flag Register
           (0 << INTF0);
    EIMSK = (1 << INT1) |       // External Interrupt Mask Register
            (0 << INT0);
    #endif
}


void encoder_process(void) {
    bool is_enc_btn_down;


    is_enc_btn_down = !ENC_BTN_PIN_STATE;
    switch (enc_btn_handler_state) {
        case ENC_BT_HANDLER_STATE_UP:
            if (is_enc_btn_down) {
                enc_btn_handler_state = ENC_BT_HANDLER_STATE_UP_DEBOUNCE;
                enc_btn_debounce_timer = systimer_set_ms(BUTTON_DEBOUNCE_TIME_MS);
                is_double_click_proc = true;
                if (systimer_triggered_ms(enc_btn_double_press_timer)) {
                    is_double_click_proc = false;   
                    enc_btn_double_press_timer = systimer_set_ms(BUTTON_DOUBLE_PRESS_TIME_MS);
                }
            }
            break;

        case ENC_BT_HANDLER_STATE_UP_DEBOUNCE:
            if (systimer_triggered_ms(enc_btn_debounce_timer)) {
                if (is_enc_btn_down) {
                    enc_btn_handler_state = ENC_BT_HANDLER_STATE_DOWN;
                    enc_btn_long_press_timer = systimer_set_ms(BUTTON_LONG_PRESS_TIME_MS);
                }
                else {
                    enc_btn_handler_state = ENC_BT_HANDLER_STATE_UP;
                }
            }
            break;

        case ENC_BT_HANDLER_STATE_DOWN:
            if (!is_enc_btn_down) {
                enc_btn_handler_state = ENC_BT_HANDLER_STATE_DOWN_DEBOUNCE;
                enc_btn_debounce_timer = systimer_set_ms(BUTTON_DEBOUNCE_TIME_MS);
            }
            else if (systimer_triggered_ms(enc_btn_long_press_timer)) {
                enc_btn_handler_state = ENC_BT_HANDLER_STATE_LONG_DOWN;
                enc_btn_event = ENC_BTN_EVENT_LONG_PRESS;
            }
            break;

        case ENC_BT_HANDLER_STATE_LONG_DOWN:
            if (!is_enc_btn_down) {
                enc_btn_handler_state = ENC_BT_HANDLER_STATE_DOWN_DEBOUNCE;
                enc_btn_debounce_timer = systimer_set_ms(BUTTON_DEBOUNCE_TIME_MS);
            }
            break;

        case ENC_BT_HANDLER_STATE_DOWN_DEBOUNCE:
            if (systimer_triggered_ms(enc_btn_debounce_timer)) {
                if (!is_enc_btn_down) {
                    if (!systimer_triggered_ms(enc_btn_long_press_timer)) {
                        ////if (!systimer_triggered_ms(enc_btn_double_press_timer) && is_double_click_proc) enc_btn_event = ENC_BTN_EVENT_DOUBLE_PRESS;
                        ////else enc_btn_event = ENC_BTN_EVENT_PRESS;
                        enc_btn_event = ENC_BTN_EVENT_PRESS;
                    }
                    enc_btn_handler_state = ENC_BT_HANDLER_STATE_UP;
                }
                else {
                    if (!systimer_triggered_ms(enc_btn_long_press_timer)) enc_btn_handler_state = ENC_BT_HANDLER_STATE_DOWN;
                    else enc_btn_handler_state = ENC_BT_HANDLER_STATE_LONG_DOWN;
                }
            }
            break; 
    }


    switch (enc_state) {
        case ENC_STATE_EN:
            if (systimer_triggered_ms(enc_acceleration_timer)) enc_step = -1;
            else enc_step = -ENC_ACC_STEP;
            enc_acceleration_timer = systimer_set_ms(ENC_ACC_TIMEOUT_MS);
#if (ENC_WAIT_TIMEOUT_EN == 1)
            enc_state = ENC_STATE_DEBOUNCE;
#else
            enc_state = ENC_STATE_IDLE;
#endif
            break;

        case ENC_STATE_DEC:
            if (systimer_triggered_ms(enc_acceleration_timer)) enc_step = 1;
            else enc_step = ENC_ACC_STEP;
            enc_acceleration_timer = systimer_set_ms(ENC_ACC_TIMEOUT_MS);
#if (ENC_WAIT_TIMEOUT_EN == 1)
            enc_state = ENC_STATE_DEBOUNCE;
#else
            enc_state = ENC_STATE_IDLE;
#endif
            break;

#if (ENC_WAIT_TIMEOUT_EN == 1)
        case ENC_STATE_DEBOUNCE:
            if (systimer_triggered_ms(enc_wait_timer)) {
                enc_state = ENC_STATE_IDLE;
            }
            break;
#endif

        default:
            break;
    }

}


void encoder_clear_all_events(void) {
    enc_btn_event = ENC_BTN_EVENT_NULL;
    enc_step = 0;
}


int8_t encoder_get_step(void) {
    int8_t result = enc_step;
    enc_step = 0;
    return result;
}


bool encoder_is_press_event(void) {
    if (enc_btn_event == ENC_BTN_EVENT_PRESS) {
        enc_btn_event = ENC_BTN_EVENT_NULL;
        return true;
    }
    return false;
}


bool encoder_is_double_press_event(void) {
    if (enc_btn_event == ENC_BTN_EVENT_DOUBLE_PRESS) {
        enc_btn_event = ENC_BTN_EVENT_NULL;
        return true;
    }
    return false;
}


bool encoder_is_long_press_event(void) {
    if (enc_btn_event == ENC_BTN_EVENT_LONG_PRESS) {
        enc_btn_event = ENC_BTN_EVENT_NULL;
        return true;
    }
    return false;
}




ISR(INT1_vect) {
    #ifdef __AVR_ATmega8__
    GIFR = 0;   // Clear flags
    #else
    EIFR = 0;   // Clear flags
    #endif

    if (enc_state == ENC_STATE_DEBOUNCE) return;

    if (ENC_CODE_PIN_STATE) enc_state = ENC_STATE_DEC;
    else enc_state = ENC_STATE_EN;

#if (ENC_WAIT_TIMEOUT_EN == 1)
    enc_wait_timer = systimer_set_ms(ENC_WAIT_TIMEOUT_MS);
#endif
}
