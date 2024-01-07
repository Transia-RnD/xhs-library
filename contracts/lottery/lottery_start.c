//------------------------------------------------------------------------------
/*
    Copyright (c) 2023 Transia, LLC

    This software component is intended for use by individuals or entities who
    possess the necessary technical understanding and qualifications to utilize
    such components in accordance with applicable laws and regulations.
    Unauthorized use or distribution of this component may be subject to legal
    consequences.

    The information provided in this software component is for informational
    purposes only and should not be considered as technical advice or a
    recommendation to engage in any specific implementation or usage strategy.
    It is important to consult with a qualified professional or technical advisor
    before integrating or making any decisions based on this component.
*/
//==============================================================================

#include "hookapi.h"

// #define LEDGER_OFFSET 604100 // 7 days
#define LEDGER_OFFSET 5 // 5 seconds
uint8_t data[8];
#define SEQ_OUT (data + 0U)

uint8_t lottery_ns[32] = {
    0xBEU, 0x7FU, 0x94U, 0xBBU, 0x10U, 0xC4U, 0xBEU, 0x7BU, 0x44U, 0x7FU,
    0x06U, 0x5EU, 0x21U, 0xB3U, 0x01U, 0x85U, 0x3BU, 0x84U, 0x96U, 0x4BU,
    0x94U, 0xD3U, 0xBCU, 0x03U, 0xDCU, 0x11U, 0x73U, 0x2DU, 0xEEU, 0x21U,
    0x2DU, 0x73U};

int64_t hook(uint32_t reserved)
{
    TRACESTR("lottery_start.c: Called.");

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