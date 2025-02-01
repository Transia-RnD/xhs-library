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

// -----------------------------------------------------------------------------

/**
 * 
 * These functions should be moved into the macro.c file
*/

#define SVAR(x) &(x), sizeof(x)

#define ACCOUNT_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    *(uint64_t*)(buf + 0) = *(uint64_t*)(i +  0);\
    *(uint64_t*)(buf + 8) = *(uint64_t*)(i +  8);\
    *(uint32_t*)(buf + 16) = *(uint32_t*)(i + 16);\
}

#define FLIP_ENDIAN_64(n) ((uint64_t)(((n & 0xFFULL) << 56ULL) |             \
                                      ((n & 0xFF00ULL) << 40ULL) |           \
                                      ((n & 0xFF0000ULL) << 24ULL) |         \
                                      ((n & 0xFF000000ULL) << 8ULL) |        \
                                      ((n & 0xFF00000000ULL) >> 8ULL) |      \
                                      ((n & 0xFF0000000000ULL) >> 24ULL) |   \
                                      ((n & 0xFF000000000000ULL) >> 40ULL) | \
                                      ((n & 0xFF00000000000000ULL) >> 56ULL)))

#define NOPE(x)                                    \
    {                                              \
        return rollback((x), sizeof(x), __LINE__); \
    }


// -----------------------------------------------------------------------------

// clang-format off 
uint8_t ctxn[229] =
{
/* size,upto */
/* 3,    0, tt = ClaimReward      */   0x12U, 0x00U, 0x62U,
/* 5,    3  flags = tfCanonical   */   0x22U, 0x80U, 0x00U, 0x00U, 0x00U,
/* 5,    8, sequence              */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 6,   13, firstledgersequence   */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,
/* 6,   19, lastledgersequence    */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,
/* 9,   25, fee                   */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 35,  34, signingpubkey         */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 22,  69, account               */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 22,  91, issuer                */   0x84U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 116, 113  emit details          */  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 0,  229                        */ 
};
// clang-format on

// TX BUILDER
#define CFLAGS_OUT (ctxn + 4U) // + leading bytes (1)
#define CFLS_OUT (ctxn + 15U) // + leading bytes (2)
#define CLLS_OUT (ctxn + 21U) // + leading bytes (2)
#define CFEE_OUT (ctxn + 26U) // + leading bytes (1)
#define CACCOUNT_OUT (ctxn + 71U) // + leading bytes (2)
#define CISSUER_OUT (ctxn + 93U) // + leading bytes (2)
#define CEMIT_OUT (ctxn + 113U) // + leading bytes (0)

// clang-format off
uint8_t txn[6000] =
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
#define PREPARE_REMIT_TXN(account_buffer, dest_buffer, txn_size) do { \
    if (otxn_field(DTAG_OUT, 4, sfSourceTag) == 4) \
        *(DTAG_OUT - 1) = 0x2EU; \
    uint32_t fls = (uint32_t)ledger_seq() + 1; \
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls); \
    uint32_t lls = fls + 4; \
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls); \
    ACCOUNT_TO_BUF(HOOK_ACC, account_buffer); \
    ACCOUNT_TO_BUF(OTX_ACC, dest_buffer); \
    etxn_details(EMIT_OUT, 116U); \
    TRACEHEX(txn); \
    int64_t fee = etxn_fee_base(txn, txn_size); \
    uint8_t *b = FEE_OUT; \
    *b++ = 0b01000000 + ((fee >> 56) & 0b00111111); \
    *b++ = (fee >> 48) & 0xFFU; \
    *b++ = (fee >> 40) & 0xFFU; \
    *b++ = (fee >> 32) & 0xFFU; \
    *b++ = (fee >> 24) & 0xFFU; \
    *b++ = (fee >> 16) & 0xFFU; \
    *b++ = (fee >> 8) & 0xFFU; \
    *b++ = (fee >> 0) & 0xFFU; \
    TRACEHEX(txn); \
} while(0) 
// clang-format on

// TXN PREPARE: Currency - NAV
uint8_t curr_nav[20] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x4EU, 0x41U, 0x56U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

#define MIN_INVESTORS 20
#define INVESTMENT_AMT 6147909891733356544 // 5000 XAH
#define CLOSED_OFFERING 1                  // open or closed (cannot purchase after close)
#define START_LEDGER 10                    // ledger index
#define END_LEDGER 14                      // ledger index

int64_t hook(uint32_t reserved)
{

    TRACESTR("native_fund_nfo.c: Called.");
    _g(1, 1);

    int64_t TXN_SIZE = 289;

    // ACCOUNT: Origin Tx Account
    uint8_t otxn_accid[32];
    otxn_field(otxn_accid + 12, 20, sfAccount);

    // ACCOUNT: Dest Tx Account
    uint8_t dest_buf[48];
    otxn_field(dest_buf, 20, sfDestination);

    // ACCOUNT: Hook Account
    uint8_t hook_accid[32];
    hook_account(hook_accid + 12, 20);

    // FILTER ON: ACCOUNT
    if (BUFFER_EQUAL_20(hook_accid + 12, otxn_accid + 12))
        DONE("native_fund_nfo.c: outgoing tx on `Account`.");

    int64_t tt = otxn_type();
    if (tt != ttINVOKE && tt != ttPAYMENT)
        NOPE("native_fund_nfo.c: Rejecting non-Invoke, non-Payment txn.");

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
            NOPE("native_fund_nfo.c: Partial payments are not supported.");

        otxn_field(SBUF(amount_buffer), sfAmount);

        amount_xfl = slot_float(2);

        if (amount_xfl < 0 || !float_compare(amount_xfl, 0, COMPARE_GREATER))
            NOPE("native_fund_nfo.c: Invalid sfAmount.");
    }

    // Operation
    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1)
        NOPE("native_fund_nfo.c: Missing OP parameter on Invoke.");

    // sanity check
    if ((op == 'G' || op == 'D' || op == 'W') && tt != ttPAYMENT)
        NOPE("native_fund_nfo.c: Gift || Deposit || Withdraw operations must be a payment transaction.");
    
    // sanity check
    if ((op == 'C') && tt != ttINVOKE)
        NOPE("native_fund_nfo.c: Claim operations must be a invoke transaction.");
    
    int64_t total_nav;
    state(SVAR(total_nav), hook_accid + 12, 20);

    int64_t ll_seq = ledger_seq();
    int64_t nfo_ll = END_LEDGER;
    int64_t nfo_ended = ll_seq > nfo_ll;
    if (ll_seq < START_LEDGER)
    {
        NOPE("native_fund_nfo.c: NFO has not started.");
    }

    // KEYLET: Account Root
    uint8_t bal_kl[34];
    util_keylet(SBUF(bal_kl), KEYLET_ACCOUNT, hook_accid + 12, 20, 0,0,0,0);

    // SLOT KEYLET: Account Root
    if (slot_set(SBUF(bal_kl), 20) != 20)
        accept(SBUF("Savings: Could not load target balance"), __LINE__);

    if (slot_subfield(20, sfBalance, 20) != 20)
        accept(SBUF("Savings: Could not load target balance 2"), __LINE__);

    int64_t balance_xfl = slot_float(20);

    // TXN: PREPARE: Init
    etxn_reserve(2);

    // TXN PREPARE: FirstLedgerSequence
    uint32_t fls = (uint32_t)ledger_seq() + 1;
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);

    // TXN PREPARE: LastLedgerSequense
    uint32_t lls = fls + 4;
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);

    switch (op)
    {
    case 'C':
        TRACESTR("native_fund_nfo.c: Claim.");

        // OTXN PARAM: Flags
        int32_t flags;
        otxn_param(SVAR(flags), "F", 1);
        TRACEVAR(flags);

        // OTXN PARAM: Issuer
        uint8_t issuer[20];
        otxn_param(SBUF(issuer), "I", 1);
        TRACEHEX(issuer);

        // if (issuer && flags)
        // {
        //     NOPE("native_fund_nfo.c: Cannot supply issuer and flags.");
        // }

        ACCOUNT_TO_BUF(CACCOUNT_OUT, hook_accid + 12);

        ACCOUNT_TO_BUF(CISSUER_OUT, issuer);

        // TXN PREPARE: FirstLedgerSequence
        uint32_t fls = (uint32_t)ledger_seq() + 1;
        *((uint32_t *)(CFLS_OUT)) = FLIP_ENDIAN(fls);

        // TXN PREPARE: LastLedgerSequense
        uint32_t lls = fls + 4;
        *((uint32_t *)(CLLS_OUT)) = FLIP_ENDIAN(lls);

        *((uint32_t *)(CFLAGS_OUT)) = FLIP_ENDIAN(flags);

        // TXN PREPARE: Emit Metadata
        etxn_details(CEMIT_OUT, 116U);

        // TXN PREPARE: Fee
        {
            int64_t fee = etxn_fee_base(SBUF(ctxn));
            uint8_t *b = CFEE_OUT;
            *b++ = 0b01000000 + ((fee >> 56) & 0b00111111);
            *b++ = (fee >> 48) & 0xFFU;
            *b++ = (fee >> 40) & 0xFFU;
            *b++ = (fee >> 32) & 0xFFU;
            *b++ = (fee >> 24) & 0xFFU;
            *b++ = (fee >> 16) & 0xFFU;
            *b++ = (fee >> 8) & 0xFFU;
            *b++ = (fee >> 0) & 0xFFU;
        }

        TRACEHEX(txn); // <- final tx blob

        // TXN: Emit/Send Txn
        uint8_t emithash[32];
        int64_t emit_result = emit(SBUF(emithash), SBUF(ctxn));
        if (emit_result > 0)
        {
            accept(SBUF("native_fund_nfo.c: Claim Complete."), __LINE__);
        }
        return accept(SBUF("native_fund_nfo.c: Claim Failure."), __LINE__);
    
    case 'G':
        TRACESTR("native_fund_nfo.c: Gift.");
        int64_t total_nav;
        state(SVAR(total_nav), otxn_accid + 12, 20);
        total_nav += amount_xfl;
        state_set(SVAR(total_nav), otxn_accid + 12, 20);
        return accept(SBUF("native_fund_nfo.c: Gift Received."), __LINE__);
    
    case 'D':
        TRACESTR("native_fund_nfo.c: Deposit.");
        uint8_t owner_accid[32];
        if (otxn_param(SBUF(owner_accid), "ACC", 3) != 20)
            NOPE("native_fund_nfo.c: Missing owner parameter.");

        uint8_t keylet[34];
        if (util_keylet(keylet, 34, KEYLET_ACCOUNT, owner_accid, 20, 0, 0, 0, 0) != 34)
            rollback(SBUF("native_fund_nfo.c: Fetching Keylet Failed."), 8);

        if (slot_set(SBUF(keylet), 1) == DOESNT_EXIST)
            rollback(SBUF("native_fund_nfo.c: `ACC` Does Not Exist."), 9);

        // Remit the new amount to the owner
        float_sto(AMOUNT_OUT, 49, curr_nav, 20, hook_accid + 12, 20, amount_xfl, sfAmount); 
        PREPARE_REMIT_TXN(hook_accid + 12, owner_accid + 12, TXN_SIZE);

        // TXN: Emit/Send Txn
        uint8_t emithash[32];
        int64_t emit_result = emit(SBUF(emithash), txn, TXN_SIZE);
        if (emit_result > 0)
        {
            TRACEVAR(total_nav);
            if (total_nav <= 0)
            {
                // delete all entries
                state_set(0,0, hook_accid + 12, 20);
            }
            else
            {
                state_set(SVAR(total_nav), hook_accid + 12, 20);
            }
            return accept(SBUF("native_fund_nfo.c: Transaction Complete."), __LINE__);
        }
        return rollback(SBUF("native_fund_nfo.c: Transaction Failed."), __LINE__);
        break;
    
    case 'W':
        TRACESTR("native_fund_nfo.c: Withdraw.");

        // check if the trustline exists
        uint8_t keylet[34];
        util_keylet(keylet, 34, KEYLET_LINE, otxn_accid + 12, 20, hook_accid + 12, 20, curr_nav, 20);
        if (slot_set(SBUF(keylet), 10) != 10)
            DONE("native_fund_nfo.c: Invalid trustline.");

        // check trustline balance
        slot_subfield(10, sfBalance, 11);
        if (slot_size(11) != 48)
            NOPE("native_fund_nfo.c: Could not fetch trustline balance.");

        uint8_t low_limit[48];
        if (slot_subfield(10, sfLowLimit, 13) != 13)
            NOPE("native_fund_nfo.c: Could not slot subfield `sfLowLimit`");

        if (slot(SVAR(low_limit), 13) != 48)
            NOPE("native_fund_nfo.c: Could not slot `sfLowLimit`");

        int64_t owner_nav = slot_float(11);

        owner_nav = float_sign(owner_nav) ? float_negate(owner_nav) : owner_nav;

        TRACEVAR(owner_nav);
        TRACEVAR(total_nav);

        // can't withdraw if you don't have tokens
        if (!float_compare(owner_nav, 0, COMPARE_GREATER))
            NOPE("native_fund_nfo.c: You don't have any liquidity tokens in this pool.");

        // if they are more than total somehow, just assume they own all of them
        if (float_compare(owner_nav, total_nav, COMPARE_GREATER))
            owner_nav = total_nav;

        int64_t ownership_percent = float_divide(owner_nav, total_nav);
        if (ownership_percent < 0)
            NOPE("native_fund_nfo.c: Error computing ownership %");

        TRACEVAR(ownership_percent);
        int64_t out_xah_xfl = float_multiply(balance_xfl, ownership_percent);
        TRACEVAR(out_xah_xfl);

        total_nav -= owner_nav;

        TRACEVAR(total_nav);

        uint64_t drops = float_int(out_xah_xfl, 6, 1);
        uint8_t* b = AMOUNT_OUT + 1;
        *b++ = 0b01000000 + (( drops >> 56 ) & 0b00111111 );
        *b++ = (drops >> 48) & 0xFFU;
        *b++ = (drops >> 40) & 0xFFU;
        *b++ = (drops >> 32) & 0xFFU;
        *b++ = (drops >> 24) & 0xFFU;
        *b++ = (drops >> 16) & 0xFFU;
        *b++ = (drops >>  8) & 0xFFU;
        *b++ = (drops >>  0) & 0xFFU;
        txn[247] = 0xE1U;
        txn[248] = 0xF1U;
        TXN_SIZE = 249;
        PREPARE_REMIT_TXN(hook_accid + 12, otxn_accid + 12, TXN_SIZE);

        // TXN: Emit/Send Txn
        uint8_t emithash[32];
        int64_t emit_result = emit(SBUF(emithash), txn, TXN_SIZE);
        if (emit_result > 0)
        {
            TRACEVAR(total_nav);
            if (total_nav <= 0)
                state_set(0,0, hook_accid + 12, 20);
            else
                state_set(SVAR(total_nav), hook_accid + 12, 20);
            
            accept(SBUF("native_fund_nfo.c: Transaction Complete."), __LINE__);
        }
        rollback(SBUF("native_fund_nfo.c: Transaction Failed."), __LINE__);
    }
    return 1;
}