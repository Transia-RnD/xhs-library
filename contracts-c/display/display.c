#include "hookapi.h"

#define SVAR(x) &(x), sizeof(x)

#define NOPE(x)                                    \
    {                                              \
        return rollback((x), sizeof(x), __LINE__); \
    }

#define MULTIPLIER 0.2 // 0.2 (6 XAH for 30 LEDGERS == 0.2 XAH PER LEDGER)
#define MAX 60         // 30 LEDGERS

uint8_t bids_ns[32] = {
    0xAF, 0xD5, 0x20, 0x13, 0x12, 0xCC, 0xC1, 0xA5,
    0x41, 0x02, 0x63, 0x25, 0x9D, 0x0B, 0x22, 0x30,
    0x99, 0x18, 0x52, 0x16, 0x95, 0x5F, 0x8F, 0x6E,
    0x1C, 0x71, 0xF4, 0x14, 0xF0, 0x32, 0x48, 0xB6};
;

int64_t hook(uint32_t r)
{
    _g(1, 1);
    // ACCOUNT: Origin Tx Account
    uint8_t otxn_accid[32];
    otxn_field(otxn_accid + 12, 20, sfAccount);

    // ACCOUNT: Hook Account
    uint8_t hook_accid[32];
    hook_account(hook_accid + 12, 20);

    // FILTER ON: ACCOUNT
    if (BUFFER_EQUAL_20(hook_accid + 12, otxn_accid + 12))
        DONE("display.c: outgoing tx on `Account`.");

    int64_t tt = otxn_type();
    if (tt != ttINVOKE && tt != ttPAYMENT)
        NOPE("display.c: Rejecting non-Invoke, non-Payment txn.");

    uint8_t amount_buffer[48];
    otxn_slot(1);
    slot_subfield(1, sfAmount, 2);

    int64_t amount_xfl;
    uint32_t flags;
    if (tt == ttPAYMENT)
    {
        // this will fail if flags isn't in the txn, that's also ok.
        otxn_field(&flags, 4, sfFlags);

        // check for partial payments (0x00020000) -> (0x00000200 LE)
        if (flags & 0x200U)
            NOPE("display.c: Partial payments are not supported.");

        otxn_field(SBUF(amount_buffer), sfAmount);

        amount_xfl = slot_float(2);

        if (amount_xfl < 0 || !float_compare(amount_xfl, 1, COMPARE_GREATER))
            NOPE("display.c: Invalid sfAmount.");
    }

    // // Operation
    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1)
        NOPE("display.c: Missing OP parameter on Invoke/Payment.");

    // sanity check
    if (op == 'B' && tt != ttPAYMENT)
        NOPE("display.c: Bid operations must be a payment transaction.");

    // sanity check
    if (op == 'U' && tt != ttINVOKE)
        NOPE("display.c: Update operations must be a invoke transaction.");

    int64_t count;
    if (state(&count, 8, hook_accid + 12, 20) == DOESNT_EXIST)
    {
        // set current if initialization
        ASSERT(0 < state_set(&count, 8, hook_accid + 12, 20));
    }

    switch (op)
    {
        case 'B':
        {
            uint8_t tid[40];
            if (otxn_param(tid, 32, "ID", 2) != 32)
                NOPE("display.c: Missing ID parameter on Payment.");

            int64_t bid_count;
            state_foreign(&bid_count, 8, hook_accid + 12, 20, bids_ns, 32, hook_accid + 12, 20);

            int64_t end_ledger;
            int64_t cur_ledger = ledger_seq();
            if (state_foreign(&end_ledger, 8, hook_accid + 12, 20, 0, 32, hook_accid + 12, 20) == DOESNT_EXIST || end_ledger < cur_ledger)
            {
                end_ledger = cur_ledger;
            }

            // calculate ledgers
            int64_t amount_int = float_int(amount_xfl, 0, 1);
            int64_t duration = amount_int / MULTIPLIER;
            if (duration > MAX)
                NOPE("display.c: Duration cannot be more than 60 ledgers.");

            end_ledger += duration;
            UINT64_TO_BUF(tid + 32, end_ledger);
            ASSERT(0 < state_foreign_set(tid, 40, &bid_count, 8, bids_ns, 32, hook_accid + 12, 20));

            bid_count++;
            ASSERT(0 < state_foreign_set(&bid_count, 8, hook_accid + 12, 20, bids_ns, 32, hook_accid + 12, 20));
            ASSERT(0 < state_foreign_set(&end_ledger, 8, hook_accid + 12, 20, 0, 32, hook_accid + 12, 20));
            TRACESTR("display.c: Bid.");
            return accept(SBUF("display.c: Bid."), __LINE__);
        }
        case 'U':
        {
            uint8_t tid[40];
            state_foreign(tid, 40, &count, 8, bids_ns, 32, hook_accid + 12, 20);
            uint64_t final_ledger = UINT64_FROM_BUF(tid + 32);
            int64_t cur_sequence = ledger_seq();
            if (cur_sequence < final_ledger)
            {
                NOPE("display.c: Duration has not expired.");
            }

            state_foreign_set(0, 0, &count, 8, bids_ns, 32, hook_accid + 12, 20);

            count++;
            ASSERT(0 < state_set(&count, 8, hook_accid + 12, 20));
            TRACESTR("display.c: Update.");
            return accept(SBUF("display.c: Update."), __LINE__);
        }
    }
}