#include <stdint.h>


#ifndef TWI_DRIVER_H
#define TWI_DRIVER_H


typedef struct {
    uint8_t slave_addr;   // slave_addr = aaaaaaa0
    uint32_t timeout_ms;
    uint8_t tx_data_qty;
    uint8_t *tx_data;
    uint8_t rx_data_max_qty;
    uint8_t *rx_data;
    uint8_t *received_data_qty;
} twi_driver_msg_t;

typedef enum {
    TWI_DRIVER_RESULT_OK            = 0,
    TWI_DRIVER_RESULT_START_ERR     = 1,
    TWI_DRIVER_RESULT_SLA_W_ERR     = 2,
    TWI_DRIVER_RESULT_DATA_TX_ERR   = 3,
    TWI_DRIVER_RESULT_SLA_R_ERR     = 4,
    TWI_DRIVER_RESULT_TIMEOUT       = 5,
} twi_driver_result_t;


extern void twi_driver_init(void);
extern twi_driver_result_t twi_driver_transmit(twi_driver_msg_t *twi_driver_msg);


#endif   // TWI_DRIVER_H
