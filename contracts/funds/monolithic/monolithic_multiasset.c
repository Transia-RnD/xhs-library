//------------------------------------------------------------------------------
/*

HookParameters -> State:

ADM: The admin account (role)
STL: The settler account (role)
RFD: The refunder account (role)
WKEY: The withdrawer pubkey (role)

CUR: The currency allowed in the funding source
ISS: The issuer account allowed in the funding source
ACC: The settlement account (globally per asset)

for each asset:
     <20 byte currency><20 byte issuer>: <20 byte settlement address>

Operations:

// payment ops are: 
    // D - deposit (any)

// invoke ops are: (role)
    // I - initalize (any)
    // B - debit (settler)
    // S - settle (settler)
    // U/P - un/pause (admin)
    // M - modify (admin)
    // R - refund (refunder)
    // W - withdraw (user)
    // A - asset (admin)

// invoke (Modify) sub ops are:
    // A - modify admin account (role)
    // S - modify settler account (role)
    // W - modify withdraw pubkey (role)

// invoke (Asset) sub ops are:
    // C - modify create asset
    // D - modify delete asset

*/
//==============================================================================

#include "hookapi.h"

#define SVAR(x) &(x), sizeof(x)

#define DONE(x)\
    return accept(SBUF(x), __LINE__)

#define NOPE(x)\
    return rollback(SBUF(x), __LINE__)


#define FLIP_ENDIAN_64(n) ((uint64_t)(((n & 0xFFULL) << 56ULL) |             \
                                      ((n & 0xFF00ULL) << 40ULL) |           \
                                      ((n & 0xFF0000ULL) << 24ULL) |         \
                                      ((n & 0xFF000000ULL) << 8ULL) |        \
                                      ((n & 0xFF00000000ULL) >> 8ULL) |      \
                                      ((n & 0xFF0000000000ULL) >> 24ULL) |   \
                                      ((n & 0xFF000000000000ULL) >> 40ULL) | \
                                      ((n & 0xFF00000000000000ULL) >> 56ULL)))

uint8_t txn_out[300] =
{
/* size,upto */
/*   3,   0 */   0x12U, 0x00U, 0x00U,                                                               /* tt = Payment */
/*   5,   3 */   0x22U, 0x80U, 0x00U, 0x00U, 0x00U,                                          /* flags = tfCanonical */
/*   5,   8 */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                                 /* sequence = 0 */
/*   6,  13 */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                      /* first ledger seq */
/*   6,  19 */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                       /* last ledger seq */
/*   1,  25 */   0x61U, 
/*   8,  26 */   0,0,0,0,0,0,0,0,                                                                        /* amt val */
/*  20,  34 */   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                                /* amt cur */
// vvvvvvvvvvvvvvvvvv ISSUER ACC ID vvvvvvvvvvvvvvvvvvvvvvv
/*  20,  54 */   
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

/*   9,  74 */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                         /* fee      */
/*  35,  83 */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       /* pubkey   */
/*   2, 118 */   0x81U, 0x14U,                                                                  /* src acc preamble */

/*  20, 120 */   0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,                                             /* src acc */
/*   2, 140 */   0x83U, 0x14U,                                                                 /* dest acc preamble */
/*  20, 142 */   0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,                                            /* dest acc */

/* 138, 162 */   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    /* emit detail */
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
/*   0, 300 */
};

#define TLACC (txn_out + 54) /* set it above!! */

#define TXNLEN 300
#define FEEOUT (txn_out + 75)
#define EMITDET (txn_out + 162)
#define HOOKACC (txn_out + 120)
#define OTXNACC (txn_out + 142)
#define DTAG_OUT (txn_out + 142)
#define DESTACC (txn_out + 142)
#define OUTAMT (txn_out + 25)
#define OUTCUR (txn_out + 34)
#define OUTISS (txn_out + 54)
#define CURSHORT (txn_out + 46)

// if we make a TrustSet instead (used for initialisation) then we'll truncate the template above
#define TTOUT (txn_out + 2)         // when it's a TrustSet we set this to 0x14
#define OUTAMT_TL (txn_out + 25)    // when it's a TrustSet, we set this to 0x63
#define EMITDET_TL (txn_out + 140)  // when it's a TrustSet Emit Details occurs sooner
#define TXNLEN_TL 278               // .. and the txn is smaller

#define BE_DROPS(drops)\
{\
        uint64_t drops_tmp = drops;\
        uint8_t* b = (uint8_t*)&drops;\
        *b++ = 0b01000000 + (( drops_tmp >> 56 ) & 0b00111111 );\
        *b++ = (drops_tmp >> 48) & 0xFFU;\
        *b++ = (drops_tmp >> 40) & 0xFFU;\
        *b++ = (drops_tmp >> 32) & 0xFFU;\
        *b++ = (drops_tmp >> 24) & 0xFFU;\
        *b++ = (drops_tmp >> 16) & 0xFFU;\
        *b++ = (drops_tmp >>  8) & 0xFFU;\
        *b++ = (drops_tmp >>  0) & 0xFFU;\
}

#define COPY_20(src, dst)\
{\
    *((uint64_t*)(((uint8_t*)(dst)) + 0)) = \
    *((uint64_t*)(((uint8_t*)(src)) + 0));\
    *((uint64_t*)(((uint8_t*)(dst)) + 8)) = \
    *((uint64_t*)(((uint8_t*)(src)) + 8));\
    *((uint32_t*)(((uint8_t*)(dst)) + 16)) = \
    *((uint32_t*)(((uint8_t*)(src)) + 16));\
}

#define ACCOUNT_TO_BUF(buf_raw, i)                       \
{                                                    \
    unsigned char *buf = (unsigned char *)buf_raw;   \
    *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0);   \
    *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8);   \
    *(uint32_t *)(buf + 16) = *(uint32_t *)(i + 16); \
}

int64_t cbak(uint32_t f)
{
    // TODO track withdrawal txns to see if they successfully executed
    return 0;
}

int64_t hook(uint32_t r)
{
    _g(1,1);

    etxn_reserve(1);

    uint8_t hook_accid[32];
    hook_account(hook_accid + 12, 20);

    uint8_t otxn_accid[32];
    otxn_field(otxn_accid + 12, 20, sfAccount);

    if (BUFFER_EQUAL_20(hook_accid + 12, otxn_accid + 12))
        DONE("Monolithic: passing outgoing txn");

    int64_t tt = otxn_type();
    if (tt != ttINVOKE && tt != ttPAYMENT)
        NOPE("Monolithic: Rejecting non-Invoke, non-Payment txn.");

    otxn_slot(1);

    slot_subfield(1, sfAmount, 2);

    uint8_t amt[48];

    if (slot_size(2) == 8)
        DONE("Monolithic: Passing incoming XAH payment.");

    // get admin account (role)
    uint8_t _admin[20];
    if (hook_param(SBUF(_admin), "ADM", 3) != 20)
        NOPE("Monolithic: Misconfigured. Missing ADM install parameter.");

    // get settlement account (role)
    uint8_t _stl[20];
    if (hook_param(SBUF(_stl), "STL", 3) != 20)
        NOPE("Monolithic: Misconfigured. Missing STL install parameter.");

    // get the withdrawal signing key
    uint8_t _wkey[33];
    if (hook_param(SBUF(_wkey), "WKEY", 4) != 33)
        NOPE("Monolithic: Misconfigured. Missing WKEY install parameter.");

    // get currency
    if (hook_param(OUTCUR, 20, "CUR", 3) != 20)
        NOPE("Monolithic: Misconfigured. Missing CUR install parameter.");

    // get currency issuer
    if (hook_param(OUTISS, 20, "ISS", 3) != 20)
        NOPE("Monolithic: Misconfigured. Missing ISS install parameter.");
    
    // get settlement account (asset settlement account)
    uint8_t acc[20];
    if (hook_param(SBUF(acc), "ACC", 3) != 20)
        NOPE("Monolithic: Misconfigured. Missing ACC install parameter.");

    // Operation
    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1)
        NOPE("Monolithic: Missing OP parameter on Invoke.");
    
    // Sub Operation
    uint8_t sop;
    if (op == 'M' && otxn_param(&sop, 1, "SOP", 3) != 1)
        NOPE("Monolithic: Missing SOP parameter on Invoke.");

    int64_t xfl_in;
    uint32_t flags;
    uint32_t dtag;

    if (tt == ttPAYMENT)
    {
        // this will fail if flags isn't in the txn, that's also ok.
        otxn_field(&flags, 4, sfFlags);
        
        // check for partial payments (0x00020000) -> (0x00000200 LE)
        if (flags & 0x200U)
            NOPE("Monolithic: Partial payments are not supported.");

        otxn_field(SBUF(amt), sfAmount);

        if (otxn_field(SVAR(dtag), sfDestinationTag) != 4)
            NOPE("Monolithic: Destination Tag is Required.");

        if (!BUFFER_EQUAL_20(amt + 8, OUTCUR))
            NOPE("Monolithic: Wrong currency.");

        if (!BUFFER_EQUAL_20(amt + 28, OUTISS))
            NOPE("Monolithic: Wrong issuer.");

        xfl_in = slot_float(2);

        if (xfl_in < 0 || !float_compare(xfl_in, 0, COMPARE_GREATER))
            NOPE("Monolithic: Invalid sfAmount.");
    }

    // enforced pausedness
    if (op != 'U')
    {
        uint8_t paused;
        state(&paused, 1, "P", 1);
        if (paused)
            NOPE("Monolithic: Paused.");
    }

    // check if the trustline exists
    uint8_t keylet[34];
    util_keylet(keylet, 34, KEYLET_LINE, hook_accid + 12, 20, OUTISS, 20, OUTCUR, 20);

    int64_t already_setup = (slot_set(SBUF(keylet), 10) == 10);

    // enforce initalisation
    if (!already_setup && op != 'I')
        NOPE("Monolithic: Send op=I initalisation first.");

    uint8_t admin[20];
    uint8_t stl[20];
    uint8_t wkey[33];
    if (already_setup && op != 'I')
    {
        state(SBUF(admin), "ADM", 3);
        state(SBUF(stl), "STL", 3);
        state(SBUF(wkey), "WKEY", 4);
    }

    int64_t is_admin = BUFFER_EQUAL_20(otxn_accid + 12, admin);
    int64_t is_stl = BUFFER_EQUAL_20(otxn_accid + 12, stl);

    // sanity check
    if ((op == 'D') && tt != ttPAYMENT)
        NOPE("Monolithic: Deposit operations must be a payment transaction.");

    // permission check
    if (!is_admin && (op == 'U' || op == 'P' || op == 'M'))
        NOPE("Monolithic: Admin only operation.");
    
    if (!is_stl && (op == 'B' || op == 'S'))
        NOPE("Monolithic: Settler only operation.");

    // current ledger seq is used when emitting a txn
    int64_t seq = ledger_seq() + 1;

    uint8_t stl_bal[8];
    state(SBUF(stl_bal), hook_accid + 12, 20);
    int64_t stl_bal_xfl = *((int64_t*)stl_bal);

    // action
    switch (op)
    {
        case 'I':
        {
            if (already_setup)
                DONE("Monolithic: Already setup trustline.");

            ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);
            ACCOUNT_TO_BUF(OTXNACC, otxn_accid + 12);

            // create a trustline ...
            uint8_t xfl_buffer[8];
            if (otxn_param(xfl_buffer, 8, "AMT", 3) != 8)
                NOPE("Monolithic: Misconfigured. Missing AMT otxn parameter.");

            int64_t xfl_out = *((int64_t *)xfl_buffer);

            // write limit amount
            float_sto(OUTAMT_TL, 49, OUTCUR, 20, OUTISS, 20, xfl_out, sfLimitAmount);
                    
            // set the template transaction type to trustset
            *TTOUT = 0x14U;

            etxn_details(EMITDET_TL, 138);
            int64_t fee = etxn_fee_base(txn_out, TXNLEN_TL);
            BE_DROPS(fee);
            *((uint64_t*)(FEEOUT)) = fee;

            txn_out[15] = (seq >> 24U) & 0xFFU;
            txn_out[16] = (seq >> 16U) & 0xFFU;
            txn_out[17] = (seq >>  8U) & 0xFFU;
            txn_out[18] = seq & 0xFFU;

            seq += 4;
            txn_out[21] = (seq >> 24U) & 0xFFU;
            txn_out[22] = (seq >> 16U) & 0xFFU;
            txn_out[23] = (seq >>  8U) & 0xFFU;
            txn_out[24] = seq & 0xFFU;
    
            trace(SBUF("emit:"), txn_out, TXNLEN_TL, 1);

            uint8_t emithash[32];
            int64_t emit_result = emit(SBUF(emithash), txn_out, TXNLEN_TL);
            TRACEVAR(emit_result);

            state_set(_admin, 20, "ADM", 3);
            state_set(_stl, 20, "STL", 3);
            state_set(_wkey, 33, "WKEY", 4);
            
            // save asset
            uint8_t asset[40];
            COPY_20(OUTCUR, asset);
            COPY_20(OUTISS, asset);
            state_set(acc, 20, asset, 40);
            DONE("Monolithic: Emitted TrustSet to initialize.");
        }

        case 'U':
        case 'P':
        {
            // pause
            uint8_t paused = (op == 'P' ? 1 : 0);
            state_set(&paused, 1, "P", 1);
            DONE("Monolithic: Paused/Unpaused.");
        }

        case 'D':
        {
            int64_t dtag_bal_xfl;
            state(SVAR(dtag_bal_xfl), SVAR(dtag));

            int64_t final_bal_xfl = float_sum(dtag_bal_xfl, xfl_in);
            if (final_bal_xfl < 0)
                NOPE("Monolithic: Insane balance on deposit.");

            if (state_set(SVAR(final_bal_xfl), SVAR(dtag)) != 8)
                NOPE("Monolithic: Failed to set state on deposit.");

            DONE("Monolithic: Deposited.");
        }

        // debit
        case 'B':
        {
            if (otxn_param(SVAR(dtag), "TAG", 3) != 4)
                NOPE("Monolithic: Misconfigured. Missing TAG otxn parameter.");

            int64_t sig_amt;
            if (otxn_param(SVAR(sig_amt), "AMT", 3) != 8 || sig_amt < 0)
                NOPE("Monolithic: Misconfigured. Missing AMT otxn parameter.");

            uint32_t sig_nce;
            if (otxn_param(SVAR(sig_nce), "SEQ", 3) != 4)
                NOPE("Monolithic: Misconfigured. Missing SEQ otxn parameter.");

            // check the nonce
            uint32_t dbt_seq;
            state(SVAR(dbt_seq), otxn_accid + 12, 20);

            if (dbt_seq != sig_nce)
                NOPE("Monolithic: Debit nonce out of sequence.");

            int64_t dtag_bal_xfl;
            state(SVAR(dtag_bal_xfl), SVAR(dtag));

            // check dtag balance
            if (dtag_bal_xfl <= 0 || !float_compare(dtag_bal_xfl, 0, COMPARE_GREATER))
                NOPE("Monolithic: Insane balance on dtag.");

            if (sig_amt <= 0)
                NOPE("Monolithic: Must provide AMT param when performing debit.");

            if (float_compare(sig_amt, dtag_bal_xfl, COMPARE_GREATER))
                NOPE("Monolithic: Balance not high enough for this debit.");

            int64_t sub_dtag_bal_xfl = float_sum(dtag_bal_xfl, float_negate(sig_amt));
            if (state_set(SVAR(sub_dtag_bal_xfl), SVAR(dtag)) != 8)
                NOPE("Monolithic: Insane balance on dtag.");

            int64_t add_stl_bal_xfl = float_sum(stl_bal_xfl, sig_amt);
            if (state_set(SVAR(add_stl_bal_xfl), hook_accid + 12, 20) != 8)
                NOPE("Monolithic: Insane balance on stl.");

            dbt_seq++;
            if (state_set(SVAR(dbt_seq), otxn_accid + 12, 20) != 4)
                NOPE("Monolithic: Failed to set state.");

            DONE("Monolithic: Debited.");
            break;
        }

        // settlement
        case 'S':
        {
            ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);
            
            // check trustline balance
            slot_subfield(10, sfBalance, 11);
            if (slot_size(11) != 48)
                NOPE("Monolithic: Could not fetch trustline balance.");
    
            int64_t xfl_bal = slot_float(11);

            if (xfl_bal <= 0 || !float_compare(xfl_bal, 0, COMPARE_GREATER))
                NOPE("Monolithic: Insane balance on trustline.");

            // set the destination addr to the settlement addr
            COPY_20(acc, DESTACC);

            int64_t sig_amt;
            if (otxn_param(SVAR(sig_amt), "AMT", 3) != 8 || sig_amt < 0)
                NOPE("Monolithic: Misconfigured. Missing AMT otxn parameter.");

            uint32_t sig_nce;
            if (otxn_param(SVAR(sig_nce), "SEQ", 3) != 4)
                NOPE("Monolithic: Misconfigured. Missing SEQ otxn parameter.");

            // check the nonce
            uint32_t stl_seq;
            state(SVAR(stl_seq), otxn_accid + 12, 20);

            if (stl_seq != sig_nce)
                NOPE("Monolithic: Settlement nonce out of sequence.");

            if (sig_amt <= 0)
                NOPE("Monolithic: Settlement amount must be greater than 0.");

            TRACEVAR(sig_amt);
            TRACEVAR(xfl_bal);
            if (float_compare(sig_amt, xfl_bal, COMPARE_GREATER))
                NOPE("Monolithic: Balance not high enough for this settlement.");

            // write payment amount
            float_sto(OUTAMT, 49, OUTCUR, 20, OUTISS, 20, sig_amt, sfAmount);

            etxn_details(EMITDET, 138);
            int64_t fee = etxn_fee_base(txn_out, TXNLEN);
            BE_DROPS(fee);
            *((uint64_t*)(FEEOUT)) = fee;

            txn_out[15] = (seq >> 24U) & 0xFFU;
            txn_out[16] = (seq >> 16U) & 0xFFU;
            txn_out[17] = (seq >>  8U) & 0xFFU;
            txn_out[18] = seq & 0xFFU;

            seq += 4;
            txn_out[21] = (seq >> 24U) & 0xFFU;
            txn_out[22] = (seq >> 16U) & 0xFFU;
            txn_out[23] = (seq >>  8U) & 0xFFU;
            txn_out[24] = seq & 0xFFU;
    
            trace(SBUF("emit:"), txn_out, TXNLEN, 1);

            uint8_t emithash[32];
            int64_t emit_result = emit(SBUF(emithash), txn_out, TXNLEN);
            if (emit_result < 0)
            {
                NOPE("Monolithic: Settle Emitted Failure.");
            }

            int64_t sub_stl_bal_xfl = float_sum(stl_bal_xfl, float_negate(sig_amt));
            if (state_set(SVAR(sub_stl_bal_xfl), hook_accid + 12, 20) != 8)
                NOPE("Monolithic: Insane balance on stl.");

            stl_seq++;
            if (state_set(SVAR(stl_seq), otxn_accid + 12, 20) != 4)
                NOPE("Monolithic: Failed to set state.");

            DONE("Monolithic: Emitted settlement.");
            break;
        }
        
        // withdrawal
        case 'W':
        {

            // get signature if any
            // Signature format is packed binary data of the form:
            // <20 byte dest accid><8 byte le xfl amount><4 byte le int expiry timestamp><4 byte le int nonce><signature>
            uint8_t sig_buf[256];
            int64_t sig_len = otxn_param(SBUF(sig_buf), "SIG", 3);

            // place pointers according to packed data
            uint8_t* sig_acc = sig_buf;
            uint64_t sig_amt = *((uint64_t*)(sig_buf + 20));
            uint32_t sig_dtag = *((uint32_t*)(sig_buf + 28));
            uint32_t sig_exp = *((uint32_t*)(sig_buf + 32));
            uint32_t sig_nce = *((uint32_t*)(sig_buf + 36));
            uint8_t* sig = sig_buf + 40;

            if (sig_len > 0)
            {
                if (sig_len < 80)
                    NOPE("Monolithic: Signature too short.");

                if (!util_verify(sig_buf, 40, sig_buf + 40, sig_len - 40, SBUF(wkey)))
                    NOPE("Monolithic: Signature verification failed.");
            }

            ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);
            ACCOUNT_TO_BUF(OTXNACC, sig_acc);

            TRACEVAR(sig_dtag);
            
            // check dtag balance
            int64_t dtag_bal_xfl;
            state(SVAR(dtag_bal_xfl), SVAR(sig_dtag));

            if (sig_len <= 0)
                NOPE("Monolithic: Missing SIG parameter.");

            int64_t time = ledger_last_time();
            if (time > sig_exp)
                NOPE("Monolithic: Ticket has expired.");

            if (!BUFFER_EQUAL_20(sig_acc, otxn_accid + 12))
                NOPE("Monolithic: Wrong account for ticket.");

            // check the nonce
            uint32_t wth_seq;
            state(SVAR(wth_seq), otxn_accid + 12, 20);
            TRACEVAR(wth_seq);

            if (wth_seq != sig_nce)
                NOPE("Monolithic: Nonce out of sequence.");

            // check bal can support withdraw
            if (float_compare(sig_amt, dtag_bal_xfl, COMPARE_GREATER))
                NOPE("Monolithic: Balance not high enough for this withdrawal.");

            // write payment amount
            float_sto(OUTAMT, 49, OUTCUR, 20, OUTISS, 20, sig_amt, sfAmount);

            etxn_details(EMITDET, 138);
            int64_t fee = etxn_fee_base(txn_out, TXNLEN);
            BE_DROPS(fee);
            *((uint64_t*)(FEEOUT)) = fee;

            txn_out[15] = (seq >> 24U) & 0xFFU;
            txn_out[16] = (seq >> 16U) & 0xFFU;
            txn_out[17] = (seq >>  8U) & 0xFFU;
            txn_out[18] = seq & 0xFFU;

            seq += 4;
            txn_out[21] = (seq >> 24U) & 0xFFU;
            txn_out[22] = (seq >> 16U) & 0xFFU;
            txn_out[23] = (seq >>  8U) & 0xFFU;
            txn_out[24] = seq & 0xFFU;
    
            trace(SBUF("emit:"), txn_out, TXNLEN, 1);

            uint8_t emithash[32];
            int64_t emit_result = emit(SBUF(emithash), txn_out, TXNLEN);
            if (emit_result < 0)
            {
                NOPE("Monolithic: Withdrawal Emitted Failure.");
            }

            int64_t sub_stl_bal_xfl = float_sum(stl_bal_xfl, float_negate(sig_amt));
            if (state_set(SVAR(sub_stl_bal_xfl), hook_accid + 12, 20) != 8)
                NOPE("Monolithic: Insane balance on stl.");

            // update nonce
            wth_seq++;
            if (state_set(SVAR(wth_seq), otxn_accid + 12, 20) != 4)
                NOPE("Monolithic: Failed to set state.");

            DONE("Monolithic: Emitted withdrawal.");
            break;
        }

        // refund
        case 'R':
        {
            if (otxn_param(SVAR(dtag), "TAG", 3) != 4)
                NOPE("Monolithic: Misconfigured. Missing TAG otxn parameter.");

            int64_t sig_amt;
            if (otxn_param(SVAR(sig_amt), "AMT", 3) != 8 || sig_amt < 0)
                NOPE("Monolithic: Misconfigured. Missing AMT otxn parameter.");

            uint32_t sig_nce;
            if (otxn_param(SVAR(sig_nce), "SEQ", 3) != 4)
                NOPE("Monolithic: Misconfigured. Missing SEQ otxn parameter.");

            // check the nonce
            uint32_t rfd_seq;
            state(SVAR(rfd_seq), otxn_accid + 12, 20);
            TRACEVAR(rfd_seq);

            if (rfd_seq != sig_nce)
                NOPE("Monolithic: Refund nonce out of sequence.");

            uint8_t dtag_bal[8];
            state(SBUF(dtag_bal), dtag + 28, 4);
            int64_t dtag_bal_xfl = *((int64_t*)dtag_bal);

            // check dtag balance
            if (dtag_bal_xfl <= 0 || !float_compare(dtag_bal_xfl, 0, COMPARE_GREATER))
                NOPE("Monolithic: Insane balance on dtag.");

            if (sig_amt <= 0)
                NOPE("Monolithic: Must provide AMT param when performing refund.");

            if (float_compare(sig_amt, dtag_bal_xfl, COMPARE_GREATER))
                NOPE("Monolithic: Balance not high enough for this debit.");

            int64_t add_dtag_bal_xfl = float_sum(dtag_bal_xfl, sig_amt);
            if (state_set(SVAR(add_dtag_bal_xfl), SVAR(dtag)) != 8)
                NOPE("Monolithic: Insane balance on dtag.");

            int64_t sub_stl_bal_xfl = float_sum(stl_bal_xfl, float_negate(sig_amt));
            if (state_set(SVAR(sub_stl_bal_xfl), hook_accid + 12, 20) != 8)
                NOPE("Monolithic: Insane balance on stl.");

            rfd_seq++;
            if (state_set(SVAR(rfd_seq), otxn_accid + 12, 20) != 4)
                NOPE("Monolithic: Failed to set state.");

            DONE("Monolithic: Refunded.");
            break;
        }

        case 'M':
        {
            switch (op)
            {
                case 'A': // admin (role)
                {
                    uint8_t account[20];
                    if (otxn_param(SBUF(account), "ACC", 3) != 20)
                        NOPE("Monolithic: Misconfigured. Missing ACC modify parameter.");

                    state_set(account, 20, "ADM", 3);
                    DONE("Monolithic: ADM Modified.");
                }

                case 'S': // settler (role)
                {
                    uint8_t account[20];
                    if (otxn_param(SBUF(account), "ACC", 3) != 20)
                        NOPE("Monolithic: Misconfigured. Missing ACC modify parameter.");

                    state_set(account, 20, "STL", 3);
                    DONE("Monolithic: STL Modified.");
                }

                case 'W': // withdraw (role)
                {
                    uint8_t key[33];
                    if (otxn_param(SBUF(key), "WKEY", 4) != 33)
                        NOPE("Monolithic: Misconfigured. Missing KEY modify parameter.");
                    
                    state_set(key, 33, "WKEY", 4);
                    DONE("Monolithic: WKEY Modified.");
                }

                default:
                {
                    NOPE("Monolithic: Unknown operation.");
                }
            }
        }
    
        case 'A':
        {
            switch (op)
            {
                case 'C': // asset (create)
                {
                    uint8_t account[20];
                    if (otxn_param(SBUF(account), "ACC", 3) != 20)
                        NOPE("Monolithic: Misconfigured. Missing ACC modify parameter.");

                    state_set(account, 20, "ADM", 3);
                    DONE("Monolithic: ADM Modified.");
                }

                case 'U': // asset (update)
                {
                    uint8_t account[20];
                    if (otxn_param(SBUF(account), "ACC", 3) != 20)
                        NOPE("Monolithic: Misconfigured. Missing ACC modify parameter.");

                    state_set(account, 20, "ADM", 3);
                    DONE("Monolithic: ADM Modified.");
                }

                case 'D': // asset (destroy)
                {
                    uint8_t account[20];
                    if (otxn_param(SBUF(account), "ACC", 3) != 20)
                        NOPE("Monolithic: Misconfigured. Missing ACC modify parameter.");

                    state_set(account, 20, "STL", 3);
                    DONE("Monolithic: STL Modified.");
                }

                default:
                {
                    NOPE("Monolithic: Unknown operation.");
                }
            }
        }
        
        default:
        {
            NOPE("Monolithic: Unknown operation.");
        }
    }

    return 0;
}