#include "seg_led_driver.h"
#include <stdint.h>
#include <stdbool.h>
#include "systimer.h"
#include "gpio_driver.h"


#define BLINK_PERIOD_MS (300)


#define DIG_MODE_INDEX (3)
#define DIG_0_INDEX    (0)
#define DIG_1_INDEX    (1)
#define DIG_2_INDEX    (2)


#define SEG_LED_MOSI_SET   (GPIOD_SET(6))
#define SEG_LED_MOSI_RESET (GPIOD_RESET(6))

#define SEG_LED_SCK_SET    (GPIOB_SET(0))
#define SEG_LED_SCK_RESET  (GPIOB_RESET(0))

#define SEG_LED_CS_SET     (GPIOD_SET(7))
#define SEG_LED_CS_RESET   (GPIOD_RESET(7))

#define SEG_LED_DIG1_SET   (GPIOB_SET(3))
#define SEG_LED_DIG1_RESET (GPIOB_RESET(3))
#define SEG_LED_DIG2_SET   (GPIOB_SET(4))
#define SEG_LED_DIG2_RESET (GPIOB_RESET(4))
#define SEG_LED_DIG3_SET   (GPIOB_SET(2))
#define SEG_LED_DIG3_RESET (GPIOB_RESET(2))
#define SEG_LED_DIG4_SET   (GPIOB_SET(5))
#define SEG_LED_DIG4_RESET (GPIOB_RESET(5))


#define SEG_A_POS (7)
#define SEG_B_POS (3)
#define SEG_C_POS (2)
#define SEG_D_POS (0)
#define SEG_E_POS (1)
#define SEG_F_POS (6)
#define SEG_G_POS (4)
#define SEG_H_POS (5)

static const uint8_t seg_led_alphabet[] = {
    ((1 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),  // 0
    ((0 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),  // 1
    ((1 << SEG_A_POS) | (1 << SEG_B_POS) | (0 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (0 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS)),  // 2
    ((1 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS)),  // 3
    ((0 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS)),  // 4
    ((1 << SEG_A_POS) | (0 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (0 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS)),  // 5
    ((1 << SEG_A_POS) | (0 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS)),  // 6
    ((1 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),  // 7
    ((1 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS)),  // 8
    ((1 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (0 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS)),  // 9
    ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),  // 
};


typedef struct {
    uint8_t seg_led_animation_frame_dig_mode_index;
    uint8_t seg_led_animation_frame_dig_0;
    uint8_t seg_led_animation_frame_dig_1;
    uint8_t seg_led_animation_frame_dig_2;
} seg_led_animation_frame_t;
typedef struct {
    uint8_t seg_led_animation_frames_qty;
    seg_led_animation_frame_t *seg_led_animation_frames;
    uint16_t *seg_led_animation_frames_duration_ms;
} seg_led_animation_t;

static seg_led_animation_frame_t seg_led_animation_frames_intro[5] = {
    // all on
    {
        .seg_led_animation_frame_dig_mode_index = (1 << SEG_A_POS) | (1 << SEG_D_POS) | (1 << SEG_G_POS),
        .seg_led_animation_frame_dig_0 = ((1 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (1 << SEG_H_POS)),
        .seg_led_animation_frame_dig_1 = ((1 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (1 << SEG_H_POS)),
        .seg_led_animation_frame_dig_2 = ((1 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (1 << SEG_H_POS)),
    },
    // all off
    {
        .seg_led_animation_frame_dig_mode_index = (0 << SEG_A_POS) | (0 << SEG_D_POS) | (0 << SEG_G_POS),
        .seg_led_animation_frame_dig_0 = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
        .seg_led_animation_frame_dig_1 = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
        .seg_led_animation_frame_dig_2 = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
    },
    //   ||  
    {
        .seg_led_animation_frame_dig_mode_index = (0 << SEG_A_POS) | (0 << SEG_D_POS) | (0 << SEG_G_POS),
        .seg_led_animation_frame_dig_0 = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
        .seg_led_animation_frame_dig_1 = ((0 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (0 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
        .seg_led_animation_frame_dig_2 = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
    },
    //  |  | 
    {
        .seg_led_animation_frame_dig_mode_index = (0 << SEG_A_POS) | (0 << SEG_D_POS) | (0 << SEG_G_POS),
        .seg_led_animation_frame_dig_0 = ((0 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
        .seg_led_animation_frame_dig_1 = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
        .seg_led_animation_frame_dig_2 = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
    },
    // | E | 
    {
        .seg_led_animation_frame_dig_mode_index = (0 << SEG_A_POS) | (0 << SEG_D_POS) | (0 << SEG_G_POS),
        .seg_led_animation_frame_dig_0 = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
        .seg_led_animation_frame_dig_1 = ((1 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS)),
        .seg_led_animation_frame_dig_2 = ((0 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS)),
    }
};
static uint16_t seg_led_animation_frames_duration_ms_intro[5] = {
    1000,
    500,
    200,
    200,
    2000
};
static seg_led_animation_t seg_led_animation_intro = {
    .seg_led_animation_frames_qty = 5,
    .seg_led_animation_frames = seg_led_animation_frames_intro,
    .seg_led_animation_frames_duration_ms = seg_led_animation_frames_duration_ms_intro
};


static bool is_blink_en, is_blink_mask_active;
static timer_t process_timer, blink_timer;
static uint8_t seg_led_buff[4];

static bool is_animation_process;
static seg_led_animation_t *animation_ptr;
static uint8_t animation_frame_index;
static timer_t animation_timer;




void seg_led_init(void) {
    is_blink_en = false;
    is_animation_process = false;
}


void seg_led_process(void) {
    static uint8_t active_digit = 0;
    uint8_t tx_data, bit;


    if (!systimer_triggered_ms(process_timer)) return;
    process_timer = systimer_set_ms(1);


    if (is_animation_process) {
        if (systimer_triggered_ms(animation_timer)) {
            if (animation_frame_index < animation_ptr->seg_led_animation_frames_qty) {
                seg_led_buff[DIG_0_INDEX] = animation_ptr->seg_led_animation_frames[animation_frame_index].seg_led_animation_frame_dig_0;
                seg_led_buff[DIG_1_INDEX] = animation_ptr->seg_led_animation_frames[animation_frame_index].seg_led_animation_frame_dig_1;
                seg_led_buff[DIG_2_INDEX] = animation_ptr->seg_led_animation_frames[animation_frame_index].seg_led_animation_frame_dig_2;
                seg_led_buff[DIG_MODE_INDEX] = animation_ptr->seg_led_animation_frames[animation_frame_index].seg_led_animation_frame_dig_mode_index;
                animation_timer = systimer_set_ms((uint32_t)animation_ptr->seg_led_animation_frames_duration_ms[animation_frame_index]);
                animation_frame_index++;
            }
            else {
                is_animation_process = false;
            }
        }
    }
    else if (is_blink_en) {
        if (systimer_triggered_ms(blink_timer)) {
            if (is_blink_mask_active) is_blink_mask_active = false;
            else is_blink_mask_active = true;
            blink_timer = systimer_set_ms(BLINK_PERIOD_MS);
        }
    }


    SEG_LED_CS_RESET;

    tx_data = seg_led_buff[active_digit];
    if (is_blink_mask_active) tx_data = 0;
    for(bit = 1; bit != 0; bit = bit << 1) {
        SEG_LED_SCK_RESET;
        if (tx_data & bit) SEG_LED_MOSI_SET;
        else SEG_LED_MOSI_RESET;

        SEG_LED_SCK_SET;
    }

    SEG_LED_MOSI_RESET;

    SEG_LED_DIG1_RESET;
    SEG_LED_DIG2_RESET;
    SEG_LED_DIG3_RESET;
    SEG_LED_DIG4_RESET;

    SEG_LED_CS_SET;    // show new image

    if (active_digit == 0) SEG_LED_DIG1_SET;
    if (active_digit == 1) SEG_LED_DIG2_SET;
    if (active_digit == 2) SEG_LED_DIG3_SET;
    if (active_digit == 3) SEG_LED_DIG4_SET;


    active_digit++;
    if (active_digit > 3) active_digit = 0;
}


void seg_led_blink_en(void) {
    is_blink_en = true;
    is_blink_mask_active = false;
    blink_timer = systimer_set_ms(BLINK_PERIOD_MS);
}

void seg_led_blink_dis(void) {
    is_blink_en = false;
    is_blink_mask_active = false;
}


void seg_led_print_dig(uint16_t digit) {
    uint16_t tmp;


    if (digit <= 999) {
        tmp = digit / 100;
        seg_led_buff[DIG_0_INDEX] = seg_led_alphabet[tmp];
        if (tmp == 0) seg_led_buff[DIG_0_INDEX] = 0;

        tmp = tmp * 100;
        digit -= tmp;
        tmp = digit / 10;
        seg_led_buff[DIG_1_INDEX] = seg_led_alphabet[tmp];
        if ((tmp == 0) && (seg_led_buff[DIG_0_INDEX] == 0)) seg_led_buff[DIG_1_INDEX] = 0;

        tmp = tmp * 10;
        digit -= tmp;
        seg_led_buff[DIG_2_INDEX] = seg_led_alphabet[digit];
    }
}


void seg_led_clear_all_fn(void) {
    seg_led_buff[DIG_MODE_INDEX] = 0;
}


void seg_led_set_fn_fun(void) {
    ///seg_led_buff[DIG_MODE_INDEX] &= ~(1 << SEG_D_POS);
    seg_led_buff[DIG_MODE_INDEX] = 1 << SEG_D_POS;
}


void seg_led_set_fn_param(void) {
    ///seg_led_buff[DIG_MODE_INDEX] &= ~(1 << SEG_A_POS);
    seg_led_buff[DIG_MODE_INDEX] = 1 << SEG_A_POS;
}


void seg_led_set_fn_heat(void) {
    ///seg_led_buff[DIG_MODE_INDEX] &= ~(1 << SEG_G_POS);
    seg_led_buff[DIG_MODE_INDEX] = 1 << SEG_G_POS;
}


void seg_led_clr(void) {
    seg_led_buff[DIG_0_INDEX] = 0;
    seg_led_buff[DIG_1_INDEX] = 0;
    seg_led_buff[DIG_2_INDEX] = 0;
    seg_led_buff[DIG_MODE_INDEX] = 0;
}




void seg_led_print_error(uint8_t error_code) {
    seg_led_print_dig(error_code);
    seg_led_buff[DIG_0_INDEX] = ((1 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (1 << SEG_H_POS));  // E.
    seg_led_buff[DIG_MODE_INDEX] = 0;
}


void seg_led_print_parameter(uint8_t parameter_code) {
    seg_led_print_dig(parameter_code);
    seg_led_buff[DIG_0_INDEX] = ((1 << SEG_A_POS) | (1 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (1 << SEG_H_POS));  // P.
}


void seg_led_print_intro(void) {
    /*
    seg_led_buff[DIG_0_INDEX] = ((1 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS));  // /
    seg_led_buff[DIG_1_INDEX] = ((1 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS));  // E
    seg_led_buff[DIG_2_INDEX] = ((0 << SEG_A_POS) | (1 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS));  // /
    seg_led_buff[DIG_MODE_INDEX] = 0;
    */

    is_animation_process = true;
    animation_ptr = &seg_led_animation_intro;
    animation_frame_index = 0;
    animation_timer = 0;
}

void seg_led_print_cool(void) {
    seg_led_buff[DIG_0_INDEX] = ((1 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS));  // C
    seg_led_buff[DIG_1_INDEX] = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (0 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS));  // o
    seg_led_buff[DIG_2_INDEX] = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS));  // L
    seg_led_buff[DIG_MODE_INDEX] = 0;
}

void seg_led_print_off(void) {
    seg_led_buff[DIG_0_INDEX] = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (0 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS));  // o
    seg_led_buff[DIG_1_INDEX] = ((1 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (0 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS));  // F
    seg_led_buff[DIG_2_INDEX] = seg_led_buff[DIG_1_INDEX];
    seg_led_buff[DIG_MODE_INDEX] = 0;
}

void seg_led_print_sleep(void) {
    seg_led_buff[DIG_0_INDEX] = ((1 << SEG_A_POS) | (0 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (0 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS));  // S
    seg_led_buff[DIG_1_INDEX] = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS));  // t
    seg_led_buff[DIG_2_INDEX] = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (1 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (1 << SEG_G_POS) | (0 << SEG_H_POS));  // b
    seg_led_buff[DIG_MODE_INDEX] = 0;
}

void seg_led_print_cli(void) {
    seg_led_buff[DIG_0_INDEX] = ((1 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS));  // C
    seg_led_buff[DIG_1_INDEX] = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (0 << SEG_C_POS) | (1 << SEG_D_POS) | (1 << SEG_E_POS) | (1 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS));  // L
    seg_led_buff[DIG_2_INDEX] = ((0 << SEG_A_POS) | (0 << SEG_B_POS) | (1 << SEG_C_POS) | (0 << SEG_D_POS) | (0 << SEG_E_POS) | (0 << SEG_F_POS) | (0 << SEG_G_POS) | (0 << SEG_H_POS));  // i
    seg_led_buff[DIG_MODE_INDEX] = 0;
}
