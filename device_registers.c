#include <stdint.h>
#include <stdbool.h>
#include "device_registers.h"
#include "error_handler.h"
#include "gpio_driver.h"   ////dbg


uint16_t drvice_reg_enc_test_0 = 100;   ////dbg
uint16_t drvice_reg_enc_test_1 = 0;   ////dbg

static uint8_t drvice_reg_cmd = 0;


// BE - HHLL
uint8_t *device_registers_ptr[DEVICE_RAM_REG_QTY] = {
    // 0
    (uint8_t*)&drvice_reg_cmd,
    // 1
    (uint8_t*)&eh_state,
    // 2
    (uint8_t*)&drvice_reg_enc_test_0 + 1,
    (uint8_t*)&drvice_reg_enc_test_0 + 0,
    // 4
    (uint8_t*)&drvice_reg_enc_test_1 + 1,
    (uint8_t*)&drvice_reg_enc_test_1 + 0,
};


void drvice_registers_proc(void) {
    switch (drvice_reg_cmd) {
        case 0:
            break;
        
        case 1:
            break;

        case 2:
            break;

        default:
            break;
    }
    drvice_reg_cmd = 0;
}
