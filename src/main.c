#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "uart.h"
#include "mem.h"
#include "debug.h"

int main(int argc, char** argv) {
    printf("rvemu v0.1.4\n");
    
    CPU hart0;
    CPU_init(&hart0);

    MainBus systemBus;
    MainBus_init(&systemBus);
    systemBus.cpu = &hart0;
    hart0.system_bus = &systemBus;
    
    DebugModule debug;
    DebugModule_init(&debug, &systemBus);
    systemBus.dbg = &debug;

    SRAM sram0;
    SRAM_init(&sram0, &systemBus, 0x1000);
    MainBus_addDevice(&systemBus, &sram0._BusDevice, 0x0000, 0x1000);
    
    SimpleUART uart0;
    SimpleUART_init(&uart0, &systemBus);
    MainBus_addDevice(&systemBus, &uart0._BusDevice, 0x8000, 0x10);

    DebugModule_sendCmd(&debug, DBG_LOAD, "./test/test.bin", 0x0000, 0x1000);
    DebugModule_sendCmd(&debug, DBG_FILE, "./test/test.elf");
    DebugModule_sendCmd(&debug, DBG_CATCH_EBREAK);
    //DebugModule_sendCmd(&debug, DBG_CATCH_VEC);
    DebugModule_sendCmd(&debug, DBG_HALT);

    while (debug.running) {
        CPU_tick(&hart0);
        SimpleUART_tick(&uart0);
        DebugModule_tick(&debug);
    }
    
    SimpleUART_destroy(&uart0);
    SRAM_destroy(&sram0);

    return 0;
}


