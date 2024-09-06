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

int64_t hook(uint32_t reserved)
{

    TRACESTR("oracle.c: Called.");

    // ACCOUNT: Hook Account
    uint8_t hook_accid[20];
    hook_account(hook_accid, 20);

    // ACCOUNT: OTXN Account
    uint8_t otxn_accid[20];
    otxn_field(otxn_accid, 20, sfAccount);

    if (!BUFFER_EQUAL_20(hook_accid, otxn_accid))
    {
        rollback(SBUF("oracle.c: Invalid OTXN `Account`"), __LINE__);
    }

    uint8_t txn_id[32];
    ASSERT(otxn_id(txn_id, 32, 0) == 32);

    ASSERT(otxn_slot(1) == 1);
    ASSERT(slot_subfield(1, sfBlob, 2) == 2);
    
    uint8_t buffer[676];

    ASSERT(slot(SBUF(buffer), 2) > 0);

    uint16_t len = (uint16_t)buffer[0];
    uint8_t* ptr = buffer + 1;
    if (len > 192)
    {
        len = 193 + ((len - 193) * 256) + ((uint16_t)(buffer[1]));
        ptr++;
    }

    uint8_t* end = ptr + len;
    while (ptr < end)
    {
        GUARD(48);
        uint8_t hash[32];

        util_sha512h(SBUF(hash), ptr, 40);
        ASSERT(state_set(ptr + 40, 8, hash, 32) == 8);
        ptr += 48;
    }

    accept(SBUF("oracle.c: Updated."), __LINE__);
    _g(1, 1);
    return 0;
}