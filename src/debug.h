#pragma once
#include "bus.h"

typedef struct {
    MainBus* systemBus;
    int running;
} DebugModule;

void DebugModule_init(DebugModule* dbg, MainBus* systemBus);
void DebugModule_tick(DebugModule* dbg);

