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

#define FLIP_ENDIAN_64(n) ((uint64_t)(((n & 0xFFULL) << 56ULL) |             \
                                      ((n & 0xFF00ULL) << 40ULL) |           \
                                      ((n & 0xFF0000ULL) << 24ULL) |         \
                                      ((n & 0xFF000000ULL) << 8ULL) |        \
                                      ((n & 0xFF00000000ULL) >> 8ULL) |      \
                                      ((n & 0xFF0000000000ULL) >> 24ULL) |   \
                                      ((n & 0xFF000000000000ULL) >> 40ULL) | \
                                      ((n & 0xFF00000000000000ULL) >> 56ULL)))

#define ACCOUNT_TO_BUF(buf_raw, i)                       \
    {                                                    \
        unsigned char *buf = (unsigned char *)buf_raw;   \
        *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0);   \
        *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8);   \
        *(uint32_t *)(buf + 16) = *(uint32_t *)(i + 16); \
    }

#define UINT256_TO_BUF(buf_raw, i)                       \
    {                                                    \
        unsigned char *buf = (unsigned char *)buf_raw;   \
        *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0);   \
        *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8);   \
        *(uint64_t *)(buf + 16) = *(uint64_t *)(i + 16); \
        *(uint64_t *)(buf + 24) = *(uint64_t *)(i + 24); \
    }

// clang-format off
uint8_t txn[283] =
{
/* size,upto */
/*   3,  0 */   0x12U, 0x00U, 0x00U,                                                            /* tt = Payment */
/*   5,  3*/    0x22U, 0x80U, 0x00U, 0x00U, 0x00U,                                              /* flags = tfCanonical */
/*   5,  8 */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                              /* sequence = 0 */
/*   5, 13 */   0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                              /* dtag, flipped */
/*   6, 18 */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                       /* first ledger seq */
/*   6, 24 */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                       /* last ledger seq */
/*  49, 30 */   0x61U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* amount field 9 or 49 bytes */
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99,
/*   9, 79 */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                   /* fee      */
/*  35, 88 */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* pubkey   */
/*  22,123 */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                           /* src acc  */
/*  22,145 */   0x83U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                           /* dst acc  */
/* 116,167 */                                                                                    /* emit details */
/*   0,283 */
};
// clang-format on

// TX BUILDER
#define FLS_OUT (txn + 20U)
#define LLS_OUT (txn + 26U)
#define DTAG_OUT (txn + 14U)
#define AMOUNT_OUT (txn + 30U)
#define FEE_OUT (txn + 80U)
#define HOOK_ACC (txn + 125U)
#define OTX_ACC (txn + 147U)
#define EMIT_OUT (txn + 167U)

uint8_t data[8];
#define LEDGER_OFFSET 30 // 30 seconds
#define MODEL_SIZE 36
#define SEQ_OUT (data + 0U)

// STABLE COIN ACCEPTED (GATEHUB)
uint8_t curr_usd[20] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x55U, 0x53U, 0x44U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

// uint8_t issuer_usd[20] = {
//     0x2AU, 0xDBU, 0x0BU, 0x39U, 0x59U, 0xD6U, 0x0AU, 0x6EU, 0x69U, 0x91U, 
//     0xF7U, 0x29U, 0xE1U, 0x91U, 0x8BU, 0x71U, 0x63U, 0x92U, 0x52U, 0x30U};

uint8_t issuer_usd[20] = {
    0xA4U, 0x07U, 0xAF, 0x58U, 0x56U, 0xCCU, 0xF3U, 0xC4U, 0x26U, 0x19U, 
    0xDAU, 0xA9U, 0x25U, 0x81U, 0x3FU, 0xC9U, 0x55U, 0xC7U, 0x29U, 0x83U};

uint8_t lottery_start_ns[32] = {
    0x0EU, 0xD0U, 0xEBU, 0x28U, 0xB2U, 0x4DU, 0x81U, 0x2DU, 0xA0U, 0xC8U,
    0x4FU, 0xDDU, 0xC0U, 0x64U, 0x14U, 0xE0U, 0xC6U, 0x45U, 0x26U, 0x7CU,
    0x8EU, 0xCCU, 0x4DU, 0xC1U, 0xFFU, 0x58U, 0x5CU, 0xF6U, 0x28U, 0x31U,
    0x6DU, 0x70U};

int64_t hook(uint32_t reserved)
{
    // HOOK ON: TT Invoke
    int64_t tt = otxn_type();
    if (tt != ttINVOKE)
    {
        rollback(SBUF("lottery_end.c: HookOn field is incorrectly set."), INVALID_TXN);
    }

    uint8_t otx_acc[32];
    otxn_field(otx_acc + 12, 20, sfAccount);

    uint8_t hook_acc[32];
    hook_account(hook_acc + 12, 20);

    if (BUFFER_EQUAL_20(hook_acc + 12, otx_acc + 12))
        accept(SBUF("lottery_end.c: outgoing tx on: `Account`."), __LINE__);

    // NS: End Ledger
    uint8_t elt_buffer[8];
    state_foreign(elt_buffer, 8, hook_acc + 12, 20, lottery_start_ns, 32, hook_acc + 12, 20);

    int64_t el_time = FLIP_ENDIAN_64(UINT64_FROM_BUF(elt_buffer));
    int64_t cl_time = ledger_last_time();
    if (cl_time <= el_time)
    {
        rollback(SBUF("lottery_end.c: Lottery not ended."), __LINE__);
    }

    // OTXN PARAM: Hash
    uint8_t hash_buffer[32];
    uint8_t hash_key[2] = {'L', 'H'};
    if (otxn_param(SBUF(hash_buffer), SBUF(hash_key)) != 32)
    {
        rollback(SBUF("lottery_end.c: invalid otxn parameter: `LH`."), __LINE__);
    }

    // VALIDATION: Lottery Exists
    uint8_t model_buffer[MODEL_SIZE];
    if (state(SBUF(model_buffer), hook_acc + 12, 20) != MODEL_SIZE)
    {
        rollback(SBUF("lottery_end.c: Lottery does not exist."), __LINE__);
    }

    // VALIDATION: Lottery Ticket Count Exists
    int64_t count;
    if (state_foreign(&count, 8, hook_acc + 12, 20, hash_buffer, 32, hook_acc + 12, 20) != 8)
    {
        rollback(SBUF("lottery_end.c: Lottery count does not exist."), __LINE__);
    }

    uint8_t win_acct[20];
    uint64_t rnd[4];
    ledger_nonce(rnd, 32);
    if (state_foreign(SBUF(win_acct), rnd[0] % count, 8, hash_buffer, 32, hook_acc + 12, 20) != 20)
    {
        rollback(SBUF("lottery_end.c: invalid winner."), __LINE__);
    }

    // TXN: PREPARE: Init
    etxn_reserve(1);

    // TXN PREPARE: Account
    ACCOUNT_TO_BUF(HOOK_ACC, hook_acc + 12);

    // TXN PREPARE: Destination
    ACCOUNT_TO_BUF(OTX_ACC, win_acct);

    // TXN PREPARE: FirstLedgerSequence
    uint32_t fls = (uint32_t)ledger_seq() + 1;
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);

    // TXN PREPARE: LastLedgerSequense
    uint32_t lls = fls + 4;
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);

    // KEYLET: TrustLine
    uint8_t bal_kl[34];
    int64_t result = util_keylet(SBUF(bal_kl), KEYLET_LINE, hook_acc + 12, 20, issuer_usd, 20, curr_usd, 20);

    // SLOT SET:
    int64_t result_one = slot_set(SBUF(bal_kl), 1);

    if (slot_set(SBUF(bal_kl), 2) != 2)
        accept(SBUF("lottery_end.c: Could not load target balance"), __LINE__);

    // SLOT SUBFIELD: sfBalance
    if (slot_subfield(1, sfBalance, 1) != 1)
        accept(SBUF("lottery_end.c: Could not load target balance 2"), __LINE__);

    int64_t balance_xfl = slot_float(1); // <- amount as token

    if (float_compare(balance_xfl, 0, COMPARE_LESS | COMPARE_EQUAL) == 1)
        rollback(SBUF("lottery_end.c: invalid calc parameter `Amount`."), __LINE__);

    // TXN PREPARE: Amount
    float_sto(AMOUNT_OUT, 49, curr_usd, 20, issuer_usd, 20, balance_xfl, sfAmount);

    // TXN PREPARE: Dest Tag <- Source Tag
    if (otxn_field(DTAG_OUT, 4, sfSourceTag) == 4)
        *(DTAG_OUT - 1) = 0x2EU;

    // TXN PREPARE: Emit Metadata
    etxn_details(EMIT_OUT, 116U);

    // TXN PREPARE: Fee
    {
        int64_t fee = etxn_fee_base(SBUF(txn));
        uint8_t *b = FEE_OUT;
        *b++ = 0b01000000 + ((fee >> 56) & 0b00111111);
        *b++ = (fee >> 48) & 0xFFU;
        *b++ = (fee >> 40) & 0xFFU;
        *b++ = (fee >> 32) & 0xFFU;
        *b++ = (fee >> 24) & 0xFFU;
        *b++ = (fee >> 16) & 0xFFU;
        *b++ = (fee >> 8) & 0xFFU;
        *b++ = (fee >> 0) & 0xFFU;
    }

    // TXN: Emit/Send Txn
    uint8_t emithash[32];
    int64_t emit_result = emit(SBUF(emithash), SBUF(txn));
    if (emit_result > 0)
    {
        // create new lottery
        int64_t ll_time = ledger_last_time();
        ll_time += LEDGER_OFFSET;
        INT64_TO_BUF(SEQ_OUT, ll_time);
        state_foreign_set(&ll_time, 8, hook_acc + 12, 20, lottery_start_ns, 32, hook_acc + 12, 20);
        accept(SBUF("lottery_end.c: Lottery Finished."), __LINE__);
    }
    rollback(SBUF("lottery_end.c: Tx emitted failure."), __LINE__);

    _g(1, 1);
    // unreachable
    return 0;
}