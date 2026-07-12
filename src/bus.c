#include "bus.h"
#include "cpu.h"
#include <stddef.h>

static int read(struct Bus* bus, uint32_t address, void* buff, uint32_t n) {
    MainBus* mbus = (MainBus*) bus;
    for (int i = 0; i < mbus->count; i++) {
        MMEntry* entry = &mbus->entries[i];
        if (entry->base <= address && (entry->base + entry->size) > (address + n)) {
            return entry->dev->_Bus.read(&entry->dev->_Bus, address - entry->base, buff, n);
        }
    }

    return -1;
}

static int write(struct Bus* bus, uint32_t address, void* buff, uint32_t n) {
    MainBus* mbus = (MainBus*) bus;
    for (int i = 0; i < mbus->count; i++) {
        MMEntry* entry = &mbus->entries[i];
        if (entry->base <= address && (entry->base + entry->size) > (address + n)) {
            return entry->dev->_Bus.write(&entry->dev->_Bus, address - entry->base, buff, n);
        }
    }
    
    return -1;
}

void MainBus_init(MainBus* bus) {
    bus->_Bus.read = &read;
    bus->_Bus.write = &write;
    bus->count = 0;
}

void MainBus_addDevice(MainBus* bus, BusDevice* dev, uint32_t base, uint32_t size) {
    if (bus->count == MAINBUS_MAX_ENTRIES) return;
    MMEntry* entry = &bus->entries[bus->count++];
    entry->dev = dev;
    entry->base = base;
    entry->size = size;
}

BusDevice* MainBus_getDevice(MainBus* bus, uint32_t address) {
    for (int i = 0; i < bus->count; i++) {
        MMEntry* entry = &bus->entries[i];
        if (entry->base <= address && (entry->base + entry->size) > (address)) {
            return entry->dev;
        }
    }
    
    return NULL;
}


