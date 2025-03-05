#include <stdint.h>
#include <stdbool.h>
#include "mcp9804_temp_sensor_driver.h"
#include "twi_driver.h"


#define MCP9804_SLAVE_ADDR (0b01001000 << 1)

#define MCP9804_TEMPERATURE_REG (0x00)
#define MCP9804_CONFIG_REG      (0x01)
    #define MCP9804_CONFIG_REG_SD_POS  (0)  // Shutdown Mode
    #define MCP9804_CONFIG_REG_TM_POS  (1)  // Thermostat Mode
    #define MCP9804_CONFIG_REG_POL_POS (2)  // Polarity
    #define MCP9804_CONFIG_REG_F0_POS  (3)  // Fault Queue
    #define MCP9804_CONFIG_REG_R0_POS  (5)  // Converter Resolution
    #define MCP9804_CONFIG_REG_OS_POS  (7)  // One-Shot


static uint8_t mcp9804_tx_data_buff[3];
static uint8_t mcp9804_rx_data_buff[2];
static twi_driver_msg_t mcp9804_twi_driver_msg;




void mcp9804_temp_sensor_driver_init(void) {
    twi_driver_init();

    mcp9804_twi_driver_msg.slave_addr = MCP9804_SLAVE_ADDR;
    mcp9804_twi_driver_msg.timeout_ms = 1000;
    mcp9804_twi_driver_msg.tx_data = mcp9804_tx_data_buff;
    mcp9804_twi_driver_msg.rx_data = mcp9804_rx_data_buff;


    mcp9804_twi_driver_msg.tx_data_qty = 3;
    mcp9804_tx_data_buff[0] = MCP9804_CONFIG_REG;
    mcp9804_tx_data_buff[1] = 0;
    mcp9804_tx_data_buff[2] = 4 << MCP9804_CONFIG_REG_R0_POS;  // 12bit(0.0625)
    mcp9804_twi_driver_msg.rx_data_max_qty = 0;
    twi_driver_transmit(&mcp9804_twi_driver_msg);
}


bool mcp9804_temp_sensor_get_temp(uint16_t *temperature_c) {
    mcp9804_twi_driver_msg.tx_data_qty = 1;
    mcp9804_tx_data_buff[0] = MCP9804_TEMPERATURE_REG;
    mcp9804_twi_driver_msg.rx_data_max_qty = 2;
    if (twi_driver_transmit(&mcp9804_twi_driver_msg) != TWI_DRIVER_RESULT_OK) return 1;

    *temperature_c = mcp9804_rx_data_buff[0];
    *temperature_c = *temperature_c << 4;
    *temperature_c |= mcp9804_rx_data_buff[1] >> 4;
    return 0;
}
