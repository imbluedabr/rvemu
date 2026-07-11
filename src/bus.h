#pragma once
#include <stdint.h>
#include "cpu.h"

typedef struct Bus {
    int (*read)(struct Bus* bus, uint32_t address, void* buff, uint32_t n);
    int (*write)(struct Bus* bus, uint32_t address, void* buff, uint32_t n);
} Bus;

typedef struct {
    uint32_t base;
    uint32_t size;
    struct BusDevice* dev;
} MMEntry;

#define MAINBUS_MAX_ENTRIES 16
typedef struct {
    Bus _Bus;
    CPU* cpu;
    MMEntry entries[MAINBUS_MAX_ENTRIES];
    int count;
} MainBus;

typedef struct BusDevice {
    Bus _Bus;
    MainBus* _MainBus;
} BusDevice;

void MainBus_init(MainBus* bus);
void MainBus_addDevice(MainBus* bus, BusDevice* dev, uint32_t base, uint32_t size);



