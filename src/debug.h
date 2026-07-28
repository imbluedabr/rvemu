#pragma once
#include "bus.h"
#include <bfd.h>

#define BKPT_LEN 16
typedef struct DebugModule {
    MainBus* systemBus;
    int running;
    bfd *abfd;
    asymbol **syms;
    uint32_t breakpoint[BKPT_LEN];
} DebugModule;

typedef enum {
    DBG_LOAD,
    DBG_FILE,
    DBG_INFO,
    DBG_LINE,
    DBG_BREAK,
    DBG_CATCH_VEC,
    DBG_CATCH_EBREAK,
    DBG_HALT,
    DBG_STEP,
    DBG_CONTINUE,
    DBG_SKIP
} DbgCmd;

void DebugModule_init(DebugModule* dbg, MainBus* systemBus);
void DebugModule_sendHalt(DebugModule* dbg, int cause);
void DebugModule_sendCmd(DebugModule* dbg, DbgCmd cmd, ...);
void DebugModule_tick(DebugModule* dbg);

