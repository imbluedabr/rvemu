#pragma once
#include <stdint.h>

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
typedef struct MainBus {
    Bus _Bus;
    struct CPU* cpu;
    struct DebugModule* dbg;
    MMEntry entries[MAINBUS_MAX_ENTRIES];
    int count;
} MainBus;

typedef enum {
    DEV_SRAM,
    DEV_SERIAL,
    DEV_TIMER
} DeviceType;

typedef struct BusDevice {
    Bus _Bus;
    MainBus* _MainBus;
    DeviceType type;
} BusDevice;

void MainBus_init(MainBus* bus);
void MainBus_addDevice(MainBus* bus, BusDevice* dev, uint32_t base, uint32_t size);
MMEntry* MainBus_getMMEntry(MainBus* bus, uint32_t address);


