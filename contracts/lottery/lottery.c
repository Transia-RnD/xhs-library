/**
 *
 */
#include "hookapi.h"

#define SVAR(x) &x, sizeof(x)

#define MODEL_SIZE 72

int64_t hook(uint32_t reserved ) {

    TRACESTR("lottery.c: called");

    // HOOK ON: TT Payment
    int64_t tt = otxn_type();
    if (tt != ttPAYMENT)
    {
        rollback(SBUF("lottery.c: HookOn field is incorrectly set."), INVALID_TXN);
    }

    uint8_t otx_acc[32];
    otxn_field(otx_acc + 12, 20, sfAccount);

    uint8_t hook_acc[32];
    hook_account(hook_acc + 12, 20);

    if (BUFFER_EQUAL_20(hook_acc + 12, otx_acc + 12))
        rollback(SBUF("lottery.c: outgoing tx on: `Account`."), __LINE__);


    // Validate: Current Ledger Time < End Time
    // Validate: Current Ledger Time > Start Time
    // Validate: Payment Amount is == price

    // OTXN PARAM: Hash
    uint8_t hash_buffer[32];
    uint8_t hash_key[1] = {'H'};
    if (otxn_param(SBUF(hash_buffer), SBUF(hash_key)) != 32)
    {
        rollback(SBUF("lottery.c: invalid otxn parameter: `H`."), __LINE__);
    }

    // VALIDATION: Lottery Exists
    uint8_t model_buffer[MODEL_SIZE];
    if (state(SBUF(model_buffer), hash_buffer, 32) != MODEL_SIZE)
    {
        rollback(SBUF("lottery.c: Lottery does not exist."), __LINE__);
    }

    int64_t oslot = otxn_slot(0);
    if (oslot < 0)
        rollback(SBUF("lottery.c: Could not slot originating txn."), 1);

    int64_t amt_slot = slot_subfield(oslot, sfAmount, 0);
    if (amt_slot < 0)
        rollback(SBUF("lottery.c: Could not slot otxn.sfAmount"), 2);

    int64_t amount_xfl = slot_float(amt_slot);
    if (amount_xfl < 0)
        rollback(SBUF("lottery.c: Could not parse amount."), 1);
    
    TRACEVAR(amount_xfl) // <- amount as token

    int64_t price_xfl = 6107881094714392576;
    // VALIDATION: Payment
    // int64_t value1 = -INT64_FROM_BUF(model_buffer + 12U);
    // TRACEVAR(value1);
    // int64_t value2 = float_int(value1, 0, 1);
    if (float_compare(amount_xfl, price_xfl, COMPARE_EQUAL) == 0)
    {
        rollback(SBUF("lottery.c: Lottery payment error."), __LINE__);
    }

    // Valid Payment
    
    // Save state for lottery ticket
    uint8_t ticket_hash[32];
    ledger_nonce(SBUF(ticket_hash));
    state_set(SBUF(otx_acc), SBUF(ticket_hash));
    
    // Update Ticket count
    int64_t count;
    state(&count, 8, hook_acc + 12, 20);

    TRACEVAR(count);

    // Reverse ticket / count
    state_set(otx_acc + 12, 20, &count, 8);

    count++;

    TRACEVAR(count);

    state_set(&count, 8, hook_acc + 12, 20);

    _g(1,1);
    accept(SBUF("lottery.c: Ticket Created."), __LINE__);

    // unreachable
    return 0;
}