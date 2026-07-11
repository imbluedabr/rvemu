#pragma once
#include "bus.h"

typedef struct {
    BusDevice _BusDevice;
    uint8_t* base;
    uint32_t size;
} SRAM;

void SRAM_init(SRAM* sram, MainBus* _MainBus, int size);
void SRAM_loadFile(SRAM* sram, const char* path, uint32_t start, uint32_t size);
void SRAM_destroy(SRAM* sram);

