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

// ---------------------------------------------------------------------------------------------------------------------------------------------

/**
 *
 * These functions should be moved into the macro.c file
 */

#define UINT256_TO_BUF(buf_raw, i)                       \
    {                                                    \
        unsigned char *buf = (unsigned char *)buf_raw;   \
        *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0);   \
        *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8);   \
        *(uint64_t *)(buf + 16) = *(uint64_t *)(i + 16); \
        *(uint64_t *)(buf + 24) = *(uint64_t *)(i + 24); \
    }
// ---------------------------------------------------------------------------------------------------------------------------------------------

uint8_t txn[295] =
    {
        /* size,upto */
        /*   3,  0  */ 0x12U, 0x00U, 0x2FU,                                                                                             /* tt = URITokenBuy */
        /*   5,  3  */ 0x22U, 0x80U, 0x00U, 0x00U, 0x00U,                                                                               /* flags = tfCanonical */
        /*   5,  8  */ 0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                                                               /* sequence = 0 */
        /*   5, 13  */ 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                                                               /* dtag, flipped */
        /*   6, 18  */ 0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                                                        /* first ledger seq */
        /*   6, 24  */ 0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                                                        /* last ledger seq */
        /*  49, 30  */ 0x61U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                                          /* amount field 9 or 49 bytes */
        0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                                                         /* cont...  */
        0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                                                         /* cont...  */
        0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                                                         /* cont...  */
        0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                                                         /* cont...  */
        0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                                                         /* cont...  */
        0x99,                                                                                                                           /* cont...  */
        /*  34, 79  */ 0x50U, 0x24U, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* hash256 = URITokenID  */
        /*   9, 113 */ 0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                                                          /* fee      */
        0x00U,                                                                                                                          /* cont...  */
        /*  35, 122 */ 0x73U, 0x21U, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* pubkey   */
        /*  22, 157 */ 0x81U, 0x14U, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                        /* src acc  */
        /* 116, 179 */                                                                                                                  /* emit details */
        /*   0, 295 */
};

// TX BUILDER
#define FLS_OUT (txn + 20U)
#define LLS_OUT (txn + 26U)
#define DTAG_OUT (txn + 14U)
#define INDEXID_OUT (txn + 81U)
#define AMOUNT_OUT (txn + 30U)
#define HOOK_ACC (txn + 159U)
#define FEE_OUT (txn + 114U)
#define EMIT_OUT (txn + 179U)

int64_t hook(uint32_t reserved)
{
    TRACESTR("auction_start.c: Called.");

    // HOOK ON: TT
    int64_t tt = otxn_type();
    if (tt != ttURITOKEN_CREATE_SELL_OFFER)
    {
        rollback(SBUF("auction_start: HookOn field is incorrectly set."), INVALID_TXN);
    }

    // ACCOUNT: Hook Account
    uint8_t hook_acc[20];
    hook_account(HOOK_ACC, 20);

    uint8_t auction_model[84];
    uint8_t am_key[2] = {'A', 'M'};
    if (otxn_param(SBUF(auction_model), SBUF(am_key)) != 84)
    {
        rollback(SBUF("auction_start.c: Invalid Txn Parameter `AM`"), __LINE__);
    }
    TRACEHEX(auction_model);

    uint32_t start_time = UINT32_FROM_BUF(auction_model);
    uint32_t end_time = UINT32_FROM_BUF(auction_model + 4);
    uint64_t cur_time = ledger_last_time();
    TRACEVAR(start_time);
    TRACEVAR(end_time);
    TRACEVAR(cur_time);

    if (end_time < start_time || start_time == 0 || end_time == 0)
    {
        rollback(SBUF("auction_start.c: Invalid Binary Model Field `end_time ||/< start_time`"), __LINE__);
    }
    if (end_time < cur_time)
    {
        rollback(SBUF("auction_start.c: Invalid Binary Model Field `end_time`"), __LINE__);
    }

    uint8_t tid_buffer[32];
    if (otxn_field(SBUF(tid_buffer), sfURITokenID) != 32)
    {
        rollback(SBUF("auction_start.c: Invalid URITokenID"), __LINE__);
    }
    TRACEHEX(tid_buffer);
    UINT256_TO_BUF(INDEXID_OUT, tid_buffer);

    // TXN: PREPARE: Init
    etxn_reserve(1);

    // TXN PREPARE: FirstLedgerSequence
    uint32_t fls = (uint32_t)ledger_seq() + 1;
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);

    // TXN PREPARE: LastLedgerSequense
    uint32_t lls = fls + 4;
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);

    // TXN PREPARE: Amount
    uint64_t drops = 0;
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

    TRACEHEX(txn); // <- final tx blob

    // TXN: Emit/Send Txn
    uint8_t emithash[32];
    int64_t emit_result = emit(SBUF(emithash), SBUF(txn));
    if (emit_result > 0)
    {
        state_set(SBUF(auction_model), SBUF(tid_buffer));
        accept(SBUF("auction_start.c: Tx emitted success."), __LINE__);
    }
    accept(SBUF("auction_start.c: Tx emitted failure."), __LINE__);

    _g(1, 1);
    // unreachable
    return 0;
}