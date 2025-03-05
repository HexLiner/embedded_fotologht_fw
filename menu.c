#include <stdint.h>
#include <stdbool.h>
#include "menu.h"
#include "encoder_driver.h"
#include "meas.h"
#include "led_driver.h"
#include "systimer.h"
#include "char1602.h"
#include "gpio_driver.h"
#include "eeprom_driver.h"
#include "error_handler.h"


#define EEPROM_FIRST_FL_PROFILE_ADDR     (0x0080)
#define EEPROM_FL_PROFILE_NAME_OFFSET    (0)
#define EEPROM_FL_PROFILE_NAME_SIZE      (14)
#define EEPROM_FL_PROFILE_CURRENT_OFFSET (EEPROM_FL_PROFILE_NAME_OFFSET + EEPROM_FL_PROFILE_NAME_SIZE)
#define EEPROM_FL_PROFILE_CURRENT_SIZE   (2)
#define EEPROM_FL_PROFILE_TIME_OFFSET    (EEPROM_FL_PROFILE_CURRENT_OFFSET + EEPROM_FL_PROFILE_CURRENT_SIZE)
#define EEPROM_FL_PROFILE_TIME_SIZE      (2)
#define EEPROM_FL_PROFILE_SIZE           (EEPROM_FL_PROFILE_NAME_SIZE + EEPROM_FL_PROFILE_CURRENT_SIZE + EEPROM_FL_PROFILE_TIME_SIZE)


#define STATUS_LED_EN  (GPIOB_SET(3))
#define STATUS_LED_DIS (GPIOB_RESET(3))
#define BUZZER_EN      (GPIOD_SET(7))
#define BUZZER_DIS     (GPIOD_RESET(7))
#define TAMPER_PIN     (GPIOB_GET(0))


typedef enum {
    MENU_STATE_INTRO = 0,
    MENU_STATE_FL_PROFILES_MENU,
    MENU_STATE_FL_PROCESS_PAUSE,
    MENU_STATE_FL_PROCESS_PREPARATION,
    MENU_STATE_FL_PROCESS,
    MENU_STATE_FL_PROCESS_DONE,
    MENU_STATE_CUSTOM_CURR_SETUP_MENU,
    MENU_STATE_CUSTOM_TIME_SETUP_MENU,
    MENU_STATE_TEST_CURR_SETUP_MENU,
    MENU_STATE_TEST_TIME_SETUP_MENU,
    MENU_STATE_FATAL_ERROR,
} menu_state_t;

static menu_state_t menu_state;
static bool fl_process_is_test;
static timer_t menu_timer;
static uint8_t lcd_string[15];
static uint8_t fl_profilse_index;

static timer_t status_led_process_timer;
static uint8_t status_led_state = 0;
static timer_t buzzer_process_timer;
static uint8_t buzzer_state = 0;

typedef enum {
    TAMPER_PROCESS_STATE_UP = 0,
    TAMPER_PROCESS_STATE_UP_DEBOUNCE,
    TAMPER_PROCESS_STATE_DOWN,
    TAMPER_PROCESS_STATE_DOWN_DEBOUNCE,
} tamper_process_state_t;
static tamper_process_state_t tamper_process_state;
static timer_t tamper_process_timer;
static bool tamper_is_pressed = false;

uint32_t eeprom_fl_profiles_qty;




static void status_led_process(void);
static void status_led_const_en(void);
static void status_led_blink_en(void);
static void status_led_dis(void);

static void buzzer_process(void);
static void buzzer_single_beep(void);
static void buzzer_ready_beep(void);
static void buzzer_dis(void);

static void tamper_process(void);

static void get_fl_profile_name_from_eeprom(uint8_t profile_index, uint8_t *name);
static void get_fl_profile_param_from_eeprom(uint8_t profile_index, uint16_t *current_pct, uint16_t *time_s);
static void change_var_value(uint16_t *var, int8_t delta, uint16_t max_var_value);
static void dig_to_string(uint16_t digit, uint8_t *string);




void menu_init(void) {
    uint8_t i, y;
    bool is_correct_profile;
    uint16_t current_pct, time_s;
    uint8_t empty_simw_cnt;


    // Check profiles in EEPROM
    for (i = 0; i <= 7; i++) {
        is_correct_profile = true;
        get_fl_profile_name_from_eeprom(i, lcd_string);
        empty_simw_cnt = 0;
        for (y = 0; y <= 14; y++) {
            if ((lcd_string[y] == ' ') || (lcd_string[y] == 0xFF)) empty_simw_cnt++;
        }
        if (empty_simw_cnt == 14) is_correct_profile = false;
        get_fl_profile_param_from_eeprom(i, &current_pct, &time_s);
        if (current_pct > 100) is_correct_profile = false;
        if (time_s < 1) is_correct_profile = false;
        if (is_correct_profile) eeprom_fl_profiles_qty++;
    }

    lcd1602_clear();
    lcd1602_move_coursor(6, 0);
    lcd1602_print_str("/E/");
    lcd1602_move_coursor(3, 1);
    lcd1602_print_str("FotoLight");
    menu_timer = systimer_set_ms(3000);
    menu_state = MENU_STATE_INTRO;
}


void menu_process(void) {
    int8_t encoder_step;
    static uint16_t setup_led_current_pct;
    static uint16_t setup_fl_process_time_s;
    static uint16_t fl_process_time_s;
    static bool is_state_init;
    uint16_t minutes, seconds;
    static uint8_t current_test_time_point, test_time_points_qty;
    static uint16_t test_time_points[6];


    status_led_process();
    buzzer_process();
    tamper_process();


    if ((eh_state != 0) && (menu_state != MENU_STATE_INTRO)) {
        is_state_init = true;
        menu_state = MENU_STATE_FATAL_ERROR;
    }


    switch (menu_state) {
        case MENU_STATE_INTRO:
            if (systimer_triggered_ms(menu_timer)) {
                fl_profilse_index = 0;
                is_state_init = true;
                menu_state = MENU_STATE_FL_PROFILES_MENU;
            }
            break;


        case MENU_STATE_FL_PROFILES_MENU:
            if (is_state_init) {
                encoder_clear_all_events();
                lcd1602_move_coursor(0, 0);
                lcd1602_print_str("Select profile: ");
            }

            encoder_step = encoder_get_step();
            if ((encoder_step != 0) || is_state_init) {
                is_state_init = false;

                if ((encoder_step > 0) && (fl_profilse_index < (eeprom_fl_profiles_qty + 2))) {
                    fl_profilse_index++;
                }
                if ((encoder_step < 0) && (fl_profilse_index > 0)) {
                    fl_profilse_index--;
                }

                lcd1602_move_coursor(0, 1);
                // Tanning - constant profile
                if (fl_profilse_index == 0) {
                    lcd1602_print_str("Tanning       ");
                    lcd1602_print_char(' ');
                    lcd1602_print_char(0x7E);   // ->
                }
                // Custom - constant profile
                else if (fl_profilse_index == (eeprom_fl_profiles_qty + 1)) {
                    lcd1602_print_str("Custom        ");
                    lcd1602_print_char(0x7F);   // <-
                    lcd1602_print_char(0x7E);   // ->
                }
                // Test - constant profile
                else if (fl_profilse_index == (eeprom_fl_profiles_qty + 2)) {
                    lcd1602_print_str("Test          ");
                    lcd1602_print_char(0x7F);   // <-
                    lcd1602_print_char(' ');
                }
                // EEPROM profiles
                else {
                    get_fl_profile_name_from_eeprom((fl_profilse_index - 1), lcd_string);
                    lcd1602_print_str((char*)lcd_string);
                    lcd1602_print_char(0x7F);   // <-
                    lcd1602_print_char(0x7E);   // ->
                }
            }

            if (encoder_is_press_event()) {
                fl_process_time_s = 0;
                fl_process_is_test = false;
                is_state_init = true;
                menu_state = MENU_STATE_FL_PROCESS_PAUSE;
                // Tanning - constant profile
                if (fl_profilse_index == 0) {
                    setup_led_current_pct = 60;
                    setup_fl_process_time_s = 10 * 60;
                }
                // Custom - constant profile
                else if (fl_profilse_index == (eeprom_fl_profiles_qty + 1)) {
                    menu_state = MENU_STATE_CUSTOM_CURR_SETUP_MENU;
                }
                // Test - constant profile
                else if (fl_profilse_index == (eeprom_fl_profiles_qty + 2)) {
                    menu_state = MENU_STATE_TEST_CURR_SETUP_MENU;
                }
                // EEPROM profiles
                else {
                    get_fl_profile_param_from_eeprom((fl_profilse_index - 1), &setup_led_current_pct, &setup_fl_process_time_s);
                }
            }
            break;


            case MENU_STATE_CUSTOM_CURR_SETUP_MENU:
                if (is_state_init) {
                    is_state_init = false;
                    setup_led_current_pct = 50;
                    setup_fl_process_time_s = 10;
                    encoder_clear_all_events();
                    lcd1602_move_coursor(0, 0);
                    lcd1602_print_str("Custom current: ");
                    lcd1602_move_coursor(0, 1);
                    lcd1602_print_str(" 50            \%");
                }

                encoder_step = encoder_get_step();
                if (encoder_step != 0) {
                    change_var_value(&setup_led_current_pct, encoder_step, 100);
                    dig_to_string(setup_led_current_pct, lcd_string);
                    lcd1602_move_coursor(0, 1);
                    lcd1602_print_str((char*)&lcd_string[2]);
                }

                if (encoder_is_long_press_event()) {
                    is_state_init = true;
                    menu_state = MENU_STATE_FL_PROFILES_MENU;
                }
                else if (encoder_is_press_event()) {
                    is_state_init = true;
                    menu_state = MENU_STATE_CUSTOM_TIME_SETUP_MENU;
                }
                break;


            case MENU_STATE_CUSTOM_TIME_SETUP_MENU:
                if (is_state_init) {
                    is_state_init = false;
                    encoder_clear_all_events();
                    lcd1602_move_coursor(0, 0);
                    lcd1602_print_str("Custom time:    ");
                    lcd1602_move_coursor(0, 1);
                    lcd1602_print_str("   10        sec");
                }

                encoder_step = encoder_get_step();
                if (encoder_step != 0) {
                    change_var_value(&setup_fl_process_time_s, encoder_step, 0xFFFF);
                    dig_to_string(setup_fl_process_time_s, lcd_string);
                    lcd1602_move_coursor(0, 1);
                    lcd1602_print_str((char*)&lcd_string[0]);
                }

                if (encoder_is_long_press_event()) {
                    is_state_init = true;
                    menu_state = MENU_STATE_FL_PROFILES_MENU;
                }
                else if (encoder_is_press_event()) {
                    fl_process_time_s = 0;
                    fl_process_is_test = false;
                    is_state_init = true;
                    menu_state = MENU_STATE_FL_PROCESS_PAUSE;
                }
                break;


            case MENU_STATE_TEST_CURR_SETUP_MENU:
                if (is_state_init) {
                    is_state_init = false;
                    setup_led_current_pct = 50;
                    setup_fl_process_time_s = 10;
                    encoder_clear_all_events();
                    lcd1602_move_coursor(0, 0);
                    lcd1602_print_str("Test current:   ");
                    lcd1602_move_coursor(0, 1);
                    lcd1602_print_str(" 50            \%");
                }

                encoder_step = encoder_get_step();
                if (encoder_step != 0) {
                    change_var_value(&setup_led_current_pct, encoder_step, 100);
                    dig_to_string(setup_led_current_pct, lcd_string);
                    lcd1602_move_coursor(0, 1);
                    lcd1602_print_str((char*)&lcd_string[2]);
                }

                if (encoder_is_long_press_event()) {
                    is_state_init = true;
                    menu_state = MENU_STATE_FL_PROFILES_MENU;
                }
                else if (encoder_is_press_event()) {
                    is_state_init = true;
                    menu_state = MENU_STATE_TEST_TIME_SETUP_MENU;
                }
                break;


            case MENU_STATE_TEST_TIME_SETUP_MENU:
                if (is_state_init) {
                    is_state_init = false;
                    encoder_clear_all_events();
                    lcd1602_move_coursor(0, 0);
                    lcd1602_print_str("Test t points:  ");
                    lcd1602_move_coursor(0, 1);
                    lcd1602_print_str("t1/6    1    sec");

                    current_test_time_point = 0;
                    test_time_points_qty = 0;
                    test_time_points[0] = 1;
                }

                encoder_step = encoder_get_step();
                if (encoder_step != 0) {
                    change_var_value(&test_time_points[current_test_time_point], encoder_step, 0xFFFF);
                    lcd1602_move_coursor(4, 1);
                    if ((current_test_time_point == 0) && (test_time_points[current_test_time_point] == 0)) test_time_points[current_test_time_point] = 1;
                    if (test_time_points[current_test_time_point] == 0) {
                        lcd1602_print_str(" skip");
                    }
                    else {
                        dig_to_string(test_time_points[current_test_time_point], lcd_string);
                        lcd1602_print_str((char*)&lcd_string[0]);
                    }
                }

                if (encoder_is_long_press_event()) {
                    is_state_init = true;
                    menu_state = MENU_STATE_FL_PROFILES_MENU;
                }
                else if (encoder_is_press_event()) {
                    if ((test_time_points[current_test_time_point] == 0) || (current_test_time_point >= 6)) {
                        if (test_time_points[current_test_time_point] > 0) test_time_points_qty++;
                        current_test_time_point = 0;
                        setup_fl_process_time_s = test_time_points[0];
                        fl_process_time_s = 0;
                        fl_process_is_test = true;
                        is_state_init = true;
                        menu_state = MENU_STATE_FL_PROCESS_PAUSE;
                    }
                    else {
                        test_time_points_qty++;
                        current_test_time_point++;
                        test_time_points[current_test_time_point] = 0;
                        lcd1602_move_coursor(0, 1);
                        lcd1602_print_str("t1/6 skip    sec");
                        lcd1602_move_coursor(1, 1);
                        lcd1602_print_char('1' + current_test_time_point);
                    }
                }
                break;


            case MENU_STATE_FL_PROCESS_PAUSE:
                if (is_state_init) {
                    is_state_init = false;
                    encoder_clear_all_events();

                    if (fl_process_is_test) {
                        lcd1602_move_coursor(0, 0);
                        lcd1602_print_str("Test point   ");
                        lcd1602_print_char('1' + current_test_time_point);
                        lcd1602_print_char('/');
                        lcd1602_print_char('0' + test_time_points_qty);
                        
                    }
                    else {
                        lcd1602_move_coursor(0, 0);
                        lcd1602_print_str("Fl process      ");
                    }

                    lcd1602_move_coursor(0, 1);
                    minutes = fl_process_time_s / 60;
                    seconds = minutes * 60;
                    seconds = fl_process_time_s - seconds;
                    dig_to_string(minutes, lcd_string);
                    lcd1602_print_str((char*)&lcd_string[2]);
                    lcd1602_print_char('m');
                    dig_to_string(seconds, lcd_string);
                    lcd1602_print_str((char*)&lcd_string[3]);
                    lcd1602_print_str("s/");

                    minutes = setup_fl_process_time_s / 60;
                    seconds = minutes * 60;
                    seconds = setup_fl_process_time_s - seconds;
                    dig_to_string(minutes, lcd_string);
                    lcd1602_print_str((char*)&lcd_string[2]);
                    lcd1602_print_char('m');
                    dig_to_string(seconds, lcd_string);
                    lcd1602_print_str((char*)&lcd_string[3]);
                    lcd1602_print_char('s');
                }

                if (encoder_is_long_press_event()) {
                    is_state_init = true;
                    menu_state = MENU_STATE_FL_PROFILES_MENU;
                }
                else if (tamper_is_pressed) {
                    menu_timer = systimer_set_ms(2 * 1000);
                    is_state_init = true;
                    menu_state = MENU_STATE_FL_PROCESS_PREPARATION;
                }
                break;


            case MENU_STATE_FL_PROCESS_PREPARATION:
                if (systimer_triggered_ms(menu_timer)) {
                    is_state_init = true;
                    menu_state = MENU_STATE_FL_PROCESS;
                }
                if (!tamper_is_pressed) {
                    is_state_init = true;
                    menu_state = MENU_STATE_FL_PROCESS_PAUSE;
                }
                break;


            case MENU_STATE_FL_PROCESS:
                if (is_state_init) {
                    is_state_init = false;
                    encoder_clear_all_events();

                    ////db
                    lcd1602_move_coursor(0, 1);
                    lcd1602_print_str("Process         ");

                    menu_timer = systimer_set_ms(1000);
                    ////dbg led_current_pct = setup_led_current_pct;
                    status_led_const_en();
                    buzzer_single_beep();
                }

                if (!tamper_is_pressed) {
                    led_current_pct = 0;
                    status_led_dis();
                    buzzer_single_beep();
                    is_state_init = true;
                    menu_state = MENU_STATE_FL_PROCESS_PAUSE;
                }
                else if (systimer_triggered_ms(menu_timer)) {
                    fl_process_time_s++;
                    if (fl_process_time_s >= setup_fl_process_time_s) {
                        led_current_pct = 0;
                        is_state_init = true;
                        menu_state = MENU_STATE_FL_PROCESS_DONE;
                    }
                    else {
                        menu_timer = systimer_set_ms(1000);
                    }
                }
                break;


            case MENU_STATE_FL_PROCESS_DONE:
                if (is_state_init) {
                    is_state_init = false;
                    encoder_clear_all_events();

                    status_led_blink_en();
                    buzzer_ready_beep();
                    menu_timer = systimer_set_ms(15 * 1000);

                    ////dbg
                    lcd1602_move_coursor(0, 1);
                    lcd1602_print_str("Done            ");
                }

                if (systimer_triggered_ms(menu_timer)) buzzer_dis();
                if (!tamper_is_pressed) {
                    status_led_dis();
                    buzzer_dis();

                    is_state_init = true;
                    if (fl_process_is_test) {
                        current_test_time_point++;
                        if (current_test_time_point >= test_time_points_qty) {
                            menu_state = MENU_STATE_FL_PROFILES_MENU;
                        }
                        else {
                            fl_process_time_s = 0;
                            setup_fl_process_time_s = test_time_points[current_test_time_point];
                            menu_state = MENU_STATE_FL_PROCESS_PAUSE;
                        }
                    }
                    else {
                        menu_state = MENU_STATE_FL_PROFILES_MENU;
                    }
                }
                break;


            case MENU_STATE_FATAL_ERROR:
                if (is_state_init) {
                    is_state_init = false;
                    lcd1602_clear();
                    lcd1602_move_coursor(0, 0);
                    lcd1602_print_str("Fatal error!");
                    lcd1602_move_coursor(0, 1);
                    if (eh_state & EH_STATUS_FLAG_FW_ERR) lcd1602_print_str("FW ");
                    if (eh_state & EH_STATUS_FLAG_LCD_DCDC_OVERVOLTAGE_ERR) lcd1602_print_str("OVRV ");
                    if (eh_state & EH_STATUS_FLAG_LCD_DCDC_OVERCURRENT_ERR) lcd1602_print_str("OVRC");
                }
                break;


            default:
                eh_state |= EH_STATUS_FLAG_FW_ERR;
                is_state_init = true;
                menu_state = MENU_STATE_FATAL_ERROR;
                break;
    }
}




static void status_led_process(void) {
    if (status_led_state == 0) return;
    if (!systimer_triggered_ms(status_led_process_timer)) return;
    if (status_led_state == 1) {
        STATUS_LED_EN;
        status_led_state++;
    }
    else {
        STATUS_LED_DIS;
        status_led_state = 1;
    }
    status_led_process_timer = systimer_set_ms(1000);
}


static void status_led_const_en(void) {
    STATUS_LED_EN;
    status_led_state = 0;
}


static void status_led_blink_en(void) {
    status_led_process_timer = 0;
    status_led_state = 1;
}


static void status_led_dis(void) {
    STATUS_LED_DIS;
    status_led_state = 0;
}


static void buzzer_process(void) {
    if (buzzer_state == 0) return;
    if (!systimer_triggered_ms(buzzer_process_timer)) return;
    if (buzzer_state == 1) {
        BUZZER_DIS;
        buzzer_state = 0;
    }
    else if (buzzer_state == 2) {
        buzzer_state++;
        BUZZER_EN;
    }
    else {
        buzzer_state = 2;
        BUZZER_DIS;
    }
    buzzer_process_timer = systimer_set_ms(1000);
}


static void buzzer_single_beep(void) {
    BUZZER_EN;
    buzzer_process_timer = systimer_set_ms(200);
    buzzer_state = 1;
}


static void buzzer_ready_beep(void) {
    buzzer_process_timer = 0;
    buzzer_state = 2;
}


static void buzzer_dis(void) {
    BUZZER_DIS;
    buzzer_state = 0;
}


static void tamper_process(void) {
    switch (tamper_process_state) {
        case TAMPER_PROCESS_STATE_UP:
            if (TAMPER_PIN == 0) {
                tamper_process_timer = systimer_set_ms(100);
                tamper_process_state = TAMPER_PROCESS_STATE_UP_DEBOUNCE;
            }
            break;

        case TAMPER_PROCESS_STATE_UP_DEBOUNCE:
            if (systimer_triggered_ms(tamper_process_timer)) {
                if (TAMPER_PIN == 0) {
                    tamper_is_pressed = true;
                    tamper_process_state = TAMPER_PROCESS_STATE_DOWN;
                }
                else {
                    tamper_process_state = TAMPER_PROCESS_STATE_UP;
                }
            }
            break;

        case TAMPER_PROCESS_STATE_DOWN:
            if (TAMPER_PIN == 1) {
                tamper_process_timer = systimer_set_ms(100);
                tamper_process_state = TAMPER_PROCESS_STATE_DOWN_DEBOUNCE;
            }
            break;

        case TAMPER_PROCESS_STATE_DOWN_DEBOUNCE:
            if (systimer_triggered_ms(tamper_process_timer)) {
                if (TAMPER_PIN == 1) {
                    tamper_is_pressed = false;
                    tamper_process_state = TAMPER_PROCESS_STATE_UP;
                }
                else {
                    tamper_process_state = TAMPER_PROCESS_STATE_DOWN;
                }
            }
            break;
    }
}


static void get_fl_profile_name_from_eeprom(uint8_t profile_index, uint8_t *name) {
    uint16_t addr;


    addr = EEPROM_FL_PROFILE_SIZE;
    addr = addr * profile_index;
    addr += EEPROM_FIRST_FL_PROFILE_ADDR + EEPROM_FL_PROFILE_NAME_OFFSET;
    eeprom_driver_read(addr, EEPROM_FL_PROFILE_NAME_SIZE, name);
    name[14] = '\0';
}


static void get_fl_profile_param_from_eeprom(uint8_t profile_index, uint16_t *current_pct, uint16_t *time_s) {
    uint16_t addr_b, addr;


    addr_b = EEPROM_FL_PROFILE_SIZE;
    addr_b = addr_b * profile_index;
    addr_b += EEPROM_FIRST_FL_PROFILE_ADDR;
    addr = addr_b + EEPROM_FL_PROFILE_CURRENT_OFFSET;
    eeprom_driver_read_16(addr, current_pct);
    addr = addr_b + EEPROM_FL_PROFILE_TIME_OFFSET;
    eeprom_driver_read_16(addr, time_s);
}


static void change_var_value(uint16_t *var, int8_t delta, uint16_t max_var_value) {
    if (delta > 0) {
        if ((max_var_value - *var) >= delta) *var += delta;
        else *var = max_var_value;
    }
    if (delta < 0) {
        delta = -delta;
        if (delta <= *var) *var -= delta;
        else *var = 0;
    }
}


static void dig_to_string(uint16_t digit, uint8_t *string) {
    uint16_t tmp;


    tmp = digit / 10000;
    string[0] = tmp + 0x30;
    if (tmp == 0) string[0] = 0x20;

    tmp = tmp * 10000;
    digit -= tmp;
    tmp = digit / 1000;
    string[1] = tmp + 0x30;
    if ((tmp == 0) && (string[0] == 0x20)) string[1] = 0x20;

    tmp = tmp * 1000;
    digit -= tmp;
    tmp = digit / 100;
    string[2] = tmp + 0x30;
    if ((tmp == 0) && (string[1] == 0x20)) string[2] = 0x20;

    tmp = tmp * 100;
    digit -= tmp;
    tmp = digit / 10;
    string[3] = tmp + 0x30;
    if ((tmp == 0) && (string[2] == 0x20)) string[3] = 0x20;

    tmp = tmp * 10;
    digit -= tmp;
    string[4] = digit + 0x30;

    string[5] = '\0';
}
