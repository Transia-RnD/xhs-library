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

#define ACCOUNT_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    *(uint64_t*)(buf + 0) = *(uint64_t*)(i +  0);\
    *(uint64_t*)(buf + 8) = *(uint64_t*)(i +  8);\
    *(uint32_t*)(buf + 16) = *(uint32_t*)(i + 16);\
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

#define MODEL_SIZE 93

int64_t hook(uint32_t reserved ) {

    TRACESTR("voucher_claim.c: called");

    // HOOK ON: TT
    int64_t tt = otxn_type();
    if (tt != ttINVOKE)
    {
        rollback(SBUF("voucher_claim.c: HookOn field is incorrectly set."), INVALID_TXN);
    }

    uint8_t otx_acc[32];
    otxn_field(otx_acc + 12, 20, sfAccount);

    uint8_t hook_acc[32];
    hook_account(hook_acc + 12, 20);

    if (BUFFER_EQUAL_20(hook_acc + 12, otx_acc + 12))
        rollback(SBUF("voucher_claim.c: outgoing tx on: `Account`."), __LINE__);

    // OTXN PARAM: Hash
    uint8_t hash_buffer[32];
    uint8_t hash_key[1] = {'H'};
    if (otxn_param(SBUF(hash_buffer), SBUF(hash_key)) != 32)
    {
        rollback(SBUF("voucher_claim.c: invalid otxn parameter: `H`."), __LINE__);
    }

    // OTXN PARAM: Signature Length
    uint8_t sigl_buffer[4];
    uint8_t sigl_key[4] = {'S', 'I', 'G', 'L'};
    if (otxn_param(SBUF(sigl_buffer), SBUF(sigl_key)) != 4)
    {
        rollback(SBUF("voucher_claim.c: invalid hook otxn parameter: `SIGL`."), __LINE__);
    }
    uint32_t sig_len = UINT32_FROM_BUF(sigl_buffer);

    // OTXN PARAM: Signature
    uint8_t sig_buffer[sig_len];
    uint8_t sig_key[3] = {'S', 'I', 'G'};
    if (otxn_param(SBUF(sig_buffer), SBUF(sig_key)) != sig_len)
    {
        rollback(SBUF("voucher_claim.c: invalid hook otxn parameter: `SIG`."), __LINE__);
    }

    // VALIDATION: Exists
    uint8_t model_buffer[MODEL_SIZE];
    if (state(SBUF(model_buffer), hash_buffer, 32) != MODEL_SIZE)
    {
        rollback(SBUF("voucher_claim.c: Voucher does not exist."), __LINE__);
    }

    // VALIDATION: User Limit
    uint8_t data_in[52];
    uint8_t hash_out[32];
#define ACCT_OUT (data_in + 0U)
#define HASH_OUT (data_in + 20U)
    ACCOUNT_TO_BUF(ACCT_OUT, otx_acc + 12);
    UINT256_TO_BUF(HASH_OUT, hash_buffer);
    util_sha512h(SBUF(hash_out), SBUF(data_in));
    uint8_t dump[20];
    if (state(SBUF(dump), SBUF(hash_out)) == 20)
    {
        rollback(SBUF("voucher_claim.c: User already claimed."), __LINE__);
    }

    // VALIDATION: Limit
    int32_t limit = UINT32_FROM_BUF(model_buffer + 0U);
    int64_t count_buf[1];
    state(SBUF(count_buf), model_buffer + 57U, 32);
    if (count_buf[0] > limit)
    {
        rollback(SBUF("voucher_claim.c: Voucher limit reached."), __LINE__);
    }

    // VALIDATION: Signature
    if (util_verify(SBUF(hash_buffer), SBUF(sig_buffer), SBUF(model_buffer + 57U)) == 0)
    {
        rollback(SBUF("voucher_claim.c: Invalid signature."), __LINE__);
    }

    // TXN: PREPARE: Init
    etxn_reserve(1);

    // TXN PREPARE: Account
    ACCOUNT_TO_BUF(HOOK_ACC, hook_acc + 12);

    // TXN PREPARE: Destination
    ACCOUNT_TO_BUF(OTX_ACC, otx_acc + 12);

    // TXN PREPARE: FirstLedgerSequence
    uint32_t fls = (uint32_t)ledger_seq() + 1;
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);

    // TXN PREPARE: LastLedgerSequense
    uint32_t lls = fls + 4;
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);

    // AMOUNT
    int64_t amount_native = 1;
    uint8_t zero_buf[20];
    if (
        !BUFFER_EQUAL_20(model_buffer + 20, zero_buf) 
        && !BUFFER_EQUAL_20(model_buffer + 40, zero_buf)
    )
    {
        amount_native = 0;
    }
    uint64_t amount = FLIP_ENDIAN_64(UINT64_FROM_BUF(model_buffer + 12));
    if (float_compare(amount, 0, COMPARE_LESS | COMPARE_EQUAL) == 1)
        rollback(SBUF("voucher_claim.c: invalid model parameter `Amount`."), __LINE__);

    // TXN PREPARE: Amount
    if (amount_native)
    {
        uint64_t drops = float_int(amount, 6, 1);
        uint8_t *b = AMOUNT_OUT + 1;
        *b++ = 0b01000000 + ((drops >> 56) & 0b00111111);
        *b++ = (drops >> 48) & 0xFFU;
        *b++ = (drops >> 40) & 0xFFU;
        *b++ = (drops >> 32) & 0xFFU;
        *b++ = (drops >> 24) & 0xFFU;
        *b++ = (drops >> 16) & 0xFFU;
        *b++ = (drops >> 8) & 0xFFU;
        *b++ = (drops >> 0) & 0xFFU;
    }
    else
        float_sto(AMOUNT_OUT, 49, model_buffer + 20, 20, model_buffer + 40, 20, amount, sfAmount);

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

    TRACEHEX(txn);

    // TXN: Emit/Send Txn
    uint8_t emithash[32];
    int64_t emit_result = emit(SBUF(emithash), SBUF(txn));
    if (emit_result > 0)
    {
        count_buf[0]++;
        state_set(count_buf, 1, model_buffer + 57U, 32);
        state_set(otx_acc + 12, 20, SBUF(hash_out));
        accept(SBUF("voucher_claim.c: Voucher Claimed."), __LINE__);
    }
    rollback(SBUF("voucher_claim.c: Tx emitted failure."), __LINE__);

    _g(1,1);
    // unreachable
    return 0;
}