
#include "hookapi.h"

// Hook Parameter
// Name: AMT
// Value (XFL=10): 0080C6A47E8DC354

int64_t hook(uint32_t reserved)
{
    // HOOK ON: TT
    int64_t tt = otxn_type();
    if (tt != ttPAYMENT)
    {
        rollback(SBUF("lesson_one.c: HookOn field is incorrectly set."), INVALID_TXN);
    }

    // ACCOUNT: Origin Tx Account
    uint8_t otx_acc[20];
    otxn_field(otx_acc, 20, sfAccount);
    
    // ACCOUNT: Hook Account
    uint8_t hook_acc[20];
    hook_account(SBUF(hook_acc));

    // FILTER ON: ACCOUNT
    if (!BUFFER_EQUAL_20(hook_acc, otx_acc))
    {
        accept(SBUF("lesson_one.c: Accepting Transaction `Incoming`."), __LINE__);
    }

    uint8_t param_buffer[8];
    uint8_t param_key[3] = {'A', 'M', 'T'};
    uint64_t guard_drops = 1000000;
    if (hook_param(param_buffer, 8, SBUF(param_key)) == 8)
    {
        guard_drops = float_int(*((int64_t*)param_buffer), 6, 1);
        if (guard_drops <= 0)
        {
            rollback(SBUF("lesson_one.c: Invalid Amount Guard."), __LINE__);
        }
    }

    uint8_t field_value[8];
    int64_t otxn_field_size = otxn_field(field_value, 8,  sfAmount);
    int64_t amount_drops = AMOUNT_TO_DROPS(field_value);
    if (amount_drops > guard_drops)
    {
        rollback(SBUF("lesson_one.c: Blocking Transaction `Amount`."), __LINE__);
    }

    accept(SBUF("lesson_one.c: Accepting Transaction."), __LINE__);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}