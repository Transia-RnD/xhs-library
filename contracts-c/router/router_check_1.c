/**
 *
 */
#include "hookapi.h"

int64_t hook(uint32_t reserved)
{
    TRACESTR("router_check_1.c: Called.");
    TRACEVAR(hook_pos());
    accept(SBUF("router_check_1.c: Finished."), __LINE__);

    _g(1,1);
    // unreachable
    return 0;
}