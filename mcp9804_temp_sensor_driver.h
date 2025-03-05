#include <stdint.h>
#include <stdbool.h>

#ifndef MCP9804_TEMP_SENSOR_DRIVER_H
#define MCP9804_TEMP_SENSOR_DRIVER_H


extern void mcp9804_temp_sensor_driver_init(void);
extern bool mcp9804_temp_sensor_get_temp(uint16_t *temperature);

#endif   // MCP9804_TEMP_SENSOR_DRIVER_H
