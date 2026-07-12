#pragma once
#include "bus.h"

typedef struct DebugModule {
    MainBus* systemBus;
    int running;
} DebugModule;

typedef enum {
    DBG_HALT,
    DBG_LOAD,
    DBG_CATCH_VEC,
    DBG_CATCH_EBREAK,
    DBG_CONTINUE
} DbgCmd;

void DebugModule_init(DebugModule* dbg, MainBus* systemBus);
void DebugModule_sendHalt(DebugModule* dbg, int cause);
void DebugModule_sendCmd(DebugModule* dbg, DbgCmd cmd, ...);
void DebugModule_tick(DebugModule* dbg);

