#include "cli_uart.h"
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>


#define UART_SRC_CLOCK_HZ (8000000)
#define UART_BAUD         (9600)
#define UART_BRR          (UART_SRC_CLOCK_HZ / 16 / (UART_BAUD - 1))
#define UART_CHAR_SIZE    (0b011)   // 8-bit


typedef enum {
    CLI_UART_STATE_IDLE = 0,
    CLI_UART_STATE_RX_PROC,
    CLI_UART_STATE_RX_READY,
    CLI_UART_STATE_TX_PROC,
} cli_uart_state_t;


uint8_t cli_uart_rx_buff[UART_RX_BUFF];
uint8_t cli_uart_rx_cnt;
uint8_t cli_uart_tx_buff[UART_TX_BUFF];
uint8_t cli_uart_tx_size;


static uint8_t cli_uart_tx_cnt;
static cli_uart_state_t cli_uart_state = CLI_UART_STATE_IDLE;




void cli_uart_init(void) {
    // Set baud rate
    #ifdef __AVR_ATmega8__
    UBRRH = (uint8_t)(UART_BRR >> 8);
    UBRRL = (uint8_t)UART_BRR;
    #else
    UBRR0H = (uint8_t)(UART_BRR >> 8);
    UBRR0L = (uint8_t)UART_BRR;
    #endif

    #ifdef __AVR_ATmega8__
    UCSRA = (1 << UDRE) | (0 << U2X);
    UCSRB = (1 << RXCIE) |  // RX Complete Interrupt Enable
            (1 << TXCIE) |  // TX Complete Interrupt Enable
            (0 << UDRIE) |  // USART Data Register Empty Interrupt Enable
            (1 << RXEN)  |  // RX Enable
            (1 << TXEN)  |  // TX Enable
            ((UART_CHAR_SIZE >> 2) << UCSZ2) |  // Character Size
            (0 << TXB8);    // Transmit Data Bit 8
    UCSRC =  (1 << URSEL)  |  // !!!!!!!!!!!!!!!!! a8 only
             (0 << UMSEL)  |  // USART Mode Select (0 - Asynchronous USART)
             (0 << UPM0)   |  // Parity Mode (0 - Disabled)
             (1 << USBS)   |  // Stop Bit Select (1 - 2-bit)
             ((UART_CHAR_SIZE & 0b11) << UCSZ0) |  // Character Size
             (0 << UCPOL);    // Clock Polarity
    #else
    // Enable receiver and transmitter
    UCSR0B = (1 << RXCIE0) |  // RX Complete Interrupt Enable
             (1 << TXCIE0) |  // TX Complete Interrupt Enable
             (0 << UDRIE0) |  // USART Data Register Empty Interrupt Enable
             (1 << RXEN0)  |  // RX Enable
             (1 << TXEN0)  |  // TX Enable
             ((UART_CHAR_SIZE >> 2) << UCSZ02) |  // Character Size
             (0 << TXB80);    // Transmit Data Bit 8
    // Set frame format: 8data, 2stop bit
    UCSR0C = (0 << UMSEL00) |  // USART Mode Select (0 - Asynchronous USART)
             (0 << UPM00)   |  // Parity Mode (0 - Disabled)
             (1 << USBS0)   |  // Stop Bit Select (1 - 2-bit)
             ((UART_CHAR_SIZE & 0b11) << UCSZ00) |  // Character Size
             (0 << UCPOL0);    // Clock Polarity
    #endif
}


bool cli_uart_is_cmd_recived(void) {
    if (cli_uart_state == CLI_UART_STATE_RX_READY) return true;
    return false;
}

void cli_uart_send_answer(void) {
    //if (cli_uart_state != CLI_UART_STATE_RX_READY) return;

    cli_uart_state = CLI_UART_STATE_TX_PROC;
    cli_uart_tx_cnt = 0;
    #ifdef __AVR_ATmega8__
    UDR = cli_uart_tx_buff[cli_uart_tx_cnt];
    #else
    UDR0 = cli_uart_tx_buff[cli_uart_tx_cnt];
    #endif
}



#ifdef __AVR_ATmega8__
ISR(USART_TXC_vect) {
#else
ISR(USART_TX_vect) {
#endif
    if (cli_uart_state != CLI_UART_STATE_TX_PROC) return;

    cli_uart_tx_cnt++;
    if (cli_uart_tx_cnt < cli_uart_tx_size) {
        #ifdef __AVR_ATmega8__
        UDR = cli_uart_tx_buff[cli_uart_tx_cnt];
        #else
        UDR0 = cli_uart_tx_buff[cli_uart_tx_cnt];
        #endif
    }
    else {
        cli_uart_state = CLI_UART_STATE_IDLE;
    }
}


#ifdef __AVR_ATmega8__
ISR(USART_RXC_vect) {
#else
ISR (USART_RX_vect) {
#endif
    uint8_t rx_data;


    #ifdef __AVR_ATmega8__
    rx_data = UDR;
    #else
    rx_data = UDR0;
    #endif

    if (cli_uart_state == CLI_UART_STATE_TX_PROC) return;

    if (rx_data == 127) {
        if (cli_uart_rx_cnt > 0) {
            cli_uart_rx_cnt--;
            #ifdef __AVR_ATmega8__
            UDR = rx_data;  // echo
            #else
            UDR0 = rx_data;  // echo
            #endif
        }
        return;
    }
    #ifdef __AVR_ATmega8__
    if (rx_data != '\r') UDR = rx_data;  // ech
    #else
    if (rx_data != '\r') UDR0 = rx_data;  // echo
    #endif
    
    if (rx_data == '\r') {
        cli_uart_state = CLI_UART_STATE_RX_READY;
    }
    else if ((cli_uart_rx_cnt < UART_RX_BUFF) && (cli_uart_state != CLI_UART_STATE_RX_READY)) {
        cli_uart_rx_buff[cli_uart_rx_cnt] = rx_data;
        cli_uart_rx_cnt++;
        cli_uart_state = CLI_UART_STATE_RX_PROC;
    }
    else {
        cli_uart_state = CLI_UART_STATE_RX_READY;
    }
}
