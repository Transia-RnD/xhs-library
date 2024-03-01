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

#define LEDGER_OFFSET 30 // 30 seconds
uint8_t data[8];
#define SEQ_OUT (data + 0U)

uint8_t lottery_ns[32] = {
    0xBEU, 0x7FU, 0x94U, 0xBBU, 0x10U, 0xC4U, 0xBEU, 0x7BU, 0x44U, 0x7FU,
    0x06U, 0x5EU, 0x21U, 0xB3U, 0x01U, 0x85U, 0x3BU, 0x84U, 0x96U, 0x4BU,
    0x94U, 0xD3U, 0xBCU, 0x03U, 0xDCU, 0x11U, 0x73U, 0x2DU, 0xEEU, 0x21U,
    0x2DU, 0x73U};

int64_t hook(uint32_t reserved)
{
    // HOOK ON: TT
    int64_t tt = otxn_type();
    if (tt != ttINVOKE)
    {
        rollback(SBUF("lottery_start: HookOn field is incorrectly set."), INVALID_TXN);
    }

    // ACCOUNT: Hook Account
    uint8_t hook_acc[32];
    hook_account(hook_acc + 12, 20);

    // ACCOUNT: Hook Account
    uint8_t otx_acc[32];
    otxn_field(otx_acc + 12, 20, sfAccount);

    if (!BUFFER_EQUAL_20(hook_acc + 12, otx_acc + 12))
        accept(SBUF("lottery_end.c: incoming tx on: `Account`."), __LINE__);

    uint8_t lottery_model[36];
    uint8_t lm_key[2] = {'L', 'M'};
    if (otxn_param(SBUF(lottery_model), SBUF(lm_key)) != 36)
    {
        rollback(SBUF("lottery_start.c: Invalid Hook Parameter `LM`"), __LINE__);
    }

    // saves the hook param lottery model into the "lottery" namespace
    state_foreign_set(SBUF(lottery_model), hook_acc + 12, 20, lottery_ns, 32, hook_acc + 12, 20);

    // create new lottery
    int64_t ll_time = ledger_last_time();
    ll_time += LEDGER_OFFSET;
    INT64_TO_BUF(SEQ_OUT, ll_time);
    state_set(&ll_time, 8, hook_acc + 12, 20);

    accept(SBUF("lottery_start.c: Lottery Created."), __LINE__);

    _g(1, 1);
    // unreachable
    return 0;
}