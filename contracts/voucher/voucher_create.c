/**
 *
 */
#include "hookapi.h"

#define FLIP_ENDIAN_64(n) ((uint64_t)(((n & 0xFFULL) << 56ULL) |             \
                                      ((n & 0xFF00ULL) << 40ULL) |           \
                                      ((n & 0xFF0000ULL) << 24ULL) |         \
                                      ((n & 0xFF000000ULL) << 8ULL) |        \
                                      ((n & 0xFF00000000ULL) >> 8ULL) |      \
                                      ((n & 0xFF0000000000ULL) >> 24ULL) |   \
                                      ((n & 0xFF000000000000ULL) >> 40ULL) | \
                                      ((n & 0xFF00000000000000ULL) >> 56ULL)))

#define MODEL_SIZE 93

int64_t hook(uint32_t reserved)
{

    TRACESTR("voucher_create.c: called");

    // HOOK ON: TT
    int64_t tt = otxn_type();
    if (tt != ttPAYMENT)
    {
        rollback(SBUF("voucher_create.c: HookOn field is incorrectly set."), INVALID_TXN);
    }

    uint8_t otx_acc[32];
    otxn_field(otx_acc + 12, 20, sfAccount);

    uint8_t hook_acc[32];
    hook_account(hook_acc + 12, 20);

    if (BUFFER_EQUAL_20(hook_acc + 12, otx_acc + 12))
        DONE("voucher_create.c: outgoing tx on: `Account`.");

    uint8_t model_buffer[MODEL_SIZE];
    uint8_t model_key[1] = {'M'};
    if (otxn_param(SBUF(model_buffer), SBUF(model_key)) != MODEL_SIZE)
    {
        DONE("voucher_create.c: invalid otxn parameter: `M`.");
    }

    uint8_t hash_buffer[32];
    uint8_t hash_key[1] = {'H'};
    if (otxn_param(SBUF(hash_buffer), SBUF(hash_key)) != 32)
    {
        DONE("voucher_create.c: invalid otxn parameter: `H`.");
    }

    if (state(SBUF(model_buffer), hash_buffer, 32) == 32)
    {
        accept(SBUF("voucher_create.c: Voucher already exists."), __LINE__);
    }

    int64_t oslot = otxn_slot(0);
    if (oslot < 0)
        rollback(SBUF("voucher_create: Could not slot originating txn."), 1);

    int64_t amt_slot = slot_subfield(oslot, sfAmount, 0);
    if (amt_slot < 0)
        rollback(SBUF("voucher_create: Could not slot otxn.sfAmount"), 2);

    int64_t amount_token = slot_float(amt_slot);
    if (amount_token < 0)
        rollback(SBUF("voucher_create: Could not parse amount."), 1);

    uint64_t model_amount = FLIP_ENDIAN_64(UINT64_FROM_BUF(model_buffer + 12));
    uint32_t model_limit = UINT32_FROM_BUF(model_buffer);
    if (float_compare(model_amount * model_limit, amount_token, COMPARE_EQUAL) == 0)
        rollback(SBUF("voucher_create.c: invalid model parameter `Amount`."), __LINE__);

    state_set(SBUF(model_buffer), hash_buffer, 32);

    accept(SBUF("voucher_create.c: Voucher Created."), __LINE__);
    _g(1, 1);
    // unreachable
    return 0;
}