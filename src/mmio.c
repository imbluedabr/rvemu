#include "mmio.h"
#include <unistd.h>
#include <sys/fcntl.h>

static int SimpleUART_read(struct Bus* bus, uint32_t address, void* buff, uint32_t n) {
    SimpleUART* uart = (SimpleUART*) bus;
    if (address == 0x00 && n == 1) {
        char c = uart->data;
        uart->data = 0;
        return c;
    }
    return -1;
}

static int SimpleUART_write(struct Bus* bus, uint32_t address, void* buff, uint32_t n) {
    if (address == 0x00 && n == 1) {
        return write(STDIN_FILENO, buff, 1);
    }
    return -1;
}


void SimpleUART_init(SimpleUART* uart, MainBus* _MainBus) {
    uart->_BusDevice._MainBus = _MainBus;
    uart->_BusDevice._Bus.read = SimpleUART_read;
    uart->_BusDevice._Bus.write = SimpleUART_write;

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
    if (read(STDIN_FILENO, &buff, 1) > 0) {
        if (buff == 0x17) {
            CPU* hart = uart->_BusDevice._MainBus->cpu;
            hart->mode = MODE_D;
            hart->csr_mcause = MCAUSE_CODE(FAULT_DEBUG);
            hart->csr_mtval = 0;
            return;
        }
        uart->data = buff;
    }
}

void SimpleUART_destroy(SimpleUART* uart) {
    tcsetattr(STDIN_FILENO, TCSANOW, &uart->oldt);
}

