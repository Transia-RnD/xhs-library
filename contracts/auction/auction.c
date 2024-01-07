//------------------------------------------------------------------------------
/*
    Copyright (c) 2023 Transia, LLC

    This software component is intended for use by individuals or entities who
    possess the necessary technical understanding and qualifications to utilize
    such components in accordance with applicable laws and regulations.
    Unauthorized use or distribution of this component may be subject to legal
    consequences.

    The information provided in this software component is for informational
    purposes only and should not be considered as technical advice or a
    recommendation to engage in any specific implementation or usage strategy.
    It is important to consult with a qualified professional or technical advisor
    before integrating or making any decisions based on this component.
*/
//==============================================================================

#define DEBUG 1

#define ACCOUNT_TO_BUF(buf_raw, i)                       \
    {                                                    \
        unsigned char *buf = (unsigned char *)buf_raw;   \
        *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0);   \
        *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8);   \
        *(uint32_t *)(buf + 16) = *(uint32_t *)(i + 16); \
    }

#define AMOUNT_TO_BUF(buf_raw, i)                        \
    {                                                    \
        unsigned char *buf = (unsigned char *)buf_raw;   \
        *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0);   \
        *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8);   \
        *(uint64_t *)(buf + 16) = *(uint64_t *)(i + 16); \
        *(uint64_t *)(buf + 24) = *(uint64_t *)(i + 24); \
        *(uint64_t *)(buf + 32) = *(uint64_t *)(i + 32); \
        *(uint64_t *)(buf + 40) = *(uint64_t *)(i + 40); \
    }

#define BUFFER_EQUAL_AMOUNT(buf1, buf2)                               \
    (                                                                 \
        *(((uint64_t *)(buf1)) + 0) == *(((uint64_t *)(buf2)) + 0) && \
        *(((uint64_t *)(buf1)) + 1) == *(((uint64_t *)(buf2)) + 1) && \
        *(((uint64_t *)(buf1)) + 2) == *(((uint64_t *)(buf2)) + 2) && \
        *(((uint64_t *)(buf1)) + 3) == *(((uint64_t *)(buf2)) + 3) && \
        *(((uint64_t *)(buf1)) + 4) == *(((uint64_t *)(buf2)) + 4) && \
        *(((uint64_t *)(buf1)) + 5) == *(((uint64_t *)(buf2)) + 5))

#define INT32_TO_BUF(buf_raw, i)                       \
    {                                                  \
        unsigned char *buf = (unsigned char *)buf_raw; \
        *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0); \
        *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8); \
    }

#define UINT256_TO_BUF(buf_raw, i)                       \
    {                                                    \
        unsigned char *buf = (unsigned char *)buf_raw;   \
        *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0);   \
        *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8);   \
        *(uint64_t *)(buf + 16) = *(uint64_t *)(i + 16); \
        *(uint64_t *)(buf + 24) = *(uint64_t *)(i + 24); \
    }

#define FLIP_ENDIAN_64(n) ((uint64_t)(((n & 0xFFULL) << 56ULL) |             \
                                      ((n & 0xFF00ULL) << 40ULL) |           \
                                      ((n & 0xFF0000ULL) << 24ULL) |         \
                                      ((n & 0xFF000000ULL) << 8ULL) |        \
                                      ((n & 0xFF00000000ULL) >> 8ULL) |      \
                                      ((n & 0xFF0000000000ULL) >> 24ULL) |   \
                                      ((n & 0xFF000000000000ULL) >> 40ULL) | \
                                      ((n & 0xFF00000000000000ULL) >> 56ULL)))

#define INT64_FROM_BUF(buf)                  \
    ((((uint64_t)((buf)[0] & 0x7FU) << 56) + \
      ((uint64_t)((buf)[1]) << 48) +         \
      ((uint64_t)((buf)[2]) << 40) +         \
      ((uint64_t)((buf)[3]) << 32) +         \
      ((uint64_t)((buf)[4]) << 24) +         \
      ((uint64_t)((buf)[5]) << 16) +         \
      ((uint64_t)((buf)[6]) << 8) +          \
      ((uint64_t)((buf)[7]) << 0)) *         \
     (buf[0] & 0x80U ? -1 : 1))

#include "hookapi.h"

// clang-format off
uint8_t txn[317] =
{
/* size,upto */
/*   3,  0  */ 0x12U, 0x00U, 0x30U,                                                             /* tt = URITokenCreateSellOffer */
/*   5,  3  */ 0x22U, 0x80U, 0x00U, 0x00U, 0x00U,                                               /* flags = tfCanonical */
/*   5,  8  */ 0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                               /* sequence = 0 */
/*   5, 13  */ 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                               /* dtag, flipped */
/*   6, 18  */ 0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                        /* first ledger seq */
/*   6, 24  */ 0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                        /* last ledger seq */
/*  49, 30  */ 0x61U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* amount field 9 or 49 bytes */
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                0x99,                                                                            /* cont...  */
/*  34, 79  */ 0x50U, 0x24U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   /* hash256 id  */
/*   9, 113 */ 0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                          /* fee      */
                0x00U,                                                                           /* cont...  */
/*  35, 122 */ 0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* pubkey   */
/*  22, 157 */ 0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                           /* src acc  */
/*  22, 179 */ 0x83U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                           /* dst acc  */
/* 116, 201 */                                                                                  /* emit details */
/*   0, 317 */
};
// clang-format on

// clang-format off
uint8_t e_txn[268] =
{
/* size,upto */
/*   3,  0 */ 0x12U, 0x00U, 0x04U,                                                             /* tt = EscrowCancel */
/*   5,  3 */ 0x22U, 0x80U, 0x00U, 0x00U, 0x00U,                                               /* flags = tfCanonical */
/*   5,  8 */ 0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                               /* sequence = 0 */
/*   5, 13 */ 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                               /* dtag, flipped */
/*   6, 18 */ 0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                        /* first ledger seq */
/*   6, 24 */ 0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                        /* last ledger seq */
/*  34, 30 */ 0x50U, 0x23U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   /* hash256 id */
/*   9, 64 */ 0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                   /* fee      */
/*  35, 73 */ 0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* pubkey   */
/*  22,108 */ 0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                           /* src acc  */
/*  22,130 */ 0x82U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                           /* owner acc  */
/* 116,152 */                                                                                  /* emit details */
/*   0,268 */
};
// clang-format on

// ESCROW TX BUILDER
#define E_TT_OUT (e_txn + 2U)
#define E_HOOK_ACC (e_txn + 110U)
#define E_DTAG_OUT (e_txn + 14U)
#define E_FLS_OUT (e_txn + 20U)
#define E_LLS_OUT (e_txn + 26U)
#define E_OWNER_OUT (e_txn + 132U)
#define E_ID_OUT (e_txn + 32U)
#define E_EMIT_OUT (e_txn + 152U)
#define E_FEE_OUT (e_txn + 65U)

// TX BUILDER
#define FLS_OUT (txn + 20U)
#define LLS_OUT (txn + 26U)
#define DTAG_OUT (txn + 14U)
#define INDEXID_OUT (txn + 81U)
#define AMOUNT_OUT (txn + 30U)
#define HOOK_ACC (txn + 159U)
#define DEST_ACC (txn + 181U)
#define FEE_OUT (txn + 114U)
#define EMIT_OUT (txn + 201U)

// MODEL
#define ET_OFFSET 4U
#define MB_OFFSET 8U
#define BC_OFFSET 16U
#define WB_OFFSET 24U
#define WA_OFFSET 32U
#define WID_OFFSET 52U

#define NUM_BIDS (auction_buffer + BC_OFFSET)
#define WIN_BID (auction_buffer + WB_OFFSET)
#define WIN_ACCOUNT (auction_buffer + WA_OFFSET)
#define WIN_ID (auction_buffer + WID_OFFSET)

int64_t hook(uint32_t r)
{
    TRACESTR("auction.c: Called.");

    // HOOK ON: TT
    int64_t tt = otxn_type();
    if (tt != ttESCROW_CREATE && tt != ttINVOKE)
    {
        rollback(SBUF("auction: HookOn field is incorrectly set."), INVALID_TXN);
    }

    // ACCOUNT: Hook Account
    hook_account(HOOK_ACC, 20);

    // ACCOUNT: Origin Tx Account
    uint8_t otx_acc[20];
    otxn_field(otx_acc, 20, sfAccount);

    // OTXN PARAM: URI TOKEN ID
    uint8_t tid_buffer[32];
    uint8_t tid_key[3] = {'T', 'I', 'D'};
    if (otxn_param(SBUF(tid_buffer), SBUF(tid_key)) != 32)
    {
        rollback(SBUF("auction.c: Invalid Txn Parameter `TID`."), __LINE__);
    }

    // VALIDATE: Incoming Amount
    // MUST BE USD/GATEHUB
    uint8_t auction_buffer[84];
    if (state(SBUF(auction_buffer), SBUF(tid_buffer)) != 84)
    {
        rollback(SBUF("auction.c: Invalid URIToken Auction."), __LINE__);
    }

    // END AUCTION
    uint32_t end_time = UINT32_FROM_BUF(auction_buffer + ET_OFFSET);
    int64_t min_bid_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(auction_buffer + MB_OFFSET));
    int64_t win_bid_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(auction_buffer + WB_OFFSET));
    uint64_t cur_time = ledger_last_time();
    if (tt == ttINVOKE)
    {
        TRACEVAR(cur_time);
        TRACEVAR(end_time);
        if (cur_time < end_time)
        {
            rollback(SBUF("auction.c: Auction has not ended."), __LINE__);
        }

        if (win_bid_xfl < min_bid_xfl)
        {
            rollback(SBUF("auction.c: Min Bid not reached."), __LINE__);
        }
        TRACESTR("auction.c: End.");

        // VALIDATE WIN BID && ACCOUNT

        // TXN: PREPARE: Init
        etxn_reserve(2);

        // TXN PREPARE: Destination Account
        ACCOUNT_TO_BUF(DEST_ACC, auction_buffer + WA_OFFSET);

        // TXN PREPARE: URITokenID
        UINT256_TO_BUF(INDEXID_OUT, tid_buffer);

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
        if (emit_result < 1)
        {
            TRACEVAR(emit_result);
            rollback(SBUF("auction.c: URIToken Sell Failure."), __LINE__);
        }

        // TXN: PREPARE: Init
        *E_TT_OUT = 0x02U;
        
        // ACCOUNT: Hook Account
        ACCOUNT_TO_BUF(E_HOOK_ACC, HOOK_ACC);
        
        // ACCOUNT: Owner Account
        ACCOUNT_TO_BUF(E_OWNER_OUT, auction_buffer + WA_OFFSET);

        // ACCOUNT: Ewscow ID
        UINT256_TO_BUF(E_ID_OUT, auction_buffer + WID_OFFSET);

        // TXN PREPARE: FirstLedgerSequence
        uint32_t e_fls = (uint32_t)ledger_seq() + 1;
        *((uint32_t *)(E_FLS_OUT)) = FLIP_ENDIAN(e_fls);

        // TXN PREPARE: LastLedgerSequense
        uint32_t e_lls = e_fls + 4;
        *((uint32_t *)(E_LLS_OUT)) = FLIP_ENDIAN(e_lls);

        // TXN PREPARE: Dest Tag <- Source Tag
        if (otxn_field(E_DTAG_OUT, 4, sfSourceTag) == 4)
            *(E_DTAG_OUT - 1) = 0x2EU;

        // TXN PREPARE: Emit Metadata
        etxn_details(E_EMIT_OUT, 116U);

        // TXN PREPARE: Fee
        {
            int64_t e_fee = etxn_fee_base(SBUF(e_txn));
            uint8_t *e_b = E_FEE_OUT;
            *e_b++ = 0b01000000 + ((e_fee >> 56) & 0b00111111);
            *e_b++ = (e_fee >> 48) & 0xFFU;
            *e_b++ = (e_fee >> 40) & 0xFFU;
            *e_b++ = (e_fee >> 32) & 0xFFU;
            *e_b++ = (e_fee >> 24) & 0xFFU;
            *e_b++ = (e_fee >> 16) & 0xFFU;
            *e_b++ = (e_fee >> 8) & 0xFFU;
            *e_b++ = (e_fee >> 0) & 0xFFU;
        }

        TRACEHEX(e_txn); // <- final tx blob

        // TXN: Emit/Send Txn
        uint8_t e_hash[32];
        int64_t e_result = emit(SBUF(e_hash), SBUF(e_txn));
        if (e_result < 1)
        {
            TRACEVAR(e_result);
            rollback(SBUF("auction.c: Escrow Finish Failure."), __LINE__);
        }

        return accept(SBUF("auction.c: Completed."), __LINE__);
    }

    // VALIDATE: Bid Txn
    uint8_t ca_buffer[4];
    if (otxn_field(SBUF(ca_buffer), sfCancelAfter) != 4)
    {
        rollback(SBUF("auction.c: Missing Tx Field `CancelAfter`."), __LINE__);
    }

    int32_t cancel_after = UINT32_FROM_BUF(ca_buffer);
    if (cancel_after < end_time)
    {
        rollback(SBUF("auction.c: Invalid Tx Field `CancelAfter`."), __LINE__);
    }

    // recored new bid
    int64_t oslot = otxn_slot(0);
    if (oslot < 0)
        rollback(SBUF("auction.c: Could not slot originating txn."), 1);

    int64_t amt_slot = slot_subfield(oslot, sfAmount, 0);
    if (amt_slot < 0)
        rollback(SBUF("auction.c: Could not slot otxn.sfAmount."), 2);

    int64_t acct_bid_xfl = slot_float(amt_slot);
    if (acct_bid_xfl < 0)
        rollback(SBUF("auction.c: Could not parse amount."), 1);

    int64_t highest_bid_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(auction_buffer + WB_OFFSET));

    if (DEBUG)
    {
        TRACEVAR(acct_bid_xfl);
        TRACEVAR(highest_bid_xfl);
    }

    if (float_compare(acct_bid_xfl, highest_bid_xfl, COMPARE_LESS))
    {
        rollback(SBUF("auction.c: Bid `Amount` to low."), __LINE__);
    }

    // VALIDATE: User Bid
    unsigned char amount_buffer[48];
    int64_t amount_len = otxn_field(SBUF(amount_buffer), sfAmount);
    TRACEHEX(amount_buffer);

    // VALIDATE: Sequence
    uint8_t seq_buffer[4];
    otxn_field(SBUF(seq_buffer), sfSequence);

    TRACEHEX(seq_buffer);

    uint8_t hash_out[32];
    uint8_t data[26];
    data[1] = 'u';
#define ACCT_OUT (data + 2U)
#define SEQ_OUT (data + 22U)
    ACCOUNT_TO_BUF(ACCT_OUT, otx_acc);
    INT32_TO_BUF(SEQ_OUT, seq_buffer);
    TRACEHEX(data);

    util_sha512h(hash_out, 32, SBUF(data));

    uint8_t data_out[80];
#define AMT_OUT (data_out + 0U)
#define HASH_OUT (data_out + 48U)
    AMOUNT_TO_BUF(AMT_OUT, amount_buffer);
    UINT256_TO_BUF(HASH_OUT, hash_out);

    TRACEHEX(data_out);

    uint8_t user_bid[80];
    if (state(SBUF(user_bid), SBUF(otx_acc)) != 80)
    {
        state_set(SBUF(data_out), SBUF(otx_acc));
    }
    else
    {
        // if (BUFFER_EQUAL_AMOUNT(user_bid, amount_buffer))
        // {
        //     rollback(SBUF("auction.c: Duplicate Bid `Amount`."), __LINE__);
        // }
    }

    int64_t count = UINT64_FROM_BUF(auction_buffer + BC_OFFSET);
    TRACEVAR(count);
    TRACEHEX(auction_buffer);
    if (count > 0)
    {
        TRACEHEX(auction_buffer);
        TRACESTR("auction.c: Cancel Last Bid.");
        // ACCOUNT: Hook Account
        ACCOUNT_TO_BUF(E_HOOK_ACC, HOOK_ACC);
        
        // ACCOUNT: Owner Account
        ACCOUNT_TO_BUF(E_OWNER_OUT, auction_buffer + WA_OFFSET);

        // ACCOUNT: Ewscow ID
        UINT256_TO_BUF(E_ID_OUT, auction_buffer + WID_OFFSET);

        // TXN: PREPARE: Init
        etxn_reserve(1);

        // TXN PREPARE: FirstLedgerSequence
        uint32_t fls = (uint32_t)ledger_seq() + 1;
        *((uint32_t *)(E_FLS_OUT)) = FLIP_ENDIAN(fls);

        // TXN PREPARE: LastLedgerSequense
        uint32_t lls = fls + 4;
        *((uint32_t *)(E_LLS_OUT)) = FLIP_ENDIAN(lls);

        // TXN PREPARE: Dest Tag <- Source Tag
        if (otxn_field(E_DTAG_OUT, 4, sfSourceTag) == 4)
            *(E_DTAG_OUT - 1) = 0x2EU;

        // TXN PREPARE: Emit Metadata
        etxn_details(E_EMIT_OUT, 116U);

        // TXN PREPARE: Fee
        {
            int64_t fee = etxn_fee_base(SBUF(e_txn));
            uint8_t *b = E_FEE_OUT;
            *b++ = 0b01000000 + ((fee >> 56) & 0b00111111);
            *b++ = (fee >> 48) & 0xFFU;
            *b++ = (fee >> 40) & 0xFFU;
            *b++ = (fee >> 32) & 0xFFU;
            *b++ = (fee >> 24) & 0xFFU;
            *b++ = (fee >> 16) & 0xFFU;
            *b++ = (fee >> 8) & 0xFFU;
            *b++ = (fee >> 0) & 0xFFU;
        }

        TRACEHEX(e_txn); // <- final tx blob

        // TXN: Emit/Send Txn
        uint8_t emithash[32];
        int64_t emit_result = emit(SBUF(emithash), SBUF(e_txn));
        if (emit_result < 1)
        {
            rollback(SBUF("auction.c: Tx emitted failure."), __LINE__);
        }
    }

    TRACESTR("auction.c: New High Bidder.");
    INT64_TO_BUF(WIN_BID, FLIP_ENDIAN_64(acct_bid_xfl));
    ACCOUNT_TO_BUF(WIN_ACCOUNT, otx_acc);
    UINT256_TO_BUF(WIN_ID, hash_out);

    count++;
    INT64_TO_BUF(NUM_BIDS, count);
    state_set(SBUF(auction_buffer), SBUF(tid_buffer));

    _g(1, 1);

    return accept(SBUF("auction.c: Accept."), __LINE__);
}
