#pragma once
#include "bus.h"
#include <termios.h>

typedef struct {
    BusDevice _BusDevice;
    struct termios oldt;
    char data;
} SimpleUART;


void SimpleUART_init(SimpleUART* uart, MainBus* _MainBus);
void SimpleUART_tick(SimpleUART* uart);
void SimpleUART_destroy(SimpleUART* uart);

