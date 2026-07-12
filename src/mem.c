#include "mem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>

static int SRAM_read(struct Bus* bus, uint32_t address, void* buff, uint32_t n) {
    SRAM* sram = (SRAM*) bus;
    if ((address + n) > sram->size) return -1;
    memcpy(buff, sram->base + address, n);
    return 0;
}

static int SRAM_write(struct Bus* bus, uint32_t address, void* buff, uint32_t n) {
    SRAM* sram = (SRAM*) bus;
    if ((address + n) > sram->size) return -1;
    memcpy(sram->base + address, buff, n);
    return 0;
}


void SRAM_init(SRAM* sram, MainBus* _MainBus, int size) {
    sram->_BusDevice._MainBus = _MainBus;
    sram->_BusDevice._Bus.read = SRAM_read;
    sram->_BusDevice._Bus.write = SRAM_write;
    sram->_BusDevice.type = DEV_SRAM;
    sram->base = malloc(size);
    sram->size = size;
}

void SRAM_loadFile(SRAM* sram, const char* path, uint32_t start, uint32_t size) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("SRAM: %s: failed to load file\n", path);
        return;
    }
    
    if (size > sram->size) size = sram->size;
    read(fd, sram->base, size);
    close(fd);
}

void SRAM_destroy(SRAM* sram) {
    free(sram->base);
    sram->base = NULL;
}

