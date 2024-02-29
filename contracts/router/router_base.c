//------------------------------------------------------------------------------
/*
    Copyright (c) 2024 Transia, LLC

    This financial tool is designed for use by individuals or organizations 
    that hold the appropriate licenses and qualifications to engage with such 
    products in a manner that complies with all relevant laws and regulations. 
    Unauthorized use or redistribution of this tool may result in legal action.

    The content provided in this financial tool is for informational purposes 
    only and should not be interpreted as financial advice or an endorsement of 
    any particular investment or financial strategy. Users should seek advice 
    from a certified professional or financial advisor before making any 
    investment choices.

    By using this financial tool, the user agrees to indemnify Transia, LLC from 
    any claims, damages, or losses that may arise from their use or reliance on 
    the information contained within. The user acknowledges that they are utilizing 
    this tool at their own risk and that Transia, LLC will not be held responsible 
    for any direct, indirect, incidental, punitive, special, or consequential 
    damages, including but not limited to, damages for lost profits, goodwill, 
    usage, data, or other intangible losses.

*/
//==============================================================================

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

