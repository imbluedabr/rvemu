#include <stdio.h>
#include <time.h>
#include "cpu.h"
#include "bus.h"
#include "uart.h"
#include "timer.h"
#include "mem.h"
#include "debug.h"

int main(int argc, char** argv) {
    printf("rvemu v0.1.5\n");
    
    MainBus systemBus;
    MainBus_init(&systemBus);

    CPU hart0;
    CPU_init(&hart0, &systemBus);
    systemBus.cpu = &hart0;
    
    DebugModule debug;
    DebugModule_init(&debug, &systemBus);
    systemBus.dbg = &debug;

    SRAM sram0;
    SRAM_init(&sram0, &systemBus, 0x2000);
    MainBus_addDevice(&systemBus, &sram0._BusDevice, 0x0000, 0x2000);
    
    SimpleUART uart0;
    SimpleUART_init(&uart0, &systemBus);
    MainBus_addDevice(&systemBus, &uart0._BusDevice, 0x8000, 0x10);
    
    SysTickTimer timer0;
    SysTickTimer_init(&timer0, &systemBus);
    MainBus_addDevice(&systemBus, &timer0._BusDevice, 0x8010, 0x10);

    DebugModule_sendCmd(&debug, DBG_LOAD, "./test/test.bin", 0x0000, 0x2000);
    DebugModule_sendCmd(&debug, DBG_FILE, "./test/test.elf");
    DebugModule_sendCmd(&debug, DBG_CATCH_EBREAK);
    //DebugModule_sendCmd(&debug, DBG_CATCH_VEC);
    DebugModule_sendCmd(&debug, DBG_HALT);
    
    int cycle_count = 0;
    clock_t start = clock(), diff;

    while (debug.running) {
        CPU_tick(&hart0);
        SimpleUART_tick(&uart0);
        SysTickTimer_tick(&timer0);
        DebugModule_tick(&debug);
        cycle_count++;
    }
    diff = clock() - start;
    int elapsed_time = diff*10000000/CLOCKS_PER_SEC; 
    int tickspeed = elapsed_time/cycle_count;
    int clockspeed = 1000/tickspeed;
    SimpleUART_destroy(&uart0);
    SRAM_destroy(&sram0);
    printf("timer0: count=%d, tmcr=%d, ctrl=%d\n", timer0.count, timer0.tmcr, timer0.ctrl);
    printf("cycles=%d, elapsed_time=%d us, tickspeed=%d us/t or %d kHz\n", cycle_count, elapsed_time, tickspeed, clockspeed);
    return 0;
}


