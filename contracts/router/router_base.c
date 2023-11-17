#include "hookapi.h"

#define UINT8_FROM_BUF(buf)\
    (((uint8_t)((buf)[0]) <<  0))
            
int64_t hook(uint32_t r)
{
    _g(1,1);

    uint8_t buffer[11];
    if (otxn_param(buffer, 11, "HPA", 3) != 11)
    {
        accept(SBUF("router.c: Ignore."), __LINE__);
    };
    // 0A01000101000000000000

    int64_t num_pos = buffer[0];
    for (int i = 0; GUARD(10), i < num_pos; ++i)
    {
        if (UINT8_FROM_BUF(buffer + 1 + (1 * i)) == 0) {
            uint8_t h_hash[32];
            hook_hash(h_hash, 32, i);
            hook_skip(h_hash, 32, 0);
        }
    }

    return accept(SBUF("router.c: Accept."), __LINE__);
}

