//------------------------------------------------------------------------------
/*

Operations:

// invoke (Pool) ops are:
    // C - create pool - 
    // U - update pool - 
    // D - delete pool - 

// invoke (Liquidity) ops are:
    // D - deposit liquidity - 
    // W - withdraw liquidity - 


*/
//==============================================================================

#include "hookapi.h"

#define SVAR(x) &(x), sizeof(x)

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

// TXN PREPARE: Currency - NAV
uint8_t curr_nav[20] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x4EU, 0x41U, 0x56U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

// liquidity namespace: D6F390A642BEEC7DD0A9F450FC9FC61324683A3BEFCDD398A01BD4995D8BFBB6
uint8_t liquidity_ns[32] = {
    0xD6U, 0xF3U, 0x90U, 0xA6U, 0x42U, 0xBEU, 0xECU, 0x7DU, 0xD0U, 0xA9U,
    0xF4U, 0x50U, 0xFCU, 0x9FU, 0xC6U, 0x13U, 0x24U, 0x68U, 0x3AU, 0x3BU,
    0xEFU, 0xCDU, 0xD3U, 0x98U, 0xA0U, 0x1BU, 0xD4U, 0x99U, 0x5DU, 0x8BU,
    0xFBU, 0xB6U};

// loan namespace: 470CC828CA6A4681034F21E22CECBEEC8BB450EB204559C82932B28F3DFB19BC
uint8_t loan_ns[32] = {
    0x47U, 0x0CU, 0xC8U, 0x28U, 0xCAU, 0x6AU, 0x46U, 0x81U, 0x03U, 0x4FU,
    0x21U, 0xE2U, 0x2CU, 0xECU, 0xBEU, 0xECU, 0x8BU, 0xB4U, 0x50U, 0xEBU,
    0x20U, 0x45U, 0x59U, 0xC8U, 0x29U, 0x32U, 0xB2U, 0x8FU, 0x3DU, 0xFBU,
    0x19U, 0xBCU};

// outstanding key: 35AB87618C9FC09E53A67F4CB194FC4B51D327EC5D21C6958B00A623D6F8EC1F
uint8_t outstanding_key[32] = {
    0x35U, 0xABU, 0x87U, 0x61U, 0x8CU, 0x9FU, 0xC0U, 0x9EU, 0x53U, 0xA6U, 
    0x7FU, 0x4CU, 0xB1U, 0x94U, 0xFCU, 0x4BU, 0x51U, 0xD3U, 0x27U, 0xECU, 
    0x5DU, 0x21U, 0xC6U, 0x95U, 0x8BU, 0x00U, 0xA6U, 0x23U, 0xD6U, 0xF8U, 
    0xECU, 0x1FU
};

#define POOL_MODEL 114U
#define ISSUER_OFFSET 1U
#define CURRENCY_OFFSET 21U

int64_t hook(uint32_t r)
{
    _g(1,1);

    uint8_t hook_accid[32];
    hook_account(hook_accid + 12, 20);

    uint8_t otxn_accid[32];
    otxn_field(otxn_accid + 12, 20, sfAccount);

    int64_t tt = otxn_type();
    if (tt != ttINVOKE && tt != ttPAYMENT)
        NOPE("pool.c: Rejecting non-Invoke, non-Payment txn.");

    if (!BUFFER_EQUAL_20(hook_accid + 12, otxn_accid + 12) && tt == ttINVOKE)
        DONE("pool.c: passing incoming invoke txn");

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

    int64_t total_nav = float_one();
    state_foreign(SVAR(total_nav), hook_accid + 12, 20, SBUF(liquidity_ns), hook_accid + 12, 20);
    TRACEVAR(total_nav);

    // Operation
    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1)
        NOPE("pool.c: Missing OP parameter on Invoke.");

    // Sub Operation
    uint8_t sop;
    if ((op == 'P' || op == 'L') && otxn_param(&sop, 1, "SOP", 3) != 1)
        NOPE("User: Missing SOP parameter on Pool or Liquidity Operation.");

    // TXN: PREPARE: Init
    etxn_reserve(1);

    // TXN PREPARE: FirstLedgerSequence
    uint32_t fls = (uint32_t)ledger_seq() + 1;
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);

    // TXN PREPARE: LastLedgerSequense
    uint32_t lls = fls + 4;
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);

    uint8_t pool_model[POOL_MODEL];
    if (op == 'P' && (sop == 'U' || sop == 'D') || op == 'L')
    {
        if (state(SBUF(pool_model), hook_accid + 12, 20) == DOESNT_EXIST)
        {
            NOPE("loan.c: Pool does not exist.");
        }
    }

    // action
    switch (op)
    {
        case 'P':
        {
            // action
            switch (sop)
            {
                case 'C': // create pool
                {
                    uint8_t pool_model[POOL_MODEL];
                    if (state(SBUF(pool_model), hook_accid + 12, 20) != DOESNT_EXIST)
                    {
                        NOPE("pool.c: Pool already exists.");
                    }
                    
                    otxn_param(SBUF(pool_model), "PM", 2);

                    state_set(SBUF(pool_model), hook_accid + 12, 20);
                    DONE("pool.c: Created Pool.");
                }

                case 'U': // update pool
                {
                    DONE("pool.c: Updated Pool.");
                }

                case 'D': // delete pool
                {
                    DONE("pool.c: Deleted Pool.");
                }

                default:
                {
                    NOPE("pool.c: Unknown Pool operation.");
                }
            }
        }
        case 'L':
        {
            // action
            switch (sop)
            {
                case 'D': // deposit liquidity
                {
                    TRACESTR("pool.c: Deposit Liquidity.");
                    
                    int64_t _iop = float_divide(amount_xfl, total_nav);
                    TRACEVAR(_iop);
                    if (_iop < 0)
                        NOPE("native_fund_nfo.c: Error computing ownership %");

                    int64_t _owner_nav = float_multiply(total_nav, _iop);
                    TRACEVAR(_owner_nav);
                    total_nav = float_sum(total_nav, _owner_nav);
                    // Calculate Withdrawable
                    int64_t withdrawable = total_nav;
                    // Increase Total Value
                    // Increase Available Liquidity

                    float_sto(AMOUNT_OUT, 49, curr_nav, 20, hook_accid + 12, 20, _owner_nav, sfAmount);
                    PREPARE_REMIT_TXN(hook_accid + 12, otxn_accid + 12);

                    // TXN: Emit/Send Txn
                    uint8_t emithash[32];
                    int64_t emit_result = emit(SBUF(emithash), SBUF(txn));
                    TRACEVAR(emit_result)
                    if (emit_result > 0)
                    {
                        TRACEVAR(total_nav);
                        if (total_nav <= 0)
                        {
                            // delete all entries
                            state_foreign_set(0,0, hook_accid + 12, 20, SBUF(liquidity_ns), hook_accid + 12, 20);
                        }
                        else
                        {
                            // Should be <8 bytes contribution> <8 bytes withdrawable>
                            state_foreign_set(SVAR(_owner_nav), otxn_accid + 12, 20, SBUF(liquidity_ns), hook_accid + 12, 20);
                            state_foreign_set(SVAR(total_nav), hook_accid + 12, 20, SBUF(liquidity_ns), hook_accid + 12, 20);
                        }
                        accept(SBUF("pool.c: Transaction Complete."), __LINE__);
                    }
                    NOPE("pool.c: Transaction Failed.");
                }
                case 'W': // withdraw liquidity
                {
                    TRACESTR("pool.c: Withdraw Liquidity.");
                    // Get OwnerNav from State
                    int64_t owner_nav;
                    state_foreign(SVAR(owner_nav), otxn_accid + 12, 20, SBUF(liquidity_ns), hook_accid + 12, 20);
                    int64_t _iop = float_divide(owner_nav, total_nav);
                    if(_iop < 0)
                        NOPE("native_fund_nfo.c: Error computing ownership %");

                    int64_t total_outstanding;
                    state_foreign(SVAR(total_outstanding), SBUF(outstanding_key), SBUF(loan_ns), hook_accid + 12, 20);
                    int64_t withdrawable = float_multiply(total_outstanding, _iop);

                    TRACEVAR(_iop);
                    
                    uint8_t amt_buf[48];
                    if (otxn_param(SBUF(amt_buf), "AMT", 3) < 0)
                        NOPE("loan.c: Missing AMT parameter.");
                    
                    int64_t amt_xfl = *((int64_t *)amt_buf);
                    TRACEVAR(amt_xfl);
                    
                    if (amt_xfl < 0 || !float_compare(amt_xfl, 0, COMPARE_GREATER))
                        NOPE("loan.c: Invalid sfAmount.");

                    if (!float_compare(owner_nav, amt_xfl, COMPARE_GREATER))
                        NOPE("loan.c: Insufficient funds.");

                    if (!float_compare(withdrawable, amt_xfl, COMPARE_GREATER))
                        NOPE("loan.c: Insufficient withdrawable funds.");

                    int64_t _owner_nav = float_sum(owner_nav, float_negate(amt_xfl));
                    TRACEVAR(_owner_nav);
                    total_nav = float_sum(total_nav, float_negate(amt_xfl));
                    TRACEVAR(total_nav);
                    
                    // USD - USD(Issuer)
                    float_sto(AMOUNT_OUT, 49, pool_model + CURRENCY_OFFSET, 20, pool_model + ISSUER_OFFSET, 20, _owner_nav, sfAmount);
                    PREPARE_REMIT_TXN(hook_accid + 12, otxn_accid + 12);

                    // TXN: Emit/Send Txn
                    uint8_t emithash[32];
                    int64_t emit_result = emit(SBUF(emithash), SBUF(txn));
                    TRACEVAR(emit_result)
                    if (emit_result > 0)
                    {
                        TRACEVAR(total_nav);
                        if (total_nav <= 0)
                        {
                            // delete all entries
                            state_foreign_set(0,0, hook_accid + 12, 20, SBUF(liquidity_ns), hook_accid + 12, 20);
                        }
                        else
                        {
                            // Should be <8 bytes contribution> <8 bytes withdrawable>
                            state_foreign_set(SVAR(_owner_nav), otxn_accid + 12, 20, SBUF(liquidity_ns), hook_accid + 12, 20);
                            state_foreign_set(SVAR(total_nav), hook_accid + 12, 20, SBUF(liquidity_ns), hook_accid + 12, 20);
                        }
                        accept(SBUF("pool.c: Transaction Complete (Withdraw Liquidity)."), __LINE__);
                    }
                    NOPE("pool.c: Transaction Failed (Withdraw Liquidity).");
                }

                default:
                {
                    NOPE("pool.c: Unknown Liquidity operation.");
                }
            }
        }
        default:
        {
            NOPE("pool.c: Unknown operation.");
        }
    }

    return 0;
}