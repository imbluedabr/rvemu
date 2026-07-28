#include "uart.h"
#include "debug.h"
#include <unistd.h>
#include <sys/fcntl.h>

static int SimpleUART_read(struct Bus* bus, uint32_t address, void* buff, uint32_t n) {
    SimpleUART* uart = (SimpleUART*) bus;
    if (address == 0x00 && n == 1) { //DATA register
        uint8_t temp = uart->rx_tail;
        if (temp != uart->tx_head) {
            uart->rx_tail = (temp + 1) & RXMASK;
            *(char*) buff = uart->rxbuff[temp];
            return 0;
        }
    } else if (address == 0x01 && n == 1) { //CSR
        uint8_t csr = 0;
        if (uart->rx_tail != uart->tx_head) {
            csr |= UART_RXAVAIL;
        }
        if (((uart->tx_head + 1) & TXMASK) == uart->tx_tail) {
            csr |= UART_TXFULL;
        }
        *(uint8_t*) buff = csr;
        return 0;
    }
    return -1;
}

static int SimpleUART_write(struct Bus* bus, uint32_t address, void* buff, uint32_t n) {
    SimpleUART* uart = (SimpleUART*) bus;
    if (address == 0x00 && n == 1) {
        uint8_t temp = uart->tx_head;
        if (((temp + 1) & TXMASK) != uart->tx_tail) {
            uart->tx_head = (temp + 1) & TXMASK;
            uart->txbuff[temp] = *(char*) buff;
            return 0;
        }
    } else if (address == 0x01 && n == 1) {
        return 0;
    }
    return -1;
}


void SimpleUART_init(SimpleUART* uart, MainBus* _MainBus) {
    uart->_BusDevice._MainBus = _MainBus;
    uart->_BusDevice._Bus.read = SimpleUART_read;
    uart->_BusDevice._Bus.write = SimpleUART_write;
    uart->_BusDevice.type = DEV_SERIAL;
    uart->rx_head = 0;
    uart->rx_tail = 0;
    uart->tx_head = 0;
    uart->tx_tail = 0;

    struct termios newt;

    tcgetattr(STDIN_FILENO, &uart->oldt);      // get current settings
    newt = uart->oldt;

    cfmakeraw(&newt);                    // put terminal in raw mode

    newt.c_cc[VMIN]  = 0;                // return immediately
    newt.c_cc[VTIME] = 0;                // no timeout

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void SimpleUART_tick(SimpleUART* uart) {
    uint8_t buff;
    uint8_t tmp;
    if (read(STDIN_FILENO, &buff, 1) > 0) {
        if (buff == 0x17) { //CTRL+W
            DebugModule_sendHalt(uart->_BusDevice._MainBus->dbg, 2);
            return;
        }
        
        tmp = uart->rx_head;
        if (((tmp + 1) & RXMASK) != uart->rx_tail) {
            uart->rxbuff[tmp] = buff;
            uart->rx_head = (tmp + 1) & RXMASK;
        }
    }
    tmp = uart->tx_tail;
    if (tmp != uart->tx_head) {
        uart->tx_tail = (tmp + 1) & TXMASK;
        write(STDIN_FILENO, &uart->txbuff[tmp], 1);
    }
}

void SimpleUART_destroy(SimpleUART* uart) {
    tcsetattr(STDIN_FILENO, TCSANOW, &uart->oldt);
}

