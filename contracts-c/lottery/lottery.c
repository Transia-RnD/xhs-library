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

#define SVAR(x) &x, sizeof(x)

#define FLIP_ENDIAN_64(n) ((uint64_t)(((n & 0xFFULL) << 56ULL) |             \
                                      ((n & 0xFF00ULL) << 40ULL) |           \
                                      ((n & 0xFF0000ULL) << 24ULL) |         \
                                      ((n & 0xFF000000ULL) << 8ULL) |        \
                                      ((n & 0xFF00000000ULL) >> 8ULL) |      \
                                      ((n & 0xFF0000000000ULL) >> 24ULL) |   \
                                      ((n & 0xFF000000000000ULL) >> 40ULL) | \
                                      ((n & 0xFF00000000000000ULL) >> 56ULL)))

#define LOTTERY_MODEL 68U
#define ID_OFFSET 0U
#define PRICE_OFFSET 8U
#define MAX_TICKETS_OFFSET 52U

uint8_t lottery_start_ns[32] = {
    0x0EU, 0xD0U, 0xEBU, 0x28U, 0xB2U, 0x4DU, 0x81U, 0x2DU, 0xA0U, 0xC8U,
    0x4FU, 0xDDU, 0xC0U, 0x64U, 0x14U, 0xE0U, 0xC6U, 0x45U, 0x26U, 0x7CU,
    0x8EU, 0xCCU, 0x4DU, 0xC1U, 0xFFU, 0x58U, 0x5CU, 0xF6U, 0x28U, 0x31U,
    0x6DU, 0x70U};

int64_t hook(uint32_t reserved)
{
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

    // GUARD: Skip Outgoing Txns
    if (BUFFER_EQUAL_20(hook_acc + 12, otx_acc + 12))
        rollback(SBUF("lottery.c: outgoing tx on: `Account`."), __LINE__);

    // STATE: Get Lottery Model
    uint8_t lottery_hash[32];
    uint8_t lh_key[2] = {'L', 'H'};
    if (otxn_param(SBUF(lottery_hash), SBUF(lh_key)) != 32)
    {
        rollback(SBUF("lottery_start.c: Invalid OTXN Parameter `LH`"), __LINE__);
    }

    // VALIDATE: Lottery Exists
    uint8_t model_buffer[LOTTERY_MODEL];
    if (state(SBUF(model_buffer), SBUF(lottery_hash)) != LOTTERY_MODEL)
    {
        rollback(SBUF("lottery.c: Lottery does not exist."), __LINE__);
    }

    // STATE: Get End Ledger Time
    uint8_t elt_buffer[8];
    int64_t lottery_id = UINT64_FROM_BUF(model_buffer + ID_OFFSET);
    state_foreign(elt_buffer, 8, &lottery_id, 8, lottery_start_ns, 32, hook_acc + 12, 20);

    int64_t end_ledger = FLIP_ENDIAN_64(UINT64_FROM_BUF(elt_buffer));
    int64_t ll_time = ledger_last_time();
    if (ll_time > end_ledger)
    {
        rollback(SBUF("lottery.c: Lottery ended."), __LINE__);
    }

    // VALIDATE: Payment Amount == Lottery Ticket Price
    uint8_t amt_buffer[8];
    otxn_field(SBUF(amt_buffer), sfAmount);
    int64_t amount_drops = AMOUNT_TO_DROPS(amt_buffer);
    int64_t amount_xfl = float_set(-6, amount_drops);
    int64_t price_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(model_buffer + PRICE_OFFSET));
    if (float_compare(amount_xfl, price_xfl, COMPARE_EQUAL) == 0)
    {
        rollback(SBUF("lottery.c: Invalid Payment Amount."), __LINE__);
    }

    // STATE: Save Lottery Ticket
    uint8_t ticket_hash[32];
    ledger_nonce(SBUF(ticket_hash));
    state_foreign_set(SBUF(otx_acc), SBUF(ticket_hash), lottery_hash, 32, hook_acc + 12, 20);

    // STATE: Get Ticket Count
    int64_t count;
    state_foreign(&count, 8, hook_acc + 12, 20, lottery_hash, 32, hook_acc + 12, 20);

    // STATE: Save Reverse Ticket / Count
    state_foreign_set(otx_acc + 12, 20, &count, 8, lottery_hash, 32, hook_acc + 12, 20);

    // STATE: Update Ticket Count
    count++;
    int64_t max_tickets = UINT64_FROM_BUF(model_buffer + MAX_TICKETS_OFFSET);
    if (count > max_tickets)
    {
        rollback(SBUF("lottery_end.c: Lottery maximum reached."), __LINE__);
    }

    state_foreign_set(&count, 8, hook_acc + 12, 20, lottery_hash, 32, hook_acc + 12, 20);

    _g(1, 1);
    accept(SBUF("lottery.c: Ticket Created."), __LINE__);

    // unreachable
    return 0;
}