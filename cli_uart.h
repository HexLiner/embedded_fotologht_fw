#include <stdint.h>
#include <stdbool.h>

#ifndef _CLI_UART_H_
#define _CLI_UART_H_


#define UART_RX_BUFF (20)
#define UART_TX_BUFF (20)


extern uint8_t cli_uart_rx_buff[UART_RX_BUFF];
extern uint8_t cli_uart_rx_cnt;
extern uint8_t cli_uart_tx_buff[UART_TX_BUFF];
extern uint8_t cli_uart_tx_size;


extern void cli_uart_init(void);
extern bool cli_uart_is_cmd_recived(void);
extern void cli_uart_send_answer(void);


#endif  // _CLI_UART_H_
