#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "twi_driver.h"
#include "systimer.h"




typedef enum {
    TWI_DRIVER_STATUS_START_TX_OK = 0x08,
    TWI_DRIVER_STATUS_RPT_START_TX_OK = 0x10,
    TWI_DRIVER_STATUS_SLA_W_TX_ACK = 0x18,
    TWI_DRIVER_STATUS_DATA_TX_ACK = 0x28,
    TWI_DRIVER_STATUS_SLA_R_TX_ACK = 0x40,
    TWI_DRIVER_STATUS_DATA_RX_ACK = 0x50,
} twi_driver_status_t;






// slave_addr = aaaaaaa0
void twi_driver_init(void) {
    // SCLfreq = COREfreq / (16 + 2 * TWBR * presc)
    // TWBR = ((COREfreq / SCLfreq) - 16) / (2 * presc)
    // TWBR = ((8000000 / 100000) - 16) / (2 * 1) = 32
    TWBR = 32;
    TWSR = (0 << TWPS0);  // Prescaler Value 1/4/16/64
    TWCR = (1 << TWEN);
}


twi_driver_result_t twi_driver_transmit(twi_driver_msg_t *twi_driver_msg) {
    twi_driver_result_t twi_driver_result = TWI_DRIVER_RESULT_OK;
    uint8_t i;
    timer_t transaction_timer;


    transaction_timer = systimer_set_ms(twi_driver_msg->timeout_ms);

    // Send START condition
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    // Wait for TWINT Flag set. This indicates that the START condition has been transmitted
    while (!(TWCR & (1 << TWINT))) {
        if (systimer_triggered_ms(transaction_timer)) {
            twi_driver_result = (twi_driver_result_t)(0x80 | 1);
            *twi_driver_msg->received_data_qty = TWSR;
            goto twi_driver_err;
        }
    }
    // Check value of TWI Status Register. Mask prescaler bits. If status different from START go to ERROR
    if ((TWSR & 0xF8) != TWI_DRIVER_STATUS_START_TX_OK) {
        twi_driver_result = TWI_DRIVER_RESULT_START_ERR;
        *twi_driver_msg->received_data_qty = TWSR;
        goto twi_driver_err;
    }

    // Send data
    if (twi_driver_msg->tx_data_qty != 0) {
        // Send slave addr + W
        TWDR = twi_driver_msg->slave_addr | 0;
        TWCR = (1 << TWINT) | (1 << TWEN);
        while (!(TWCR & (1 << TWINT))) {
            if (systimer_triggered_ms(transaction_timer)) {
                twi_driver_result = (twi_driver_result_t)(0x80 | 2);
                *twi_driver_msg->received_data_qty = TWSR;
                goto twi_driver_err;
            }
        }
        if ((TWSR & 0xF8) != TWI_DRIVER_STATUS_SLA_W_TX_ACK)  {
            twi_driver_result = TWI_DRIVER_RESULT_SLA_W_ERR;
            *twi_driver_msg->received_data_qty = TWSR;
            goto twi_driver_err;
        }

        for (i = 0; i < twi_driver_msg->tx_data_qty; i++) {
            TWDR = twi_driver_msg->tx_data[i];
            TWCR = (1 << TWINT) | (1 << TWEN);
            while (!(TWCR & (1 << TWINT))) {
                if (systimer_triggered_ms(transaction_timer)) {
                    twi_driver_result = (twi_driver_result_t)(0x80 | 3);
                    *twi_driver_msg->received_data_qty = TWSR;
                    goto twi_driver_err;
                }
            }
            if ((TWSR & 0xF8) != TWI_DRIVER_STATUS_DATA_TX_ACK)  {
                twi_driver_result = TWI_DRIVER_RESULT_DATA_TX_ERR;
                *twi_driver_msg->received_data_qty = TWSR;
                goto twi_driver_err;
            }
        }
    }

    if ((twi_driver_msg->tx_data_qty != 0) && (twi_driver_msg->rx_data_max_qty != 0)) {
        // Send repeat START condition
        TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
        while (!(TWCR & (1 << TWINT))) {
            if (systimer_triggered_ms(transaction_timer)) {
                twi_driver_result = (twi_driver_result_t)(0x80 | 4);
                *twi_driver_msg->received_data_qty = TWSR;
                goto twi_driver_err;
            }
        }
        if ((TWSR & 0xF8) != TWI_DRIVER_STATUS_RPT_START_TX_OK) {
            twi_driver_result = TWI_DRIVER_RESULT_START_ERR;
            *twi_driver_msg->received_data_qty = TWSR;   ////
            goto twi_driver_err;
        }
    }

    // Receive data
    if (twi_driver_msg->rx_data_max_qty != 0) {
        // Send slave addr + R
        TWDR = twi_driver_msg->slave_addr | 1;
        TWCR = (1 << TWINT) | (1 << TWEN);
        while (!(TWCR & (1 << TWINT))) {
            if (systimer_triggered_ms(transaction_timer)) {
                twi_driver_result = (twi_driver_result_t)(0x80 | 5);
                *twi_driver_msg->received_data_qty = TWSR;
                goto twi_driver_err;
            }
        }
        if ((TWSR & 0xF8) != TWI_DRIVER_STATUS_SLA_R_TX_ACK)  {
            twi_driver_result = TWI_DRIVER_RESULT_SLA_R_ERR;
            *twi_driver_msg->received_data_qty = TWSR;
            goto twi_driver_err;
        }

        for (i = 0; i < twi_driver_msg->rx_data_max_qty; i++) {
            if (i == (twi_driver_msg->rx_data_max_qty - 1)) TWCR = (1 << TWINT) | (1 << TWEN);
            else TWCR = (1 << TWEA) | (1 << TWINT) | (1 << TWEN);
            while (!(TWCR & (1 << TWINT))) {
                if (systimer_triggered_ms(transaction_timer)) {
                    twi_driver_result = (twi_driver_result_t)(0x80 | 6);
                    *twi_driver_msg->received_data_qty = TWSR;
                    goto twi_driver_err;
                }
            }
            twi_driver_msg->rx_data[i] = TWDR;
            if ((TWSR & 0xF8) != TWI_DRIVER_STATUS_DATA_RX_ACK) break;
        }

        *twi_driver_msg->received_data_qty = i;
    }


twi_driver_err:
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);   // Transmit STOP condition
    return twi_driver_result;
}
