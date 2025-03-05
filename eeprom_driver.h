#include <stdint.h>
#include <avr/io.h>

#ifndef EEPROM_DRIVER_H
#define EEPROM_DRIVER_H


extern void eeprom_driver_read_8(uint16_t addr, uint8_t *data);
extern void eeprom_driver_read_16(uint16_t addr, uint16_t *data);
extern void eeprom_driver_read(uint16_t addr, uint8_t data_qty, uint8_t *data);
extern void eeprom_driver_write_8(uint16_t addr, uint8_t data);
extern void eeprom_driver_write_16(uint16_t addr, uint16_t data);
extern void eeprom_driver_write(uint16_t addr, uint8_t data_qty, const uint8_t *data);


#endif   // EEPROM_DRIVER_H
