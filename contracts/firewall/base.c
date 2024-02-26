/**
 *
 */
#include "hookapi.h"

int64_t hook(uint32_t reserved) {

    TRACESTR("Base.c: Called.");
    accept(SBUF("base: Finished."), __LINE__);

    _g(1,1);
    // unreachable
    return 0;
}