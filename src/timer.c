#include "timer.h"
#include "cpu.h"

static int SysTickTimer_read(struct Bus* bus, uint32_t address, void* buff, uint32_t n) {
    SysTickTimer* this = (SysTickTimer*) bus;
    
    if (address == 0x00 && n == 4) {
        *(uint32_t*) buff = this->count;
    } else if (address == 0x04 && n == 4) {
        *(uint32_t*) buff = this->tmcr;
    } else if (address == 0x08 && n == 4) {
        *(uint32_t*) buff = this->ctrl;
    } else {
        return -1;
    }
    return 0;
}

static int SysTickTimer_write(struct Bus* bus, uint32_t address, void* buff, uint32_t n) {
    SysTickTimer* this = (SysTickTimer*) bus;
    if (address == 0x00 && n == 4) {
        this->count = *(uint32_t*) buff;
    } else if (address == 0x04 && n == 4) {
        this->tmcr = *(uint32_t*) buff;
    } else if (address == 0x08 && n == 4) {
        this->ctrl = (*(uint32_t*) buff) & 0x3;
    } else {
        return -1;
    }
    return 0;
}

void SysTickTimer_init(SysTickTimer* this, MainBus* _MainBus) {
    this->_BusDevice._Bus.read = SysTickTimer_read;
    this->_BusDevice._Bus.write = SysTickTimer_write;
    this->_BusDevice._MainBus = _MainBus;
    this->_BusDevice.type = DEV_TIMER;
    this->count = 0;
    this->ctrl = 0;
    this->tmcr = 0;
}

void SysTickTimer_tick(SysTickTimer* this) {
    if (this->ctrl & SYSTICK_E) {
        this->count++;
        if ((this->ctrl & SYSTICK_TCI) && (this->count > this->tmcr)) {
            this->count = 0;
            CPU_interrupt_fast(this->_BusDevice._MainBus->cpu, 1);
        }
    }
}


