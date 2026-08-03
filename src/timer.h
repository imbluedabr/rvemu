#pragma once
#include "bus.h"

#define SYSTICK_E (1 << 0)
#define SYSTICK_TCI (1 << 1)

typedef struct SysTickTimer {
    BusDevice _BusDevice;
    uint32_t count;
    uint32_t tmcr;
    uint32_t ctrl;
} SysTickTimer;

void SysTickTimer_init(SysTickTimer* this, MainBus* _MainBus);
void SysTickTimer_tick(SysTickTimer* this);

