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
/*   5,  3 */   0x22U, 0x80U, 0x00U, 0x00U, 0x00U,                                              /* flags = tfCanonical */
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

// clang-format off
#define PREPARE_PAYMENT_TXN(account_buffer, dest_buffer, amount_xfl) do { \ 
    if (otxn_field(DTAG_OUT, 4, sfSourceTag) == 4) \
        *(DTAG_OUT - 1) = 0x2EU; \
    uint32_t fls = (uint32_t)ledger_seq() + 1; \
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls); \
    uint32_t lls = fls + 4; \
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls); \
    ACCOUNT_TO_BUF(HOOK_ACC, account_buffer); \
    ACCOUNT_TO_BUF(OTX_ACC, dest_buffer); \
    uint64_t drops = float_int(payout_xfl, 6, 1); \
    uint8_t *b = AMOUNT_OUT + 1; \
    *b++ = 0b01000000 + ((drops >> 56) & 0b00111111); \
    *b++ = (drops >> 48) & 0xFFU; \
    *b++ = (drops >> 40) & 0xFFU; \
    *b++ = (drops >> 32) & 0xFFU; \
    *b++ = (drops >> 24) & 0xFFU; \
    *b++ = (drops >> 16) & 0xFFU; \
    *b++ = (drops >> 8) & 0xFFU; \
    *b++ = (drops >> 0) & 0xFFU; \
    etxn_details(EMIT_OUT, 116U); \
    int64_t fee = etxn_fee_base(SBUF(txn)); \
    uint8_t *f = FEE_OUT; \
    *f++ = 0b01000000 + ((fee >> 56) & 0b00111111); \
    *f++ = (fee >> 48) & 0xFFU; \
    *f++ = (fee >> 40) & 0xFFU; \
    *f++ = (fee >> 32) & 0xFFU; \
    *f++ = (fee >> 24) & 0xFFU; \
    *f++ = (fee >> 16) & 0xFFU; \
    *f++ = (fee >> 8) & 0xFFU; \
    *f++ = (fee >> 0) & 0xFFU; \
} while(0) 
// clang-format on

// clang-format off 
uint8_t ns_txn[250] =
{
/* size,upto */
/*   3,   0,   tt = SetHook         */   0x12U, 0x00U, 0x16U,
/*   5,   3,   flags                */   0x22U, 0x00U, 0x00U, 0x00U, 0x00U,
/*   5,   8,   sequence             */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,
/*   6,   13,  firstledgersequence  */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,
/*   6,   19,  lastledgersequence   */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,
/*   1,   25,  hooks - Array Start  */   0xFBU, 
/*   1,   26,  hooks - Object Start */   0xEEU, 
/*   5,   27,  hooks - Flags        */   0x22U, 0x00U, 0x00U, 0x00U, 0x02U, 
/*   34,  32,  hooks - Namespace    */   0x50U, 0x20U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 
/*   1,   66,  hooks - Object End   */   0xE1U,
/*   1,   67,  hooks - Array End    */   0xF1U,
/*   9,   68,  fee                  */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/*   35,  77,  signingpubkey        */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   22,  112, account              */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   116, 134, emit details         */ 
/*   0,   250                       */ 
};
// clang-format on

// NS TX BUILDER
#define NSFLS_OUT (ns_txn + 15U)
#define NSLLS_OUT (ns_txn + 21U)
#define NSID_OUT (ns_txn + 34U)
#define NSFEE_OUT (ns_txn + 69U)
#define NSHOOK_ACC (ns_txn + 114U)
#define NSEMIT_OUT (ns_txn + 134U)

// clang-format off
#define PREPARE_HOOK_TXN(account_buffer, ns_hash) do { \ 
    uint32_t fls = (uint32_t)ledger_seq() + 1; \ 
    *((uint32_t *)(NSFLS_OUT)) = FLIP_ENDIAN(fls); \ 
    uint32_t lls = fls + 4; \ 
    *((uint32_t *)(NSLLS_OUT)) = FLIP_ENDIAN(lls); \
    ACCOUNT_TO_BUF(NSHOOK_ACC, account_buffer); \ 
    UINT256_TO_BUF(NSID_OUT, ns_hash) \
    etxn_details(NSEMIT_OUT, 116U); \ 
    int64_t fee = etxn_fee_base(SBUF(ns_txn)); \ 
    uint8_t *f = NSFEE_OUT; \ 
    *f++ = 0b01000000 + ((fee >> 56) & 0b00111111); \ 
    *f++ = (fee >> 48) & 0xFFU; \ 
    *f++ = (fee >> 40) & 0xFFU; \ 
    *f++ = (fee >> 32) & 0xFFU; \ 
    *f++ = (fee >> 24) & 0xFFU; \ 
    *f++ = (fee >> 16) & 0xFFU; \ 
    *f++ = (fee >> 8) & 0xFFU; \ 
    *f++ = (fee >> 0) & 0xFFU; \
} while(0) 
// clang-format on

#define LOTTERY_MODEL 68U
#define ID_OFFSET 0U
#define PRICE_OFFSET 8U
#define MAX_OFFSET 44U
#define MAX_TICKETS_OFFSET 52U
#define TIME_OFFSET 60U
uint8_t data[8];
#define TIME_OUT (data + 0U)

#define LCG_MULTIPLIER 1103515245
#define LCG_INCREMENT 12345
#define LCG_MODULUS (1U << 31)

// Macro to generate a seed from the hash provided by ledger_nonce
#define GENERATE_SEED(hash, hash_size, seed) do { \
    seed = 0; \
    for (int i = 0; GUARD(32), i < sizeof(seed); ++i) { \
        seed ^= ((unsigned int)hash[i] & 0xFF) << (i * 8); \
    } \
} while(0)

// Macro for the LCG function to generate a pseudo-random number
#define LCG_RAND(seed) \
    ((seed) = (LCG_MULTIPLIER * (seed) + LCG_INCREMENT) % LCG_MODULUS)

// Macro to generate a random number in the range [0, count)
#define GENERATE_RANDOM(hash, hash_size, count, random) do { \
    unsigned int seed; \
    GENERATE_SEED(hash, hash_size, seed); \
    LCG_RAND(seed); \
    random = seed % count; \
} while(0)

uint8_t lottery_start_ns[32] = {
    0x0EU, 0xD0U, 0xEBU, 0x28U, 0xB2U, 0x4DU, 0x81U, 0x2DU, 0xA0U, 0xC8U,
    0x4FU, 0xDDU, 0xC0U, 0x64U, 0x14U, 0xE0U, 0xC6U, 0x45U, 0x26U, 0x7CU,
    0x8EU, 0xCCU, 0x4DU, 0xC1U, 0xFFU, 0x58U, 0x5CU, 0xF6U, 0x28U, 0x31U,
    0x6DU, 0x70U};

uint8_t rate_acct[20] = {
    0x05U, 0xB5U, 0xF4U, 0x3AU, 0xF7U, 0x17U, 0xB8U, 0x19U, 0x48U, 0x49U, 
    0x1FU, 0xB7U, 0x07U, 0x9EU, 0x4FU, 0x17U, 0x3FU, 0x4EU, 0xCEU, 0xB3U
};

// uint8_t rate_acct[20] = {
//     0x7CU, 0xE1U, 0x83U, 0x29U, 0x70U, 0x51U, 0x4CU, 0x37U, 0x97U, 0xB2U, 
//     0xABU, 0x5EU, 0x14U, 0x52U, 0x69U, 0x0FU, 0x1DU, 0x6EU, 0xA3U, 0x70U
// };

uint8_t issuer_buffer[20] = {
    0x5BU, 0xEFU, 0x92U, 0x1AU, 0x21U, 0x7DU, 0x57U, 0xFDU, 0xA5U, 0xB5U, 
    0x6DU, 0x5BU, 0x40U, 0xBEU, 0xE4U, 0x0DU, 0x1AU, 0xC1U, 0x12U, 0x7FU
};

// uint8_t issuer_buffer[20] = {
//     0xA4U, 0x07U, 0xAF, 0x58U, 0x56U, 0xCCU, 0xF3U, 0xC4U, 0x26U, 0x19U, 
//     0xDAU, 0xA9U, 0x25U, 0x81U, 0x3FU, 0xC9U, 0x55U, 0xC7U, 0x29U, 0x83U
// };

uint8_t currency_buffer[20] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x55U, 0x53U, 0x44U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
};

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

    // OTXN PARAM: Hash
    uint8_t hash_buffer[32];
    uint8_t hash_key[2] = {'L', 'H'};
    if (otxn_param(SBUF(hash_buffer), SBUF(hash_key)) != 32)
    {
        rollback(SBUF("lottery_end.c: invalid otxn parameter: `LH`."), __LINE__);
    }

    // VALIDATION: Lottery Exists
    uint8_t model_buffer[LOTTERY_MODEL];
    if (state(SBUF(model_buffer), SBUF(hash_buffer)) != LOTTERY_MODEL)
    {
        rollback(SBUF("lottery_end.c: Lottery does not exist."), __LINE__);
    }

    // VALIDATE: Lottery Has Ended
    uint8_t elt_buffer[8];
    int64_t lottery_id = UINT64_FROM_BUF(model_buffer + ID_OFFSET);
    state_foreign(elt_buffer, 8, &lottery_id, 8, lottery_start_ns, 32, hook_acc + 12, 20);
    int64_t el_time = FLIP_ENDIAN_64(UINT64_FROM_BUF(elt_buffer));
    int64_t cl_time = ledger_last_time();

    // VALIDATION: Lottery Ticket Count Exists
    int64_t ticket_count;
    if (state_foreign(&ticket_count, 8, hook_acc + 12, 20, hash_buffer, 32, hook_acc + 12, 20) == DOESNT_EXIST)
    {
        int64_t ll_time = ledger_last_time();
        int64_t ledger_offset = UINT64_FROM_BUF(model_buffer + TIME_OFFSET);
        ll_time += ledger_offset;
        INT64_TO_BUF(TIME_OUT, ll_time);
        state_foreign_set(&ll_time, 8, &lottery_id, 8, lottery_start_ns, 32, hook_acc + 12, 20);
        accept(SBUF("lottery_end.c: Lottery Finished."), __LINE__);
    }

    if (ticket_count < 0) {
        rollback(SBUF("lottery_end.c: Lottery count insane value."), __LINE__);
    }

    int64_t max_tickets = UINT64_FROM_BUF(model_buffer + MAX_TICKETS_OFFSET);
    if (cl_time < el_time && ticket_count < max_tickets)
    {
        rollback(SBUF("lottery_end.c: Lottery not ended."), __LINE__);
    }

    uint8_t win_acct[20];
    uint64_t hash[32];
    ledger_nonce(hash, 32);
    int64_t random_number;
    GENERATE_RANDOM(hash, 32, ticket_count, random_number);
    if (state_foreign(SBUF(win_acct), &random_number, 8, hash_buffer, 32, hook_acc + 12, 20) != 20)
    {
        rollback(SBUF("lottery_end.c: invalid winner."), __LINE__);
    }

    // TXN: PREPARE: Init
    etxn_reserve(2);

    int64_t amount_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(model_buffer + PRICE_OFFSET));
    int64_t payout_xfl = float_multiply(amount_xfl, float_set(0, ticket_count));

    // VALIDATE: Lottery Has Reached Limit
    uint8_t line_kl[34];
    util_keylet(SBUF(line_kl), KEYLET_LINE, rate_acct, 20, issuer_buffer, 20, currency_buffer, 20);

    // SLOT SET: Slot 1
    if (slot_set(SBUF(line_kl), 1) != 1)
        accept(SBUF("lottery_end.c: Could not load trustline"), __LINE__);

    // SLOT SUBFIELD: sfLowLimit
    if (slot_subfield(1, sfLowLimit, 1) != 1)
        accept(SBUF("lottery_end.c: Could not load trustline `sfLowLimit`"), __LINE__);

    int64_t rate_xfl = slot_float(1); // <- amount as token
    int64_t max_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(model_buffer + MAX_OFFSET));
    int64_t count_xfl = float_set(0, ticket_count);
    int64_t rate_pool_xfl = float_multiply(rate_xfl, payout_xfl);
    
    // ADJUSTMENT: If Prize Pool > Max Allowed -> Payout == Max
    if (float_compare(rate_pool_xfl, max_xfl, COMPARE_GREATER) == 1) \
        payout_xfl = max_xfl;
    
    if (float_compare(payout_xfl, 0, COMPARE_LESS | COMPARE_EQUAL) == 1) \
        rollback(SBUF("lottery_end.c: invalid calc parameter `Amount`."), __LINE__);

    PREPARE_PAYMENT_TXN(hook_acc + 12, win_acct, payout_xfl);
    PREPARE_HOOK_TXN(hook_acc + 12, hash_buffer);

    // TXN: Emit/Send Txn
    uint8_t emithash[32];
    int64_t emit_result = emit(SBUF(emithash), SBUF(txn));
    if (emit_result < 0)
    {
        rollback(SBUF("lottery_end.c: Tx emitted failure."), __LINE__);
    }

    // TXN: Emit/Send Txn
    uint8_t ns_emithash[32];
    int64_t ns_emit_result = emit(SBUF(ns_emithash), SBUF(ns_txn));
    if (ns_emit_result < 0)
    {
        rollback(SBUF("lottery_end.c: Tx emitted failure."), __LINE__);
    }

    // create new lottery
    int64_t ll_time = ledger_last_time();
    int64_t ledger_offset = UINT64_FROM_BUF(model_buffer + TIME_OFFSET);
    ll_time += ledger_offset;
    INT64_TO_BUF(TIME_OUT, ll_time);
    state_foreign_set(&ll_time, 8, &lottery_id, 8, lottery_start_ns, 32, hook_acc + 12, 20);
    accept(SBUF("lottery_end.c: Lottery Finished."), __LINE__);

    _g(1, 1);
    // unreachable
    return 0;
}