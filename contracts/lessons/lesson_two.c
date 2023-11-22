/**
 *
 */
#include "hookapi.h"

// Txn Hook Parameter #1
// Name: S
// Value: 030080C6A47E8D0355F51DFC2A09D62CBBA1DFBDD4691DAC96AD98B90F0080C6A47E8D0355B389FBCED0AF9DCDFF62900BFAEFA3EB872D8A960080E03779C3D154AA266540F7DACC27E264B75ED0A5ED7330BFB614
// 1 byte = length of array
// Repeating for each distribution
// 8 bytes = amount
// 20 bytes = account id

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

uint8_t txn[283] =
{
/* size,upto */
/*   3,  0 */   0x12U, 0x00U, 0x00U,                                                               /* tt = Payment */
/*   5,  3*/    0x22U, 0x80U, 0x00U, 0x00U, 0x00U,                                          /* flags = tfCanonical */
/*   5,  8 */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                                 /* sequence = 0 */
/*   5, 13 */   0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                                /* dtag, flipped */
/*   6, 18 */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                      /* first ledger seq */
/*   6, 24 */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                       /* last ledger seq */
/*  49, 30 */   0x61U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,              /* amount field 9 or 49 bytes */
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99,
/*   9, 79 */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                         /* fee      */
/*  35, 88 */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       /* pubkey   */
/*  22,123 */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                 /* src acc  */
/*  22,145 */   0x83U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                 /* dst acc  */
/* 116,167 */   /* emit details */
/*   0,283 */
};

// ACCOUNTS
#define HOOK_ACC (txn + 125U)
#define OTX_ACC (txn + 147U)

// TXS
#define FLS_OUT (txn + 20U)                                                                                            
#define LLS_OUT (txn + 26U)                                                                                            
#define DTAG_OUT (txn + 14U)                                                                                           
#define AMOUNT_OUT (txn + 30U)                                                                                                                                                                                
#define EMIT_OUT (txn + 167U)                                                                                          
#define FEE_OUT (txn + 80U)

int64_t hook(uint32_t reserved) 
{
    // HOOK ON: TT
    int64_t tt = otxn_type();
    if (tt != ttPAYMENT)
    {
        rollback(SBUF("lesson_two.c: HookOn field is incorrectly set."), INVALID_TXN);
    }

    // ACCOUNT: Hook Account
    uint8_t hook_acc[20];
    hook_account(HOOK_ACC, 20);

    // ACCOUNT: Origin Tx Account
    uint8_t otx_acc[20];
    otxn_field(otx_acc, 20, sfAccount);

    // FILTER ON: ACCOUNT
    if (BUFFER_EQUAL_20(hook_acc, otx_acc))
    {
        accept(SBUF("lesson_one.c: Accepting Transaction `Outgoing`."), __LINE__);
    }

    uint8_t param_buffer[256];
    uint8_t param_key[2] = {'S', 'A'};
    int64_t otxn_param_size = otxn_param(param_buffer, 256, SBUF(param_key));
    int64_t num_sellers = param_buffer[0];

    etxn_reserve(num_sellers);

    for (int i = 0; GUARD(8), i < num_sellers; ++i)
    {
        // TXN PREPARE: Destination
        ACCOUNT_TO_BUF(OTX_ACC, param_buffer + 1 + 8 + (28 * i));

        // TXN PREPARE: FirstLedgerSequence
        uint32_t fls = (uint32_t)ledger_seq() + 1;
        *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);

        // TXN PREPARE: LastLedgerSequense
        uint32_t lls = fls + 4;
        *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);

        // TXN PREPARE: Amount
        uint64_t amount = FLIP_ENDIAN_64(UINT64_FROM_BUF(param_buffer + 1 + (28 * i)));
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
        if (emit_result <= 0)
        {
            rollback(SBUF("lesson_two.c: Tx Emit Failure."), __LINE__);
        }
    }
    accept(SBUF("lesson_two.c: Finished."), __LINE__);

    _g(1,1);
    // unreachable
    return 0;
}