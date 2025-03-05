#include <stdint.h>
#include "registers.h"


#define DEVICE_REG_QTY (2)
typedef struct {

} device_registers_t;


device_registers_t device_registers;
uint8_t *device_registers_prt = (uint8_t*)&device_registers;
