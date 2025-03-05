#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include "systimer.h"
#include <util/delay.h>
#include <avr/interrupt.h>



#define MOTOR_PORT   (PORTC)
#define MOTOR_A1_PIN (3)
#define MOTOR_A2_PIN (4)
#define MOTOR_B1_PIN (0)
#define MOTOR_B2_PIN (2)
const uint8_t microstep_arr[4] = {
    ((0 << MOTOR_A1_PIN) | (1 << MOTOR_A2_PIN) | (0 << MOTOR_B1_PIN) | (1 << MOTOR_B2_PIN)),
    ((1 << MOTOR_A1_PIN) | (0 << MOTOR_A2_PIN) | (0 << MOTOR_B1_PIN) | (1 << MOTOR_B2_PIN)),
    ((1 << MOTOR_A1_PIN) | (0 << MOTOR_A2_PIN) | (1 << MOTOR_B1_PIN) | (0 << MOTOR_B2_PIN)),
    ((0 << MOTOR_A1_PIN) | (1 << MOTOR_A2_PIN) | (1 << MOTOR_B1_PIN) | (0 << MOTOR_B2_PIN)),
};
#define MOTOR_RESET (MOTOR_PORT &= ~((0 << MOTOR_A1_PIN) | (0 << MOTOR_A2_PIN) | (0 << MOTOR_B1_PIN) | (0 << MOTOR_B2_PIN)))
#define MOTOR_SET(MICROSTEP_N) MOTOR_PORT = microstep_arr[(MICROSTEP_N)]   //// |=

#define EN_SET TCCR1A |= (0x02 << COM1A0) | (0x02 << COM1B0)
#define EN_RESET TCCR1A &= ~((0x02 << COM1A0) | (0x02 << COM1B0)); \
                 PORTB &= ~((1 << 1) || (1 << 2))

#define MT_WORK_CUR_MAX (60)
#define MT_BRAKE_CUR    (50)

#define MT_CURRENT_CONTROL_THRESH_MIN_RAW (25)
#define MT_CURRENT_CONTROL_THRESH_MAX_RAW (30)  //  500 mA
#define MT_CURRENT_CONTROL_AVERAGE        (4)

uint8_t mt_microstep_cnt = 0;
uint16_t mt_pwm_a_var, mt_pwm_b_var;
uint32_t mt_adc_buff;
uint8_t mt_adc_buff_cnt;

void mt_en(void);
void mt_dis(void);
void mt_move(bool up_down, uint16_t step_qty, bool home_search_mode);
void mt_set_current(uint16_t pwm_a, uint16_t pwm_b);
void mt_current_control_reset(void);
void mt_current_control(void);




#define STEPS_IN_ROUND (763)

uint8_t round_remainder;
uint8_t round_parts;
uint16_t round_steps_in_part;
int16_t round_parts_cnt;
bool round_is_home_flag;

void round_init(uint8_t part_in_round);
void round_home(void);
void round_up(void);
void round_down(void);




#define BUTTON_DEBOUNCE_TIME_MS (100)
#define BUTTON_LONG_PRESS_TIME_MS (1000)

typedef enum {
    BT_HANDLER_STATE_UP,
    BT_HANDLER_STATE_UP_DEBOUNCE,
    BT_HANDLER_STATE_DOWN,
    BT_HANDLER_STATE_LONG_DOWN,
    BT_HANDLER_STATE_DOWN_DEBOUNCE,
} button_handler_state_t;

typedef struct {
    button_handler_state_t button_handler_state;
    timer_t debounce_timer;
    timer_t long_press_timer;
    bool is_button_press_event;
    bool is_button_long_press_event;
} button_t;

button_t start_button = {
    .button_handler_state = BT_HANDLER_STATE_UP,
    .is_button_press_event = false,
    .is_button_long_press_event = false,
};
button_t stop_button = {
    .button_handler_state = BT_HANDLER_STATE_UP,
    .is_button_press_event = false,
    .is_button_long_press_event = false,
};

bool start_button_is_press(void);
bool start_button_is_long_press(void);
bool stop_button_is_press(void);
bool stop_button_is_long_press(void);
void clear_buttons_flags(void);
void button_process(void);
void button_handler(bool is_button_down, button_t *button);



typedef enum {
    BUZZER_STATE_IDLE,
    BUZZER_STATE_BEEP_PROC,
} buzzer_process_state_t;

typedef struct {
    uint8_t  repeats_qty;
    uint8_t  durations_qty;
    uint16_t *durations_ms;
} buzzer_melody_t;

buzzer_process_state_t buzzer_process_state = BUZZER_STATE_IDLE;
timer_t buzzer_timer;
uint8_t buzzer_duration_counter;
uint8_t buzzer_repeat_counter;
uint8_t buzzer_task = 0xFF;
bool buzzer_state;

uint16_t beep_melody_durations[] = {500};
uint16_t failure_melody_durations[] = {200, 100, 200, 100, 200, 1000};
uint16_t success_start_melody_durations[] = {100, 100, 400, 20, 100};

buzzer_melody_t buzzer_melodyes[] = {
    {1, (sizeof(beep_melody_durations) / sizeof(uint16_t)), beep_melody_durations},
    {5, (sizeof(failure_melody_durations) / sizeof(uint16_t)), failure_melody_durations},
    {1, (sizeof(success_start_melody_durations) / sizeof(uint16_t)), success_start_melody_durations},
};

void buzzer_process(void);
void buzzer_short_beep(void);
void buzzer_failure_beep(void);
void buzzer_success_start_beep(void);




#define BUZZER_EN  (PORTD |= 1 << 4)
#define BUZZER_DIS (PORTD &= ~(1 << 4))

#define IND_PROCESS_EN  (PORTB |= 1 << 3)
#define IND_PROCESS_DIS (PORTB &= ~(1 << 3))

#define IND_STATUS_EN  (PORTB |= 1 << 4)
#define IND_STATUS_DIS (PORTB &= ~(1 << 4))

#define HOME_TAMPER ((PIND & (1 << 2)) >> 2)
#define START_BUTTON ((PINB & (1 << 0)) >> 0)
#define STOP_BUTTON ((PINB & (1 << 5)) >> 5)




#define IND_STATUS_BLINK_PERIOD_MS (500)
#define IND_STANDBY_BLINK_PERIOD_MS (1000)
#define IND_FAILURE_BLINK_PERIOD_MS (200)
#define AUTO_PROG_PERIOD_MS (5000)
#define STANDBY_TIMEOUT_MS (10000)    ////

typedef enum {
    PROCESS_STATE_IDLE,
    PROCESS_STATE_AUTO_PROG_PROC,
    PROCESS_STATE_STEP_PROG_PAUSE,
    PROCESS_STATE_STEP_PROG_PROC,
    PROCESS_STATE_STANDBY,
    PROCESS_STATE_FAILURE,
} prcess_state_t;

prcess_state_t prcess_state;
uint8_t mode = 2;
uint8_t modes_array[] = {(360 / 360), (360 / 180), (360 / 90), (360 / 45), (360 / 30)};
bool ind_process_state;
timer_t led_timer;
timer_t auto_prog_timer;
timer_t standby_timer;
bool is_failure = false;




#define ACTION_PREACTION_PERIOD_MS  (1000)
#define ACTION_ACTION_PERIOD_MS     (500)
#define ACTION_POSTACTION_PERIOD_MS (1000)

typedef enum {
    ACTION_PROCESS_STATE_IDLE,
    ACTION_PROCESS_STATE_PREACTION,
    ACTION_PROCESS_STATE_ACTION,
    ACTION_PROCESS_STATE_POSTACTION,
} action_process_state_t;

timer_t action_preaction_timer, action_action_timer, action_postaction_timer;
action_process_state_t action_process_state = ACTION_PROCESS_STATE_IDLE;

void action_start(void);
void action_break(void);
bool action_is_done(void);
void action_process(void);




void delay_ms(uint32_t time_ms);




int main(void) {
    // DDDn = 0 -> input
    // Port B initialization
    DDRB=(0<<DDB7) | (0<<DDB6) | (0<<DDB5) | (1<<DDB4) | (1<<DDB3) | (1<<DDB2) | (1<<DDB1) | (0<<DDB0);
    PORTB=(0<<PORTB7) | (0<<PORTB6) | (1<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (1<<PORTB0);

    // Port C initialization
    DDRC=(0<<DDC6) | (0<<DDC5) | (1<<DDC4) | (1<<DDC3) | (1<<DDC2) | (0<<DDC1) | (1<<DDC0);
    PORTC=(0<<PORTC6) | (0<<PORTC5) | (0<<PORTC4) | (0<<PORTC3) | (0<<PORTC2) | (0<<PORTC1) | (0<<PORTC0);

    // Port D initialization
    //// BR - כמלאועסÿ ןנט 1<<DDD0 -ÕÇ
    DDRD=(1<<DDD7) | (1<<DDD6) | (1<<DDD5) | (1<<DDD4) | (0<<DDD3) | (0<<DDD2) | (0<<DDD1) | (0<<DDD0);
    PORTD=(0<<PORTD7) | (0<<PORTD6) | (0<<PORTD5) | (0<<PORTD4) | (1<<PORTD3) | (1<<PORTD2) | (0<<PORTD1) | (0<<PORTD0);


    // Tim 0 init
    TCCR0 = (2 << 0);  // Clock Select: 0x00-0x05 -> 0/1/8/64/256/1024
    TIMSK |= 1 << 0;   // TOIE0 irq en


    // Tim 1 init
    #define WGM1 (14)         // 
    TCCR1A = ((WGM1 & 0b11) << WGM10)  |
             (0x02 << COM1B0) | // Toggle OC1B on Compare Match
             (0x02 << COM1A0);  // Toggle OC1A on Compare Match
    TCCR1B = (0 << ICNC1) |     // Input Capture Noise Canceler
             (0 << ICES1) |     // Input Capture Edge Select
             (((WGM1 & 0b1100) >> 2) << WGM12)  |
             (1 << CS10);       // Clock Select: 0x00-0x05 -> 0/1/8/64/256/1024
    // Interrupt Mask Register
    TIMSK |=  (0 << OCIE1B) |    // Output Compare B Match Interrupt
             (0 << OCIE1A) |    // Output Compare A Match Interrupt
             (0 << TOIE1);      // Overflow Interrupt
    #define ICR1_VAL (100)
    ICR1H = (uint8_t)(ICR1_VAL >> 8);    // order!!!
    ICR1L = (uint8_t)(ICR1_VAL >> 0);


    // ADC init
    ADCSRA = (1 << 7) | (1 << 6) | (1 << 5) | (0b111 << 0);


    sei();   // global IRQ enable  (need for timer0 IRQ!!)
    mt_en();
    round_init(modes_array[mode]);
    round_home();
    IND_STATUS_EN;
    standby_timer = systimet_set_ms(STANDBY_TIMEOUT_MS);
    prcess_state = PROCESS_STATE_IDLE;
    if (!is_failure) buzzer_success_start_beep();


    while(1) {
        switch(prcess_state) {
            case PROCESS_STATE_IDLE:
                if (is_failure) {
                    ind_process_state = false;
                    led_timer = systimet_set_ms(IND_FAILURE_BLINK_PERIOD_MS);
                    mt_dis();
                    buzzer_failure_beep();
                    prcess_state = PROCESS_STATE_FAILURE;
                }
                else if (start_button_is_press()) {
                    action_start();
                    IND_PROCESS_EN;
                    prcess_state = PROCESS_STATE_AUTO_PROG_PROC;
                }
                else if (start_button_is_long_press()) {
                    ind_process_state = false;
                    led_timer = systimet_set_ms(IND_STATUS_BLINK_PERIOD_MS);
                    prcess_state = PROCESS_STATE_STEP_PROG_PAUSE;
                }
                else if (stop_button_is_long_press()) {
                    mode++;
                    if (mode >= sizeof(modes_array)) {
                        mode = 0;
                    }
                    round_init(modes_array[mode]);
                    round_up();
                    round_down();
                    round_home();
                    standby_timer = systimet_set_ms(STANDBY_TIMEOUT_MS);
                }
                else if (systimer_triggered_ms(standby_timer)) {
                    mt_dis();
                    prcess_state = PROCESS_STATE_STANDBY;
                }
                break;


            case PROCESS_STATE_AUTO_PROG_PROC:
                action_process();
                if (action_is_done()) {
                    round_up();
                    if (round_is_home_flag) {
                        clear_buttons_flags();
                        IND_PROCESS_DIS;
                        buzzer_short_beep();
                        standby_timer = systimet_set_ms(STANDBY_TIMEOUT_MS);
                        prcess_state = PROCESS_STATE_IDLE;
                    }
                    else {
                        action_start();
                    }
                }
                else if (stop_button_is_long_press()) {
                    // Break step programm
                    action_break();
                    round_home();
                    clear_buttons_flags();
                    IND_PROCESS_DIS;
                    buzzer_short_beep();
                    standby_timer = systimet_set_ms(STANDBY_TIMEOUT_MS);
                    prcess_state = PROCESS_STATE_IDLE;
                }
                break;


            case PROCESS_STATE_STEP_PROG_PAUSE:
                if (start_button_is_press()) {
                    action_start();
                    IND_PROCESS_EN;
                    prcess_state = PROCESS_STATE_STEP_PROG_PROC;
                }
                else if (stop_button_is_press()) {
                    if (!round_is_home_flag) {
                        IND_PROCESS_EN;
                        round_down();
                        IND_PROCESS_DIS;
                        ind_process_state = false;
                        led_timer = systimet_set_ms(IND_STATUS_BLINK_PERIOD_MS);
                    }
                }
                else if (stop_button_is_long_press()) {
                    // Break step programm
                    round_home();
                    clear_buttons_flags();
                    IND_PROCESS_DIS;
                    buzzer_short_beep();
                    standby_timer = systimet_set_ms(STANDBY_TIMEOUT_MS);
                    prcess_state = PROCESS_STATE_IDLE;
                }
                else {
                    if (systimer_triggered_ms(led_timer)) {
                        if (ind_process_state) {
                            ind_process_state = false;
                            IND_PROCESS_DIS;
                        }
                        else {
                            ind_process_state = true;
                            IND_PROCESS_EN;
                        }
                        led_timer = systimet_set_ms(IND_STATUS_BLINK_PERIOD_MS);
                    } 
                }
                break;


            case PROCESS_STATE_STEP_PROG_PROC:
                action_process();
                if (action_is_done()) {
                    round_up();
                    if (round_is_home_flag) {
                        clear_buttons_flags();
                        IND_PROCESS_DIS;
                        buzzer_short_beep();
                        standby_timer = systimet_set_ms(STANDBY_TIMEOUT_MS);
                        prcess_state = PROCESS_STATE_IDLE;
                    }
                    else {
                        IND_PROCESS_DIS;
                        ind_process_state = false;
                        led_timer = systimet_set_ms(IND_STATUS_BLINK_PERIOD_MS);
                        buzzer_short_beep();
                        prcess_state = PROCESS_STATE_STEP_PROG_PAUSE;
                    }
                }
                else if (stop_button_is_long_press()) {
                    // Break step programm
                    action_break();
                    round_home();
                    clear_buttons_flags();
                    IND_PROCESS_DIS;
                    buzzer_short_beep();
                    standby_timer = systimet_set_ms(STANDBY_TIMEOUT_MS);
                    prcess_state = PROCESS_STATE_IDLE;
                }
                break;


            case PROCESS_STATE_STANDBY:
                if (start_button_is_press() || stop_button_is_press()) {
                    mt_en();
                    round_home();
                    IND_STATUS_EN;
                    standby_timer = systimet_set_ms(STANDBY_TIMEOUT_MS);
                    prcess_state = PROCESS_STATE_IDLE;
                }
                else if (systimer_triggered_ms(led_timer)) {
                    if (ind_process_state) {
                        ind_process_state = false;
                        IND_STATUS_DIS;
                    }
                    else {
                        ind_process_state = true;
                        IND_STATUS_EN;
                    }
                    led_timer = systimet_set_ms(IND_STANDBY_BLINK_PERIOD_MS);
                }
                break;


            case PROCESS_STATE_FAILURE:
                if (systimer_triggered_ms(led_timer)) {
                    if (ind_process_state) {
                        ind_process_state = false;
                        IND_STATUS_DIS;
                        IND_PROCESS_DIS;
                    }
                    else {
                        ind_process_state = true;
                        IND_STATUS_EN;
                        IND_PROCESS_EN;
                    }
                    led_timer = systimet_set_ms(IND_FAILURE_BLINK_PERIOD_MS);
                }
                break;
        }
    }
}




void mt_en(void) {
    mt_set_current(MT_WORK_CUR_MAX, MT_WORK_CUR_MAX);
    EN_SET;
    mt_microstep_cnt = 3;
    mt_move(0, 4, 0);
    mt_move(1, 4, 0);
    mt_set_current(MT_BRAKE_CUR, MT_BRAKE_CUR);
}

void mt_dis(void) {
    EN_RESET;
}

void mt_move(bool up_down, uint16_t step_qty, bool home_search_mode) {
    uint16_t step_cnt;
    uint8_t delay_time;


    delay_time = 10;
    mt_pwm_a_var = MT_WORK_CUR_MAX;
    mt_pwm_b_var = MT_WORK_CUR_MAX;
    mt_current_control_reset();

    if (home_search_mode) step_qty = STEPS_IN_ROUND;

    for (step_cnt = 0; step_cnt < step_qty; step_cnt++) {
        if (home_search_mode) {
            if (HOME_TAMPER == 1) break;
        }

        if (step_cnt > 3) {
            mt_current_control();
        }
        mt_set_current(mt_pwm_a_var, mt_pwm_b_var);

        if (up_down) {
            mt_microstep_cnt++;
            if (mt_microstep_cnt > 3) mt_microstep_cnt = 0;
        }
        else {
            if (mt_microstep_cnt == 0) mt_microstep_cnt = 3;
            else mt_microstep_cnt--;
        }
        MOTOR_SET(mt_microstep_cnt);

        if ((step_qty - step_cnt) < 5) {
            delay_time++;
        }
        else {
            if (delay_time > 5) delay_time--;
        }
        delay_ms(delay_time);
    }

    mt_set_current(MT_BRAKE_CUR, MT_BRAKE_CUR);
}

void mt_set_current(uint16_t pwm_a, uint16_t pwm_b) {
    OCR1AH = (uint8_t)(pwm_a >> 8);
    OCR1AL = (uint8_t)(pwm_a >> 0);
    OCR1BH = (uint8_t)(pwm_b >> 8);
    OCR1BL = (uint8_t)(pwm_b >> 0);
}

void mt_current_control_reset(void) {
    mt_adc_buff_cnt = 0;
    mt_adc_buff = 0;
}

void mt_current_control(void) {
    uint8_t adc_h, adc_l;
    uint16_t adc_result;
    static uint8_t sens = 0;
    static uint16_t *ocr_var = &mt_pwm_a_var;


    // order!!!
    adc_l = ADCL;
    adc_h = ADCH & 0b11;

    mt_adc_buff += ((uint16_t)adc_h << 8) | adc_l;
    mt_adc_buff_cnt++;
    if (mt_adc_buff_cnt == MT_CURRENT_CONTROL_AVERAGE) {
        adc_result = mt_adc_buff / MT_CURRENT_CONTROL_AVERAGE;
        if (adc_result > MT_CURRENT_CONTROL_THRESH_MAX_RAW) {
            if (*ocr_var > 0) (*ocr_var)--;
        }
        if (adc_result < MT_CURRENT_CONTROL_THRESH_MIN_RAW) {
            (*ocr_var)++;
        }
        mt_adc_buff = 0;
        mt_adc_buff_cnt = 0;

        sens++;
        if (sens > 1) sens = 0;
        if (sens == 0) {
            ADMUX = (3 << 6) | 0b0001;  //1
            ocr_var = &mt_pwm_a_var;
        }
        else {
            ADMUX = (3 << 6) | 0b0101;  //5
            ocr_var = &mt_pwm_b_var;
        }
    }
}




void round_init(uint8_t part_in_round) {
    round_parts = part_in_round;
    round_steps_in_part = STEPS_IN_ROUND / round_parts;
    round_remainder = STEPS_IN_ROUND - (round_steps_in_part * round_parts);
}

void round_home(void) {
    uint8_t step_cnt = 0;


    mt_move(0, 0, 1);

    do {
        mt_move(0, 1, 0);
        delay_ms(100);
        step_cnt++;

        if (step_cnt > 30) {
            is_failure = true;
            return;
        }
    } while (HOME_TAMPER == 1);

    mt_move(1, 1, 0);
    delay_ms(100);
    mt_move(1, 1, 0);
    delay_ms(100);
    mt_move(1, 1, 0);

    round_parts_cnt = 0;
    round_is_home_flag = true;
}

void round_up(void) {
    uint16_t steps_qty;


    steps_qty = round_steps_in_part;
    if (round_parts_cnt < round_remainder) steps_qty++;

    mt_move(0, steps_qty, 0);
    round_is_home_flag = false;
    round_parts_cnt++;
    if (round_parts_cnt >= round_parts) {
        round_is_home_flag = true;
        round_parts_cnt = 0;
    }
}

void round_down(void) {
    uint16_t steps_qty;


    steps_qty = round_steps_in_part;
    if (round_parts_cnt < round_remainder) steps_qty++;

    mt_move(1, steps_qty, 0);
    round_is_home_flag = false;
    round_parts_cnt--;
    if (round_parts_cnt == 0) round_is_home_flag = true;
    if (round_parts_cnt <= -1) round_parts_cnt = round_parts;
}




bool start_button_is_press(void) {
    if (start_button.is_button_press_event) {
        start_button.is_button_press_event = false;
        return true;
    }
    return false;
}

bool start_button_is_long_press(void) {
    if (start_button.is_button_long_press_event) {
        start_button.is_button_long_press_event = false;
        return true;
    }
    return false;
}

bool stop_button_is_press(void) {
    if (stop_button.is_button_press_event) {
        stop_button.is_button_press_event = false;
        return true;
    }
    return false;
}

bool stop_button_is_long_press(void) {
    if (stop_button.is_button_long_press_event) {
        stop_button.is_button_long_press_event = false;
        return true;
    }
    return false;
}

void clear_buttons_flags(void) {
    start_button.is_button_press_event = false;
    start_button.is_button_long_press_event = false;
    stop_button.is_button_press_event = false;
    stop_button.is_button_long_press_event = false;
}

void button_process(void) {
    button_handler((START_BUTTON == 0), &start_button);
    button_handler((STOP_BUTTON == 0), &stop_button);
}

void button_handler(bool is_button_down, button_t *button) {
    switch (button->button_handler_state) {
        case BT_HANDLER_STATE_UP:
            if (is_button_down) {
                button->button_handler_state = BT_HANDLER_STATE_UP_DEBOUNCE;
                button->debounce_timer = systimet_set_ms(BUTTON_DEBOUNCE_TIME_MS);
            }
            break;

        case BT_HANDLER_STATE_UP_DEBOUNCE:
            if (systimer_triggered_ms(button->debounce_timer)) {
                if (is_button_down) {
                    button->button_handler_state = BT_HANDLER_STATE_DOWN;
                    button->long_press_timer = systimet_set_ms(BUTTON_LONG_PRESS_TIME_MS);
                }
                else {
                    button->button_handler_state = BT_HANDLER_STATE_UP;
                }
            }
            break;

        case BT_HANDLER_STATE_DOWN:
            if (!is_button_down) {
                button->button_handler_state = BT_HANDLER_STATE_DOWN_DEBOUNCE;
                button->debounce_timer = systimet_set_ms(BUTTON_DEBOUNCE_TIME_MS);
            }
            else if (systimer_triggered_ms(button->long_press_timer)) {
                button->button_handler_state = BT_HANDLER_STATE_LONG_DOWN;
                button->is_button_long_press_event = true;
            }
            break;

        case BT_HANDLER_STATE_LONG_DOWN:
            if (!is_button_down) {
                button->button_handler_state = BT_HANDLER_STATE_DOWN_DEBOUNCE;
                button->debounce_timer = systimet_set_ms(BUTTON_DEBOUNCE_TIME_MS);
            }
            break;

        case BT_HANDLER_STATE_DOWN_DEBOUNCE:
            if (systimer_triggered_ms(button->debounce_timer)) {
                if (!is_button_down) {
                    if (!systimer_triggered_ms(button->long_press_timer)) button->is_button_press_event = true;
                    button->button_handler_state = BT_HANDLER_STATE_UP;
                }
                else {
                    if (!systimer_triggered_ms(button->long_press_timer)) button->button_handler_state = BT_HANDLER_STATE_DOWN;
                    else button->button_handler_state = BT_HANDLER_STATE_LONG_DOWN;
                }
            }
            break; 
    }
}




void buzzer_process(void) {
    switch (buzzer_process_state) {
        case BUZZER_STATE_IDLE:
            if (buzzer_task != 0xFF) {
                buzzer_duration_counter = 0;
                buzzer_repeat_counter = 0;
                buzzer_timer = systimet_set_ms(buzzer_melodyes[buzzer_task].durations_ms[0]);
                buzzer_process_state = BUZZER_STATE_BEEP_PROC;
                buzzer_state = true;
                BUZZER_EN;
            }
            break;


        case BUZZER_STATE_BEEP_PROC:
            if (systimer_triggered_ms(buzzer_timer)) {
                buzzer_duration_counter++;
                if (buzzer_duration_counter >= buzzer_melodyes[buzzer_task].durations_qty) {
                    buzzer_duration_counter = 0;
                    buzzer_repeat_counter++;
                    if (buzzer_repeat_counter >= buzzer_melodyes[buzzer_task].repeats_qty) {
                        buzzer_process_state = BUZZER_STATE_IDLE;
                        buzzer_task = 0xFF;
                        BUZZER_DIS;
                        return;
                    }
                }

                buzzer_timer = systimet_set_ms(buzzer_melodyes[buzzer_task].durations_ms[buzzer_duration_counter]);
                if (buzzer_state) {
                    buzzer_state = false;
                    BUZZER_DIS;
                }
                else {
                    buzzer_state = true;
                    BUZZER_EN;
                }
            }
            break;
    }
}

void buzzer_short_beep(void) {
    buzzer_task = 0;
}

void buzzer_failure_beep(void) {
    buzzer_task = 1;
}

void buzzer_success_start_beep(void) {
    buzzer_task = 2;
}




void action_start(void) {
    if (action_process_state == ACTION_PROCESS_STATE_IDLE) {
        action_preaction_timer = systimet_set_ms(ACTION_PREACTION_PERIOD_MS);
        action_process_state = ACTION_PROCESS_STATE_PREACTION;
    }
}

void action_break(void) {
    ////
    action_process_state = ACTION_PROCESS_STATE_IDLE;
}

bool action_is_done(void) {
    return (action_process_state == ACTION_PROCESS_STATE_IDLE);
}

void action_process(void) {
    switch (action_process_state) {
        case ACTION_PROCESS_STATE_IDLE:
            break;

        case ACTION_PROCESS_STATE_PREACTION:
            if (systimer_triggered_ms(action_preaction_timer)) {
                action_action_timer = systimet_set_ms(ACTION_ACTION_PERIOD_MS);
                action_process_state = ACTION_PROCESS_STATE_ACTION;

                //// action start
                IND_STATUS_DIS;
            }
            break;

        case ACTION_PROCESS_STATE_ACTION:
            if (systimer_triggered_ms(action_action_timer)) {
                action_postaction_timer = systimet_set_ms(ACTION_POSTACTION_PERIOD_MS);
                action_process_state = ACTION_PROCESS_STATE_POSTACTION;

                //// action stop
                IND_STATUS_EN;
            }
            break;

        case ACTION_PROCESS_STATE_POSTACTION:
            if (systimer_triggered_ms(action_postaction_timer)) {
                action_process_state = ACTION_PROCESS_STATE_IDLE;
            }
            break;
    }
}




void delay_ms(uint32_t time_ms) {
    timer_t timer;

    timer = systimet_set_ms(time_ms);
    while (!systimer_triggered_ms(timer)) ;
}




ISR(TIMER0_OVF_vect) {
    static uint16_t div_cnt = 0;
    static uint16_t btn_period_cnt = 0;

    div_cnt++;
    if (div_cnt >= 4) {
        systimer_process_ms();
        div_cnt = 0;
    }

    btn_period_cnt++;
    if (btn_period_cnt >= (20 * 4)) {
        buzzer_process();
        button_process();
        btn_period_cnt = 0;
    }
}