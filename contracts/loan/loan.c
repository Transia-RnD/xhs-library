//------------------------------------------------------------------------------
/*

Operations:

// (Loan) ops are:
    // C - Create: create loan - Invoke
    // E - End: end loan - Invoke
    // P - Payment: loan - Payment
    // R - Receive: drawdown loan - Invoke


*/
//==============================================================================

#include "hookapi.h"

#define ttREMIT 95
#define sfAmounts ((15U << 16U) + 92U)
#define sfURITokenIDs ((19U << 16U) + 99U)

#define SVAR(x) &(x), sizeof(x)

#define UINT8_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    buf[0] = (((uint8_t)i) >> 0) & 0xFFUL;\
    if (i < 0) buf[0] |= 0x80U;\
}

#define FLIP_ENDIAN_64(n) ((uint64_t)(((n & 0xFFULL) << 56ULL) |             \
                                      ((n & 0xFF00ULL) << 40ULL) |           \
                                      ((n & 0xFF0000ULL) << 24ULL) |         \
                                      ((n & 0xFF000000ULL) << 8ULL) |        \
                                      ((n & 0xFF00000000ULL) >> 8ULL) |      \
                                      ((n & 0xFF0000000000ULL) >> 24ULL) |   \
                                      ((n & 0xFF000000000000ULL) >> 40ULL) | \
                                      ((n & 0xFF00000000000000ULL) >> 56ULL)))

#define ACCOUNT_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    *(uint64_t*)(buf + 0) = *(uint64_t*)(i +  0);\
    *(uint64_t*)(buf + 8) = *(uint64_t*)(i +  8);\
    *(uint32_t*)(buf + 16) = *(uint32_t*)(i + 16);\
}

#define DONE(x)\
    return accept(SBUF(x), __LINE__)

#define NOPE(x)\
    return rollback(SBUF(x), __LINE__)

// clang-format off
uint8_t txn[289] =
{
/* size,upto */
/*   3,   0 */   0x12U, 0x00U, 0x5FU,                                                           /* tt = Remit       */
/*   5,   3 */   0x22U, 0x80U, 0x00U, 0x00U, 0x00U,                                          /* flags = tfCanonical */
/*   5,   8 */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                                 /* sequence = 0 */
/*   5,  13 */   0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                                /* dtag, flipped */
/*   6,  18 */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                      /* first ledger seq */
/*   6,  24 */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                       /* last ledger seq */
/*   9,  30 */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                         /* fee      */
/*  35,  39 */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       /* pubkey   */
/*  22,  74 */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                  /* srcacc  */
/*  22,  96 */   0x83U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                  /* dstacc  */
/* 116, 118 */   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    /* emit detail */
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   2, 234 */   0xF0U, 0x5CU,                                                               /* lead-in amount array */
/*   2, 236 */   0xE0U, 0x5BU,                                                              /* lead-in amount entry A*/
/*  49, 238 */   0x61U,0,0,0,0,0,0,0,0,
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                                /* amount A */
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   2, 287 */   0xE1U, 0xF1U,                                                 /* lead out, may also appear at end of A */
/*   0, 289 */                
};
// clang-format on

// TX BUILDER
#define FLS_OUT (txn + 20U)
#define LLS_OUT (txn + 26U)
#define DTAG_OUT (txn + 14U)
#define FEE_OUT (txn + 31U)
#define HOOK_ACC (txn + 76U)
#define OTX_ACC (txn + 98U)
#define EMIT_OUT (txn + 118U)
#define AMOUNT_OUT (txn + 238U)

// clang-format off
#define PREPARE_REMIT_TXN(account_buffer, dest_buffer) do { \
    if (otxn_field(DTAG_OUT, 4, sfSourceTag) == 4) \
        *(DTAG_OUT - 1) = 0x2EU; \
    uint32_t fls = (uint32_t)ledger_seq() + 1; \
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls); \
    uint32_t lls = fls + 4; \
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls); \
    ACCOUNT_TO_BUF(HOOK_ACC, account_buffer); \
    ACCOUNT_TO_BUF(OTX_ACC, dest_buffer); \
    etxn_details(EMIT_OUT, 116U); \
    int64_t fee = etxn_fee_base(SBUF(txn)); \
    uint8_t *b = FEE_OUT; \
    *b++ = 0b01000000 + ((fee >> 56) & 0b00111111); \
    *b++ = (fee >> 48) & 0xFFU; \
    *b++ = (fee >> 40) & 0xFFU; \
    *b++ = (fee >> 32) & 0xFFU; \
    *b++ = (fee >> 24) & 0xFFU; \
    *b++ = (fee >> 16) & 0xFFU; \
    *b++ = (fee >> 8) & 0xFFU; \
    *b++ = (fee >> 0) & 0xFFU; \
} while(0) 
// clang-format on

// pool namespace: 27CAC5503836765CD10751D27AB4A6E17D7A80D4C948430A5A81513973F9B51E
uint8_t pool_ns[32] = {
    0x27U, 0xCAU, 0xC5U, 0x50U, 0x38U, 0x36U, 0x76U, 0x5CU, 0xD1U, 0x07U, 
    0x51U, 0xD2U, 0x7AU, 0xB4U, 0xA6U, 0xE1U, 0x7DU, 0x7AU, 0x80U, 0xD4U, 
    0xC9U, 0x48U, 0x43U, 0x0AU, 0x5AU, 0x81U, 0x51U, 0x39U, 0x73U, 0xF9U, 
    0xB5U, 0x1EU
};
// collateral namespace: 20BED891A324D5200ECC160B105762CC7E6A61F40E41462FFD7B589969D8F0A4
uint8_t collateral_ns[32] = {
    0x20U, 0xBEU, 0xD8U, 0x91U, 0xA3U, 0x24U, 0xD5U, 0x20U, 0x0EU, 0xCCU, 
    0x16U, 0x0BU, 0x10U, 0x57U, 0x62U, 0xCCU, 0x7EU, 0x6AU, 0x61U, 0xF4U, 
    0x0EU, 0x41U, 0x46U, 0x2FU, 0xFDU, 0x7BU, 0x58U, 0x99U, 0x69U, 0xD8U, 
    0xF0U, 0xA4U
};

// outstanding key: 35AB87618C9FC09E53A67F4CB194FC4B51D327EC5D21C6958B00A623D6F8EC1F
uint8_t outstanding_key[32] = {
    0x35U, 0xABU, 0x87U, 0x61U, 0x8CU, 0x9FU, 0xC0U, 0x9EU, 0x53U, 0xA6U, 
    0x7FU, 0x4CU, 0xB1U, 0x94U, 0xFCU, 0x4BU, 0x51U, 0xD3U, 0x27U, 0xECU, 
    0x5DU, 0x21U, 0xC6U, 0x95U, 0x8BU, 0x00U, 0xA6U, 0x23U, 0xD6U, 0xF8U, 
    0xECU, 0x1FU
};

// P2P namespace: 01B1F0D62228AF74081BA352D34CD62005D0FA355E13D46B8D5626B59CCD0D43
uint8_t p2p_ns[32] = {
    0x01U, 0xB1U, 0xF0U, 0xD6U, 0x22U, 0x28U, 0xAFU, 0x74U, 0x08U, 0x1BU, 
    0xA3U, 0x52U, 0xD3U, 0x4CU, 0xD6U, 0x20U, 0x05U, 0xD0U, 0xFAU, 0x35U, 
    0x5EU, 0x13U, 0xD4U, 0x6BU, 0x8DU, 0x56U, 0x26U, 0xB5U, 0x9CU, 0xCDU, 
    0x0DU, 0x43U
};

// P2P account: 12102404901E288D25AD11D3EA55A05BBA077F4D
uint8_t p2p_account[20] = {
    0x12U, 0x10U, 0x24U, 0x04U, 0x90U, 0x1EU, 0x28U, 0x8DU, 0x25U, 0xADU, 
    0x11U, 0xD3U, 0xEAU, 0x55U, 0xA0U, 0x5BU, 0xBAU, 0x07U, 0x7FU, 0x4DU
};

#define YEARLY 6108081094714392576U // 12 months

#define POOL_MODEL 114U
#define ISSUER_OFFSET 1U
#define CURRENCY_OFFSET 21U

#define LOAN_MODEL 126U
#define LOAN_STATE 0U
#define BORROWER_OFFSET 1U
#define GRACE_OFFSET 70U // 72 - 75
#define INTERVAL_OFFSET 74U // 76 - 79
#define START_OFFSET 78U // 80 - 83
#define NEXT_OFFSET 82U // 84 - 87
#define LAST_OFFSET 86U // 88 - 91
#define TOTAL_OFFSET 90U // 90 - 91
#define REMAINING_OFFSET 92U // 92 - 93
#define AMOUNT_OFFSET 94U // 96 - 101
#define REQUESTED_OFFSET 102U // 102 - 109
#define ENDING_OFFSET 110U // 110 - 117
#define DRAWABLE_OFFSET 118U // 118 - 126

int64_t hook(uint32_t r)
{
    _g(1,1);

    uint8_t hook_accid[32];
    hook_account(hook_accid + 12, 20);

    uint8_t otxn_accid[32];
    otxn_field(otxn_accid + 12, 20, sfAccount);

    int64_t tt = otxn_type();
    if (tt != ttINVOKE && tt != ttPAYMENT && tt != ttREMIT)
        NOPE("loan.c: Rejecting non-Invoke, non-Payment, non-remit txn.");

    // if (!BUFFER_EQUAL_20(hook_accid + 12, otxn_accid + 12) && tt == ttINVOKE)
    //     DONE("loan.c: passing incoming invoke txn");

    uint8_t amount_buffer[48];
    otxn_slot(1);

    int64_t amount_xfl;
    uint32_t flags;
    if (tt == ttPAYMENT)
    {
        slot_subfield(1, sfAmount, 2);

        // this will fail if flags isn't in the txn, that's also ok.
        otxn_field(&flags, 4, sfFlags);
        
        // check for partial payments (0x00020000) -> (0x00000200 LE)
        if (flags & 0x200U)
            NOPE("native_fund_nfo.c: Partial payments are not supported.");

        otxn_field(SBUF(amount_buffer), sfAmount);

        amount_xfl = slot_float(2);

        if (amount_xfl < 0 || !float_compare(amount_xfl, 0, COMPARE_GREATER))
            NOPE("native_fund_nfo.c: Invalid sfAmount.");
    }

    int64_t total_outstanding;
    state(SVAR(total_outstanding), outstanding_key, 32);

    // Operation
    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1)
        NOPE("loan.c: Missing OP parameter on Invoke.");

    if ((op == 'P' || op == 'S') && tt != ttPAYMENT)
        NOPE("loan.c: Payment/Security operation requires a Payment transaction.");

    // TXN: PREPARE: Init
    etxn_reserve(1);

    // TXN PREPARE: FirstLedgerSequence
    uint32_t fls = (uint32_t)ledger_seq() + 1;
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);

    // TXN PREPARE: LastLedgerSequense
    uint32_t lls = fls + 4;
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);

    uint8_t pool_model[POOL_MODEL];
    if (state_foreign(SBUF(pool_model), hook_accid + 12, 20, SBUF(pool_ns),
                      hook_accid + 12, 20) == DOESNT_EXIST) {
      NOPE("loan.c: Loan Pool does not exist.");
    }

    uint8_t loan_model[LOAN_MODEL];
    uint8_t uri_hash[32];
    uint8_t loan_hash[32];
    if (op == 'P' || op == 'R')
    {
        if (otxn_param(SBUF(uri_hash), "URID", 4) != 32)
            NOPE("loan.c: Missing URID parameter on Invoke.");

        uint8_t urit_buff[34];
        util_keylet(SBUF(urit_buff), KEYLET_UNCHECKED, uri_hash, 32, 0, 0, 0, 0);
        if (slot_set(SBUF(urit_buff), 1) != 1)
            rollback(SBUF("loan.c: Could not load keylet"), __LINE__);
        if (slot_subfield(1, sfOwner, 2) != 2)
            rollback(SBUF("loan.c: Could not load sfOwner"), __LINE__);
        if (slot_subfield(1, sfDigest, 3) != 3)
            rollback(SBUF("loan.c: Could not load sfDigest"), __LINE__);
        
        uint8_t loan_owner[20];
        slot(SBUF(loan_owner), 2);
        if (!BUFFER_EQUAL_20(loan_owner, otxn_accid + 12))
            NOPE("loan.c: No Permission for Operation.");

        slot(SBUF(loan_hash), 3);
        if (state(SBUF(loan_model), SBUF(loan_hash)) == DOESNT_EXIST)
        {
            NOPE("loan.c: Loan does not exist.");
        }
    }

    // action
    switch (op)
    {
        case 'C': // create loan
        {
            if (slot_subfield(1, sfURITokenIDs, 2) != 2)
                NOPE("loan.c: Missing URITokenIDs.");

            uint8_t uri_hash[34];
            slot(SBUF(uri_hash), 2);
            
            // check how many currencies were sent
            int64_t sent_currency_count = slot_subfield(1, sfAmounts, 4) == 4
                ? slot_count(4)
                : 0;
            
            if (sent_currency_count != 1)
                NOPE("loan.c: Invalid number of currencies sent.");

            if (slot_subarray(4, 0, 5) != 5)
                NOPE("loan.c: Error slotting currency");

            slot_subfield(5, sfAmount, 6);
            slot(SBUF(amount_buffer), 6);

            amount_xfl = slot_float(6);

            if (amount_xfl < 0 || !float_compare(amount_xfl, 0, COMPARE_GREATER))
                NOPE("loan.c: Invalid sfAmount.");
            
            uint8_t urit_buff[34];
            util_keylet(SBUF(urit_buff), KEYLET_UNCHECKED, uri_hash + 1, 32, 0, 0, 0, 0);
            if (slot_set(SBUF(urit_buff), 1) != 1)
                rollback(SBUF("loan.c: Could not load keylet"), __LINE__);
            if (slot_subfield(1, sfOwner, 2) != 2)
                rollback(SBUF("loan.c: Could not load sfOwner"), __LINE__);
            if (slot_subfield(1, sfDigest, 3) != 3)
                rollback(SBUF("loan.c: Could not load sfDigest"), __LINE__);
            
            uint8_t urit_owner[20];
            slot(SBUF(urit_owner), 2);

            uint8_t urit_digest[32];
            slot(SBUF(urit_digest), 3);

            uint8_t _loan_model[LOAN_MODEL];
            if (state(SBUF(_loan_model), SBUF(urit_digest)) != DOESNT_EXIST)
            {
                NOPE("loan.c: Loan already exists.");
            }
            
            if(state_foreign(SBUF(_loan_model), SBUF(urit_digest), SBUF(p2p_ns), SBUF(p2p_account)) < 0)
            {
                NOPE("loan.c: Could not get Loan from P2P.");
            }

            if(state_set(SBUF(_loan_model), SBUF(urit_digest)) < 0)
            {
                NOPE("loan.c: Could not create Loan.");
            }

            // Update Total Outstanding
            int64_t requested_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(_loan_model + REQUESTED_OFFSET));
            total_outstanding = float_sum(total_outstanding, requested_xfl);
            state_set(SVAR(total_outstanding), outstanding_key, 32);

            // Get the current collateral state
            int64_t collateral_xfl[8];
            state_foreign(SVAR(collateral_xfl), SBUF(urit_digest), collateral_ns, 32, hook_accid + 12, 20);

            // FAILURE
            // The Asset deposited is NOT equal to the asset of the Lending Pool.
            TRACEHEX(amount_buffer);
            // VALIDATE: Asset Issuer
            if (BUFFER_EQUAL_20(amount_buffer + 8, pool_model + ISSUER_OFFSET))
            {
                    NOPE("loan.c: Collateral and Lending Pool Amount.issuer are the same.");
            }

            // VALIDATE: Asset Currency
            if (BUFFER_EQUAL_20(amount_buffer + 28, pool_model + CURRENCY_OFFSET))
            {
                    NOPE("loan.c: Collateral and Lending Pool Amount.currency are the same.");
            }

            // Increment the collateral
            int64_t final_collateral = float_sum(collateral_xfl, amount_xfl);

            // Store the Collateral in state
            state_foreign_set(SVAR(final_collateral), SBUF(urit_digest), collateral_ns, 32, hook_accid + 12, 20);
            DONE("loan.c: Created Loan.");
        }

        case 'E': // end loan
        {
            // VALIDATE: After Loan Start Date
            uint32_t start_date = UINT32_FROM_BUF(loan_model + START_OFFSET);
            uint32_t current_time = ledger_last_time();
            if (current_time < start_date)
            {
                NOPE("loan.c: Loan close before start date.");
            }

            uint32_t next_payment = UINT32_FROM_BUF(loan_model + NEXT_OFFSET);
            TRACEVAR(next_payment);

            // Determine if this loan is in default
            uint32_t grace_period = UINT32_FROM_BUF(loan_model + GRACE_OFFSET);
            int8_t is_default = current_time > next_payment + grace_period;
            TRACEVAR(is_default)
            if (is_default)
            {
                // Update the loan state
                UINT8_TO_BUF(loan_model + LOAN_STATE, 2);
                state_set(SBUF(loan_model), SBUF(loan_hash));
                DONE("loan.c: Loan Defaulted.");
            }

            // Get the outstanding principal
            int64_t drawable_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(loan_model + DRAWABLE_OFFSET));
            TRACEVAR(drawable_xfl)
            int64_t requested_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(loan_model + REQUESTED_OFFSET));
            TRACEVAR(requested_xfl)
            int64_t outstanding_xfl = float_sum(float_negate(drawable_xfl), requested_xfl);
            TRACEVAR(outstanding_xfl)
            // VALIDATE: Outstanding balance must be 0
            if (outstanding_xfl > 0)
            {
                NOPE("loan.c: Loan close before outstanding balance is paid.");
            }
            // Return Collateral
            int64_t collateral_min_xfl = 10;
            TRACEVAR(collateral_min_xfl);
            int64_t collateral_xfl = float_multiply(requested_xfl, collateral_min_xfl);
            TRACEVAR(collateral_xfl);
            // Emit the Collateral back to the provider
            float_sto(AMOUNT_OUT, 49, pool_model + CURRENCY_OFFSET, 20,
                        pool_model + ISSUER_OFFSET, 20, collateral_xfl,
                        sfAmount);
            PREPARE_REMIT_TXN(hook_accid + 12, otxn_accid + 12);
            // TXN: Emit/Send Txn
            uint8_t emithash[32];
            int64_t emit_result = emit(SBUF(emithash), SBUF(txn));
            TRACEVAR(emit_result)
            if (emit_result > 0)
            {
                // Update the loan state
                UINT8_TO_BUF(loan_model + LOAN_STATE, 2);
                state_set(SBUF(loan_model), SBUF(loan_hash));
                accept(SBUF("loan.c: Transaction Success (End Loan)."), __LINE__);
            }
            NOPE("loan.c: Transaction Failed (End Loan).");
        }

        case 'P': // payback loan
        {
            TRACEHEX(loan_model);
            // VALIDATE: After Loan Start Date
            uint32_t start_date = UINT32_FROM_BUF(loan_model + START_OFFSET);
            uint32_t current_time = ledger_last_time();
            TRACEVAR(current_time);
            if (current_time < start_date)
            {
                NOPE("loan.c: Loan payment before start date.");
            }

            uint16_t total_payments = UINT16_FROM_BUF(loan_model + TOTAL_OFFSET);
            TRACEVAR(total_payments);

            uint16_t payments_remaining = UINT16_FROM_BUF(loan_model + REMAINING_OFFSET);
            TRACEVAR(payments_remaining);
            if (payments_remaining == 0)
            {
                NOPE("loan.c: Loan already paid off (No payments remaining).");
            }
            
            uint32_t payment_interval = UINT32_FROM_BUF(loan_model + INTERVAL_OFFSET);
            TRACEVAR(payment_interval);

            uint32_t next_due_date = UINT32_FROM_BUF(loan_model + NEXT_OFFSET);
            TRACEVAR(next_due_date);

            int8_t is_late = current_time > next_due_date;
            TRACEVAR(is_late);

            // Get the outstanding principal
            int64_t drawable_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(loan_model + DRAWABLE_OFFSET));
            TRACEVAR(drawable_xfl)
            int64_t requested_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(loan_model + REQUESTED_OFFSET));
            TRACEVAR(requested_xfl)
            int64_t outstanding_xfl = float_sum(float_negate(drawable_xfl), requested_xfl);
            TRACEVAR(outstanding_xfl)

            if (payments_remaining > 0 && outstanding_xfl <= 0)
            {
                DONE("loan.c: Loan already paid off (outstanding balance <= 0).");
            }

            // Get Monthly Payment
            int64_t req_monthly_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(loan_model + AMOUNT_OFFSET));
            TRACEVAR(req_monthly_xfl)
            // Get Interest Rate
            int64_t rate_xfl = 6071852297695428608; // 0.1 XFL
            int64_t interval_rate_xfl = float_divide(rate_xfl, float_divide(YEARLY, float_set(0, payment_interval)));
            TRACEVAR(interval_rate_xfl)
            // Calculate the interest amount
            int64_t interest_xfl = float_multiply(outstanding_xfl, interval_rate_xfl);
            TRACEVAR(interest_xfl)
            if (payments_remaining == 1)
            {
                req_monthly_xfl = float_sum(interest_xfl, outstanding_xfl);
            }

            // VALIDATE: Min Payment must be greater than interest
            if (!float_compare(amount_xfl, req_monthly_xfl, COMPARE_GREATER | COMPARE_EQUAL))
            {
                NOPE("loan.c: Invalid payment amount (payment < monthly payment).");
            }

            int64_t late_fee = 6089866696204910592; // 1 XFL
            if (is_late)
            {
                // VALIDATE: Min Payment when late must be greater than requested monthly + late fee
                int64_t min_due = float_sum(req_monthly_xfl, late_fee);
                if (!float_compare(amount_xfl, min_due, COMPARE_GREATER | COMPARE_EQUAL))
                {
                    NOPE("loan.c: Invalid payment amount (monthly payment + late fee).");
                }
            }

            // Calculate the principal amount
            // If late, subtract the late fee from the payment
            int64_t payment_xfl = is_late ? float_sum(amount_xfl, float_negate(late_fee)) : amount_xfl;
            int64_t principal_xfl = float_sum(payment_xfl, float_negate(interest_xfl));
            TRACEVAR(principal_xfl)

            payments_remaining -= 1;
            UINT16_TO_BUF(loan_model + REMAINING_OFFSET, payments_remaining);
            // uint32_t next_date = current_time + (payment_interval * 86400 * 30);
            next_due_date = next_due_date + (payment_interval * 300);
            TRACEVAR(next_due_date);
            UINT32_TO_BUF(loan_model + NEXT_OFFSET, next_due_date);
            TRACEVAR(payments_remaining)

            UINT32_TO_BUF(loan_model + LAST_OFFSET, current_time);

            // Update the drawable amount
            drawable_xfl = float_sum(drawable_xfl, principal_xfl);
            TRACEVAR(drawable_xfl)
            UINT64_TO_BUF(loan_model + DRAWABLE_OFFSET, FLIP_ENDIAN_64(drawable_xfl));
            // Update the loan state
            state_set(SBUF(loan_model), SBUF(loan_hash));
            DONE("loan.c: Payback Loan.");
        }

        case 'R': // receive loan
        {
            int64_t drawable_funds = FLIP_ENDIAN_64(UINT64_FROM_BUF(loan_model + DRAWABLE_OFFSET));
            TRACEVAR(drawable_funds);
            uint8_t amt_buf[48];
            if (otxn_param(SBUF(amt_buf), "AMT", 3) < 0)
                NOPE("loan.c: Missing AMT parameter.");
            
            int64_t amt_xfl = *((int64_t *)amt_buf);
            TRACEVAR(amt_xfl);
            
            if (amt_xfl < 0 || !float_compare(amt_xfl, 0, COMPARE_GREATER))
                NOPE("loan.c: Invalid sfAmount.");

            if (!float_compare(amt_xfl, drawable_funds, COMPARE_LESS | COMPARE_EQUAL))
                NOPE("loan.c: Insufficient funds (amount requested > drawable funds).");

            TRACEHEX(pool_model + CURRENCY_OFFSET)
            TRACEHEX(pool_model + ISSUER_OFFSET)

            float_sto(AMOUNT_OUT, 49, pool_model + CURRENCY_OFFSET, 20,
                        pool_model + ISSUER_OFFSET, 20, amt_xfl,
                        sfAmount);
            PREPARE_REMIT_TXN(hook_accid + 12, otxn_accid + 12);
            // TXN: Emit/Send Txn
            uint8_t emithash[32];
            int64_t emit_result = emit(SBUF(emithash), SBUF(txn));
            TRACEVAR(emit_result)
            if (emit_result > 0)
            {
                int64_t final_drawable = float_sum(drawable_funds, float_negate(amt_xfl));
                TRACEVAR(final_drawable);
                INT64_TO_BUF(loan_model + DRAWABLE_OFFSET, FLIP_ENDIAN_64(final_drawable));
                state_set(SBUF(loan_model), SBUF(loan_hash));
                accept(SBUF("loan.c: Transaction Success (Receive Loan)."), __LINE__);
            }
            NOPE("loan.c: Transaction Failed (Receive Loan).");
        }

        default:
        {
            NOPE("loan.c: Unknown Loan operation.");
        }
    }

    return 0;
}