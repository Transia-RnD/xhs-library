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
    uint8_t hook_acc[20];
    hook_account(hook_acc, 20);

    uint8_t otx_acc[20];
    otxn_field(otx_acc, 20, sfAccount);

    if (!BUFFER_EQUAL_20(hook_acc, otx_acc))
        accept(SBUF("lottery_end.c: incoming tx on: `Account`."), __LINE__);

    uint8_t lottery_hash[32];
    uint8_t lh_key[2] = {'L', 'H'};
    if (otxn_param(SBUF(lottery_hash), SBUF(lh_key)) != 32)
    {
        rollback(SBUF("lottery_start.c: Invalid Txn Parameter `LH`"), __LINE__);
    }
    uint8_t lottery_model[72];
    uint8_t lm_key[2] = {'L', 'M'};
    if (otxn_param(SBUF(lottery_model), SBUF(lm_key)) != 72)
    {
        rollback(SBUF("lottery_start.c: Invalid Txn Parameter `LM`"), __LINE__);
    }
    TRACEHEX(lottery_model);

    uint32_t start_time = UINT32_FROM_BUF(lottery_model);
    uint32_t end_time = UINT32_FROM_BUF(lottery_model + 4);
    uint64_t cur_time = ledger_last_time();
    TRACEVAR(start_time);
    TRACEVAR(end_time);
    TRACEVAR(cur_time);

    if (end_time < start_time || start_time == 0 || end_time == 0)
    {
        rollback(SBUF("lottery_start.c: Invalid Binary Model Field `end_time ||/< start_time`"), __LINE__);
    }
    if (end_time < cur_time)
    {
        rollback(SBUF("lottery_start.c: Invalid Binary Model Field `end_time`"), __LINE__);
    }

    state_set(SBUF(lottery_model), SBUF(lottery_hash));
    accept(SBUF("lottery_start.c: Lottery Created."), __LINE__);

    _g(1, 1);
    // unreachable
    return 0;
}