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

#include <stdint.h>
#include "hookapi.h"

#define NOPE(x)\
    rollback(SBUF(x), __LINE__);

#define ACCOUNT_TO_BUF(buf_raw, i)                       \
    {                                                    \
        unsigned char *buf = (unsigned char *)buf_raw;   \
        *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0);   \
        *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8);   \
        *(uint32_t *)(buf + 16) = *(uint32_t *)(i + 16); \
    }

uint8_t sell_txn[305] =
    {
        /* size,upto */
        /*   3,  0  */ 0x12U, 0x00U, 0x07U,                                                             /* tt = OfferCreate */
        /*   5,  3  */ 0x22U, 0x00U, 0x01U, 0x00U, 0x00U,                                               /* flags = tfCanonical */
        /*   5,  8  */ 0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                               /* sequence = 0 */
        /*   6, 13  */ 0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                        /* first ledger seq */
        /*   6, 19  */ 0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                        /* last ledger seq */
        /*  49, 25  */ 0x64U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* taker pays field 9 or 49 bytes */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99,                                                                            /* cont...  */
        /*  49, 74  */ 0x65U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* taker gets field 9 or 49 bytes */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99,                                                                            /* cont...  */
        /*   9, 123 */ 0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                          /* fee      */
                       0x00U,                                                                           /* cont...  */
        /*  35, 132 */ 0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* pubkey   */
        /*  22, 167 */ 0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                           /* src acc  */
        /* 116, 189 */                                                                                  /* emit details */
        /*  34, 305 */ 0x50U, 0x23U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   /* hash256 id */
        /*   0, 339 */
};

// TX BUILDER 
#define SFLAGS_OUT (sell_txn + 4U) 
#define SFLS_OUT (sell_txn + 15U) 
#define SLLS_OUT (sell_txn + 21U) 
#define ST_GETS_OUT (sell_txn + 74U) 
#define ST_PAYS_OUT (sell_txn + 25U) 
#define SFEE_OUT (sell_txn + 124U) 
#define SACCOUNT_OUT (sell_txn + 169U) 
#define SEMIT_OUT (sell_txn + 189U) 
#define SOID_OUT (sell_txn + 307U) 

uint8_t buy_txn[305] =
    {
        /* size,upto */
        /*   3,  0  */ 0x12U, 0x00U, 0x07U,                                                             /* tt = OfferCreate */
        /*   5,  3  */ 0x22U, 0x00U, 0x01U, 0x00U, 0x00U,                                               /* flags = tfCanonical */
        /*   5,  8  */ 0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                               /* sequence = 0 */
        /*   6, 13  */ 0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                        /* first ledger seq */
        /*   6, 19  */ 0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                        /* last ledger seq */
        /*  49, 25  */ 0x64U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* taker pays field 9 or 49 bytes */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99,                                                                            /* cont...  */
        /*  49, 74  */ 0x65U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* taker gets field 9 or 49 bytes */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* cont...  */
                       0x99,                                                                            /* cont...  */
        /*   9, 123 */ 0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                          /* fee      */
                       0x00U,                                                                           /* cont...  */
        /*  35, 132 */ 0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* pubkey   */
        /*  22, 167 */ 0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                           /* src acc  */
        /* 116, 189 */                                                                                  /* emit details */
        /*  34, 305 */ 0x50U, 0x23U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   /* hash256 id */
        /*   0, 339 */
};

// TX BUILDER 
#define BFLAGS_OUT (buy_txn + 4U) 
#define BFLS_OUT (buy_txn + 15U) 
#define BLLS_OUT (buy_txn + 21U) 
#define BT_GETS_OUT (buy_txn + 74U) 
#define BT_PAYS_OUT (buy_txn + 25U) 
#define BFEE_OUT (buy_txn + 124U) 
#define BACCOUNT_OUT (buy_txn + 169U) 
#define BEMIT_OUT (buy_txn + 189U) 
#define BOID_OUT (buy_txn + 307U) 

#define PREPARE_BUY_TXN(acct_buf, currency_buf, issuer_buf, taker_gets_xfl, taker_pays_xfl) do { \ 
    uint32_t fls = (uint32_t)ledger_seq() + 1; \ 
    *((uint32_t *)(BFLS_OUT)) = FLIP_ENDIAN(fls); \ 
    uint32_t lls = fls + 4; \ 
    *((uint32_t *)(BLLS_OUT)) = FLIP_ENDIAN(lls); \ 
    float_sto(BT_PAYS_OUT, 49, currency_buf, 20, issuer_buf, 20, taker_pays_xfl, sfTakerPays); \ 
    uint64_t drops = float_int(taker_gets_xfl, 6, 1); \
    uint8_t* tp = BT_GETS_OUT + 1; \
    *tp++ = 0b01000000 + (( drops >> 56 ) & 0b00111111 ); \
    *tp++ = (drops >> 48) & 0xFFU; \
    *tp++ = (drops >> 40) & 0xFFU; \
    *tp++ = (drops >> 32) & 0xFFU; \
    *tp++ = (drops >> 24) & 0xFFU; \
    *tp++ = (drops >> 16) & 0xFFU; \
    *tp++ = (drops >>  8) & 0xFFU; \
    *tp++ = (drops >>  0) & 0xFFU; \
    ACCOUNT_TO_BUF(BACCOUNT_OUT, acct_buf); \ 
    etxn_details(BEMIT_OUT, 116U); \
    int64_t fee = etxn_fee_base(SBUF(buy_txn)); \ 
    uint8_t *b = BFEE_OUT; \ 
    *b++ = 0b01000000 + ((fee >> 56) & 0b00111111); \ 
    *b++ = (fee >> 48) & 0xFFU; \ 
    *b++ = (fee >> 40) & 0xFFU; \ 
    *b++ = (fee >> 32) & 0xFFU; \ 
    *b++ = (fee >> 24) & 0xFFU; \ 
    *b++ = (fee >> 16) & 0xFFU; \ 
    *b++ = (fee >> 8) & 0xFFU; \ 
    *b++ = (fee >> 0) & 0xFFU; \ 
    TRACEHEX(buy_txn); \ 
} while(0) 

#define PREPARE_SELL_TXN(acct_buf, currency_buf, issuer_buf, taker_gets_xfl, taker_pays_xfl) do { \ 
    uint32_t fls = (uint32_t)ledger_seq() + 1; \ 
    *((uint32_t *)(SFLS_OUT)) = FLIP_ENDIAN(fls); \ 
    uint32_t lls = fls + 4; \ 
    *((uint32_t *)(SLLS_OUT)) = FLIP_ENDIAN(lls); \ 
    float_sto(ST_GETS_OUT, 49, currency_buf, 20, issuer_buf, 20, taker_gets_xfl, sfTakerGets); \ 
    uint64_t drops = float_int(taker_pays_xfl, 6, 1); \
    uint8_t* tp = ST_PAYS_OUT + 1; \
    *tp++ = 0b01000000 + (( drops >> 56 ) & 0b00111111 ); \
    *tp++ = (drops >> 48) & 0xFFU; \
    *tp++ = (drops >> 40) & 0xFFU; \
    *tp++ = (drops >> 32) & 0xFFU; \
    *tp++ = (drops >> 24) & 0xFFU; \
    *tp++ = (drops >> 16) & 0xFFU; \
    *tp++ = (drops >>  8) & 0xFFU; \
    *tp++ = (drops >>  0) & 0xFFU; \
    ACCOUNT_TO_BUF(SACCOUNT_OUT, acct_buf); \ 
    etxn_details(SEMIT_OUT, 116U); \
    int64_t fee = etxn_fee_base(SBUF(sell_txn)); \ 
    uint8_t *b = SFEE_OUT; \ 
    *b++ = 0b01000000 + ((fee >> 56) & 0b00111111); \ 
    *b++ = (fee >> 48) & 0xFFU; \ 
    *b++ = (fee >> 40) & 0xFFU; \ 
    *b++ = (fee >> 32) & 0xFFU; \ 
    *b++ = (fee >> 24) & 0xFFU; \ 
    *b++ = (fee >> 16) & 0xFFU; \ 
    *b++ = (fee >> 8) & 0xFFU; \ 
    *b++ = (fee >> 0) & 0xFFU; \ 
    TRACEHEX(sell_txn); \ 
} while(0) 

int64_t ten_xfl = 6107881094714392576;

int64_t hook(uint32_t r)
{
    etxn_reserve(2);

    // HookOn: OfferCreate
    int64_t tt = otxn_type();
    if (tt != ttINVOKE && tt != ttOFFER_CREATE)
        DONE("trailing_stop.da: Passing invalid txn type. HookOn should be changed to avoid this.");

    // ACCOUNT: Originating Transaction
    uint8_t otxn_accid[32];
    otxn_field(otxn_accid + 12, 20, sfAccount);

    // ACCOUNT: Hook
    uint8_t hook_accid[32];
    hook_account(hook_accid + 12, 20);

    if (BUFFER_EQUAL_20(hook_accid, otxn_accid) && tt != ttINVOKE)
    {
        accept(SBUF("trailing_stop.da: Outgoing Non Invoke Tx"), __LINE__);
    }

    uint8_t op;
    otxn_param(&op, 1, "OP", 2);
    
    // OTXN PARAMATER: Market Price
    uint8_t mp_buffer[8];
    uint8_t mp_key[2] = {'M', 'P'};
    int64_t mp_result = otxn_param(SBUF(mp_buffer), mp_key, 2);
    int64_t xfl_mp = *((int64_t*)mp_buffer);

    // OTXN PARAMATER: Quantity
    uint8_t qty_buffer[8];
    uint8_t qty_key[3] = {'Q', 'T', 'Y'};
    int64_t qty_result = otxn_param(SBUF(qty_buffer), qty_key, 3);
    int64_t xfl_qty = *((int64_t*)qty_buffer);

    // OTXN PARAMATER: Issuer (Currency + Issuer)
    uint8_t issue_buffer[40];
    uint8_t issue_key[3] = {'I', 'S', 'S'};
    int64_t issue_result = otxn_param(SBUF(issue_buffer), issue_key, 3);

    // OTXN PARAMATER: Lower Percentage
    uint8_t lp_buffer[8];
    uint8_t lp_key[2] = {'L', 'P'};
    int64_t lp_result = otxn_param(SBUF(lp_buffer), lp_key, 2);
    int64_t xfl_lp = *((int64_t*)lp_buffer);

    // OTXN PARAMATER: Upper Percentage
    uint8_t up_buffer[8];
    uint8_t up_key[2] = {'U', 'P'};
    int64_t up_result = otxn_param(SBUF(up_buffer), up_key, 2);
    int64_t xfl_up = *((int64_t*)up_buffer);

    uint8_t hash[32];
    util_sha512h(SBUF(hash), issue_buffer, 40);
    int64_t sup_result = state_foreign(SBUF(lp_buffer), lp_key, 2, SBUF(hash), hook_accid + 12, 20);
    int64_t slp_result = state_foreign(SBUF(up_buffer), up_key, 2, SBUF(hash), hook_accid + 12, 20);
    
    // action
    switch (op)
    {
        case 'I':
        {
            if (sup_result != DOESNT_EXIST && slp_result != DOESNT_EXIST)
            {
                DONE("trailing_stop.da: Issue already initialized.");
            }

            int64_t xs_rate_xfl = float_sum(xfl_mp, float_multiply(xfl_mp, xfl_up));
            int64_t sell_tg_xfl = float_divide(ten_xfl, xs_rate_xfl);
            TRACEVAR(xs_rate_xfl);
            TRACEVAR(sell_tg_xfl);

            // (TakerGets / 1'000'000) / TakerPays.value

            PREPARE_SELL_TXN(hook_accid + 12, issue_buffer, issue_buffer + 20, sell_tg_xfl, ten_xfl);
            uint8_t s_emithash[32];
            int64_t s_emit_result = emit(SBUF(s_emithash), SBUF(sell_txn));
            if (s_emit_result < 0)
            {
                rollback(SBUF("trailing_stop.da: Emit `Upper` Failure."), __LINE__);
            }
            
            
            int64_t xb_rate_xfl = float_sum(xfl_mp, float_negate(float_multiply(xfl_mp, xfl_lp)));
            int64_t buy_tp_xfl = float_divide(ten_xfl, xb_rate_xfl);
            TRACEVAR(xb_rate_xfl);
            TRACEVAR(buy_tp_xfl);

            // (TakerPays / 1'000'000) / TakerGets.value

            PREPARE_BUY_TXN(hook_accid + 12, issue_buffer, issue_buffer + 20, ten_xfl, buy_tp_xfl);
            uint8_t b_emithash[32];
            int64_t b_emit_result = emit(SBUF(b_emithash), SBUF(buy_txn));
            if (b_emit_result < 0)
            {
                rollback(SBUF("trailing_stop.da: Emit `Lower` Failure."), __LINE__);
            }


            int64_t _qty_result = state_foreign_set(SBUF(qty_buffer), qty_key, 3, SBUF(hash), hook_accid + 12, 20);
            int64_t _up_result = state_foreign_set(SBUF(lp_buffer), lp_key, 2, SBUF(hash), hook_accid + 12, 20);
            int64_t _lp_result = state_foreign_set(SBUF(up_buffer), up_key, 2, SBUF(hash), hook_accid + 12, 20);
            accept(SBUF("trailing_stop.da: Initialized."), __LINE__);
        }
    }

    ASSERT(otxn_slot(1) == 1);
    ASSERT(slot_subfield(1, sfTakerGets, 2) == 2);
    ASSERT(slot_subfield(1, sfTakerPays, 3) == 3);

    uint8_t tg_buffer[48];
    int64_t tg_result = slot(SBUF(tg_buffer), 2);

    uint8_t tp_buffer[48];
    int64_t tp_result = slot(SBUF(tp_buffer), 3);

    int64_t tg_is_native = tg_result == 8;

    uint8_t _hash[32];
    util_sha512h(SBUF(_hash), tg_is_native ? tp_buffer + 8 : tg_buffer + 8, 40);
    
    int64_t _qty_result = state_foreign(SBUF(qty_buffer), qty_key, 2, SBUF(_hash), hook_accid + 12, 20);
    int64_t _xfl_qty = *((int64_t*)qty_buffer);

    int64_t _up_result = state_foreign(SBUF(up_buffer), up_key, 2, SBUF(_hash), hook_accid + 12, 20);
    int64_t _xfl_up = *((int64_t*)up_buffer);
    
    int64_t _lp_result = state_foreign(SBUF(lp_buffer), lp_key, 2, SBUF(_hash), hook_accid + 12, 20);
    int64_t _xfl_lp = *((int64_t*)lp_buffer);

    TRACEVAR(_xfl_qty);
    TRACEVAR(_xfl_up);
    TRACEVAR(_xfl_lp);
    
    if (tg_is_native)
    {
        // (sfTakerGets / sfTakerPays.value) / 1000000
        // Buy (Asset Appreciated)
        int64_t tg_drops = AMOUNT_TO_DROPS(tg_buffer);
        int64_t tg_xfl = float_set(-6, tg_drops);
        int64_t tp_xfl = -INT64_FROM_BUF(tp_buffer);
        int64_t mp_xfl = float_divide(tg_xfl, tp_xfl);
        TRACEVAR(mp_xfl);
        
        // Update Upper Trail
        int64_t xs_rate_xfl = float_sum(mp_xfl, float_multiply(mp_xfl, _xfl_up));
        int64_t sell_tg_xfl = float_divide(ten_xfl, xs_rate_xfl);
        TRACEVAR(xs_rate_xfl);
        PREPARE_SELL_TXN(hook_accid + 12, tp_buffer + 8, tp_buffer + 28, sell_tg_xfl, ten_xfl);
        uint8_t _emithash[32];
        int64_t _emit_result = emit(SBUF(_emithash), SBUF(sell_txn));
        if (_emit_result < 0)
        {
            rollback(SBUF("trailing_stop.da: Emit `Upper` Failure."), __LINE__);
        }
        
        // Update Lower Trail
        int64_t xb_rate_xfl = float_sum(mp_xfl, float_negate(float_multiply(mp_xfl, _xfl_lp)));
        int64_t buy_tp_xfl = float_divide(ten_xfl, xb_rate_xfl);
        TRACEVAR(xb_rate_xfl);
        PREPARE_BUY_TXN(hook_accid + 12, tp_buffer + 8, tp_buffer + 28, ten_xfl, buy_tp_xfl);
        uint8_t emithash[32];
        int64_t emit_result = emit(SBUF(emithash), SBUF(buy_txn));
        if (emit_result < 0)
        {
            rollback(SBUF("trailing_stop.da: Emit `Lower` Failure."), __LINE__);
        }
        accept(SBUF("trailing_stop.da: Emit `Update`."), __LINE__);
    }
    else
    {
        // (sfTakerPays / sfTakerGets.value) / 1000000
        // Sell (Asset Depreciated)
        int64_t tp_drops = AMOUNT_TO_DROPS(tp_buffer);
        int64_t tp_xfl = float_set(-6, tp_drops);
        int64_t tg_xfl = -INT64_FROM_BUF(tg_buffer);
        int64_t mp_xfl = float_divide(tp_xfl, tg_xfl);
        int64_t xs_rate_xfl = mp_xfl;
        int64_t sell_tp_xfl = float_divide(_xfl_qty, xs_rate_xfl);
        PREPARE_SELL_TXN(hook_accid + 12, tg_buffer + 8, tg_buffer + 28, _xfl_qty, sell_tp_xfl);
        uint8_t s_emithash[32];
        int64_t s_emit_result = emit(SBUF(s_emithash), SBUF(sell_txn));
        if (s_emit_result < 0)
        {
            rollback(SBUF("trailing_stop.da: Emit `Sell` Failure."), __LINE__);
        }
        accept(SBUF("trailing_stop.da: Emit `Sell`."), __LINE__);
    }

    rollback(SBUF("trailing_stop.da: Failure."), __LINE__);

    _g(1, 1);
    // unreachable
    return 0;
}