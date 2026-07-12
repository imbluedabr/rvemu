#pragma once
#include "bus.h"
#include <termios.h>

#define RXSIZE 16
#define TXSIZE 16
#define RXMASK (RXSIZE-1)
#define TXMASK (TXSIZE-1)
typedef struct {
    BusDevice _BusDevice;
    struct termios oldt;
    char rxbuff[RXSIZE];
    char txbuff[TXSIZE];
    uint8_t rx_tail;
    uint8_t rx_head;
    uint8_t tx_tail;
    uint8_t tx_head;
} SimpleUART;


void SimpleUART_init(SimpleUART* uart, MainBus* _MainBus);
void SimpleUART_tick(SimpleUART* uart);
void SimpleUART_destroy(SimpleUART* uart);

