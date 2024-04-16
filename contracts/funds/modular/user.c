//------------------------------------------------------------------------------
/*

Foreign State (Read):

ADM: The admin account (role)
STL: The settler account (role)
RFD: The refunder account (role)
WKEY: The withdrawer pubkey (role)

Asset States
KEY: <hash>(<20 bytes currency><20 bytes issuer>)
DATA: <settlement address(<20 byte address>)

Operations:

// payment ops are: 
    // D - deposit (any)

// invoke ops are:
    // B - debit (settler)
    // W - withdraw (user)

// invoke (Asset) ops are:
    // C - create asset (integration) - Set TrustLine ~
    // D - delete asset (integration) - Set Trustline 0

// invoke (Withdraw) ops are:
    // A - Approval (user)
    // I - Intent (user)
    // E - Execution (user)
    // C - Cancel (user)

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

uint8_t master_accid[20] = {
    0x6D, 0xF9, 0x0E, 0xCA, 0x2B, 0x5C, 0x0B, 0x45, 0xAA, 0x2E, 
    0x1D, 0x6E, 0x99, 0x3B, 0xF7, 0xD9, 0xDC, 0xB2, 0x26, 0xB3
};

uint8_t ns[32] = {
    0x1D, 0xAE, 0x33, 0x31, 0xAD, 0x54, 0x4F, 0x4F,
    0xB6, 0x1F, 0x34, 0x75, 0x85, 0xDC, 0x39, 0x75,
    0xE5, 0xE3, 0xC1, 0x19, 0xFA, 0x6A, 0xA5, 0xED,
    0x4E, 0x6D, 0x9D, 0x58, 0x0C, 0x77, 0xB2, 0x25
};

uint8_t nonce_ns[32] = {
    0x21, 0x2C, 0xCE, 0x06, 0x94, 0xF0, 0x63, 0xCC, 
    0x1D, 0xB8, 0xCF, 0xA3, 0x46, 0xE2, 0x96, 0xF3, 
    0xE1, 0xF9, 0xE1, 0xF0, 0xFE, 0x93, 0x12, 0xF6, 
    0x87, 0x58, 0x00, 0xBA, 0x70, 0x57, 0x8F, 0xFE
};

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
        DONE("User: passing outgoing txn");

    int64_t tt = otxn_type();
    if (tt != ttINVOKE && tt != ttPAYMENT)
        NOPE("User: Rejecting non-Invoke, non-Payment txn.");

    otxn_slot(1);

    slot_subfield(1, sfAmount, 2);

    uint8_t amt[48];

    if (slot_size(2) == 8)
        DONE("User: Passing incoming XAH payment.");

    // get admin account (role)
    uint8_t admin[20];
    if (state_foreign(SBUF(admin), "ADM", 3, SBUF(ns), SBUF(master_accid)) != 20)
        NOPE("User: Misconfigured. Missing ADM state.");

    // get settlement account (role)
    uint8_t stl[20];
    if (state_foreign(SBUF(stl), "STL", 3, SBUF(ns), SBUF(master_accid)) != 20)
        NOPE("User: Misconfigured. Missing STL state.");

    // get the withdrawal signing key
    uint8_t wkey[33];
    if (state_foreign(SBUF(wkey), "WKEY", 4, SBUF(ns), SBUF(master_accid)) != 33)
        NOPE("User: Misconfigured. Missing WKEY state.");

    // get withdraw delay
    // int64_t delay;
    // if (state_foreign(SVAR(delay), "DLY", 3, SBUF(ns), SBUF(master_accid)) != 4)
    //     NOPE("User: Misconfigured. Missing DLY state.");

    // Operation
    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1)
        NOPE("User: Missing OP parameter on Invoke.");

    // Sub Operation
    uint8_t sop;
    if ((op == 'W' || op == 'A') && otxn_param(&sop, 1, "SOP", 3) != 1)
        NOPE("User: Missing SOP parameter on Invoke.");

    int64_t xfl_in;
    uint32_t flags;

    if (tt == ttPAYMENT)
    {
        // this will fail if flags isn't in the txn, that's also ok.
        otxn_field(&flags, 4, sfFlags);
        
        // check for partial payments (0x00020000) -> (0x00000200 LE)
        if (flags & 0x200U)
            NOPE("User: Partial payments are not supported.");

        otxn_field(SBUF(amt), sfAmount);

        xfl_in = slot_float(2);

        if (xfl_in < 0 || !float_compare(xfl_in, 0, COMPARE_GREATER))
            NOPE("User: Invalid sfAmount.");
    }

    // enforced pausedness
    uint8_t paused;
    state_foreign(&paused, 1, "P", 1, SBUF(ns), SBUF(master_accid));
    if (paused)
        NOPE("User: Paused.");

    int64_t is_stl = BUFFER_EQUAL_20(otxn_accid + 12, stl);

    // sanity check
    if ((op == 'D') && tt != ttPAYMENT)
        NOPE("User: Deposit operations must be a payment transaction.");

    // permission check
    if (!is_stl && (op == 'B'))
        NOPE("User: Settler only operation.");

    // current ledger seq is used when emitting a txn
    int64_t seq = ledger_seq() + 1;

    // action
    switch (op)
    {
        case 'A':
        {
            switch (sop)
            {
                case 'C': // create asset (integration)
                {
                    ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);

                    uint8_t amt[48];
                    if (otxn_param(SBUF(amt), "AMT", 3) != 48)
                        NOPE("User: Misconfigured. Missing AMT asset parameter.");

                    uint8_t hash[32];
                    if (otxn_param(SBUF(hash), "AHS", 3) != 32)
                        NOPE("User: Misconfigured. Missing AHS asset parameter.");

                    uint8_t dump[20];
                    TRACEHEX(hash);
                    if (state_foreign(SBUF(dump), SBUF(hash), SBUF(ns), SBUF(master_accid)) == DOESNT_EXIST)
                    {
                        NOPE("User: Asset has not been integrated with Master Contract.");
                    }

                    // check if the trustline exists
                    uint8_t keylet[34];
                    util_keylet(keylet, 34, KEYLET_LINE, hook_accid + 12, 20, amt + 28, 20, amt + 8, 20);
                    int64_t already_setup = (slot_set(SBUF(keylet), 10) == 10);
                    TRACEVAR(already_setup);
                    if (already_setup)
                        DONE("User: Already setup trustline.");

                    int64_t xfl_out = *((int64_t *)amt);

                    // write limit amount
                    float_sto(OUTAMT_TL, 49, amt + 8, 20, amt + 28, 20, xfl_out, sfLimitAmount);
                            
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

                    DONE("User: Created Asset Integration.");
                }

                case 'D': // delete asset (integration)
                {
                    ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);

                    uint8_t amt[48];
                    if (otxn_param(SBUF(amt), "AMT", 3) != 48)
                        NOPE("User: Misconfigured. Missing AMT asset parameter.");

                    uint8_t hash[32];
                    if (otxn_param(SBUF(hash), "AHS", 3) != 32)
                        NOPE("User: Misconfigured. Missing AHS asset parameter.");

                    int64_t xfl_out = *((int64_t *)amt);

                    // write limit amount
                    float_sto(OUTAMT_TL, 49, amt + 8, 20, amt + 28, 20, xfl_out, sfLimitAmount);
                            
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

                    DONE("User: Deleted Asset Integration.");
                }

                default:
                {
                    NOPE("User: Unknown operation.");
                }
            }
        }

        case 'D':
        {
            DONE("User: Deposited.");
        }

        // debit
        case 'B':
        {
            uint8_t amt[48];
            if (otxn_param(SBUF(amt), "AMT", 3) != 48)
                NOPE("User: Misconfigured. Missing AMT otxn parameter.");
            int64_t sig_amt = *((int64_t *)amt);

            uint32_t sig_nce;
            if (otxn_param(SVAR(sig_nce), "SEQ", 3) != 4)
                NOPE("User: Misconfigured. Missing SEQ otxn parameter.");

            // check the nonce
            uint32_t dbt_seq;
            state_foreign(SVAR(dbt_seq), otxn_accid + 12, 20, SBUF(nonce_ns), hook_accid + 12, 20);

            if (dbt_seq != sig_nce)
                NOPE("User: Debit nonce out of sequence.");

            // check if the trustline exists
            uint8_t keylet[34];
            util_keylet(keylet, 34, KEYLET_LINE, hook_accid + 12, 20, amt + 28, 20, amt + 8, 20);
            if (slot_set(SBUF(keylet), 10) != 10)
                DONE("User: Invalid trustline.");

            // check trustline balance
            slot_subfield(10, sfBalance, 11);
            if (slot_size(11) != 48)
                NOPE("User: Could not fetch trustline balance.");
    
            uint8_t low_limit[48];
            if (slot_subfield(10, sfLowLimit, 13) != 13)
                NOPE("User: Could not slot subfield `sfLowLimit`");

            if (slot(SVAR(low_limit), 13) != 48)
                NOPE("User: Could not slot `sfLowLimit`");

            int64_t xfl_bal = slot_float(11);

            // balance is negative and issuer is not low
            if (float_sign(xfl_bal) && !BUFFER_EQUAL_20(amt + 28, low_limit + 28))
            {
                NOPE("User: Insane balance on trustline.");
            }

            xfl_bal = float_sign(xfl_bal) ? float_negate(xfl_bal) : xfl_bal;

            if (xfl_bal <= 0 || !float_compare(xfl_bal, 0, COMPARE_GREATER))
                NOPE("User: Insane balance on trustline.");

            int64_t xfl_out;

            ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);

            // set the destination addr to the settlement addr
            COPY_20(stl, DESTACC); 

            if (sig_amt <= 0)
                NOPE("User: Must provide AMT param when performing settlement.");

            if (float_compare(sig_amt, xfl_bal, COMPARE_GREATER))
                NOPE("User: Balance not high enough for this settlement.");

            // write payment amount
            float_sto(OUTAMT, 49, amt + 8, 20, amt + 28, 20, sig_amt, sfAmount);

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
                NOPE("User: Debit Emitted Failure.");
            }

            dbt_seq++;
            if (state_foreign_set(SVAR(dbt_seq), otxn_accid + 12, 20, SBUF(nonce_ns), hook_accid + 12, 20) != 4)
                NOPE("User: Failed to set state.");

            DONE("User: Emitted debit.");
            break;
        }
        
        // withdrawal
        case 'W':
        {
            switch (sop)
            {
                case 'A': // approval (signature)
                {
                    // get signature if any
                    // Signature format is packed binary data of the form:
                    // <20 byte dest accid><8 byte le xfl amount><4 byte le int expiry timestamp><4 byte le int nonce><signature>
                    uint8_t sig_buf[256];
                    int64_t sig_len = otxn_param(SBUF(sig_buf), "SIG", 3);

                    // place pointers according to packed data
                    uint8_t* sig_acc = sig_buf;
                    uint64_t sig_amt = *((uint64_t*)(sig_buf + 20));
                    COPY_20(sig_buf + 28, OUTCUR);
                    COPY_20(sig_buf + 48, OUTISS);
                    // uint64_t sig_cur = *((uint64_t*)(sig_buf + 28));
                    // uint64_t sig_iss = *((uint64_t*)(sig_buf + 48));
                    uint32_t sig_exp = *((uint32_t*)(sig_buf + 68));
                    uint32_t sig_nce = *((uint32_t*)(sig_buf + 72));
                    uint8_t* sig = sig_buf + 76;

                    if (sig_len > 0)
                    {
                        if (sig_len < 120)
                            NOPE("User: Signature too short.");

                        if (!util_verify(sig_buf, 76, sig_buf + 76, sig_len - 76, SBUF(wkey)))
                            NOPE("User: Signature verification failed.");
                    }

                    if (sig_len <= 0)
                        NOPE("User: Missing SIG parameter.");

                    int64_t time = ledger_last_time();
                    if (time > sig_exp)
                        NOPE("User: Ticket has expired.");

                    if (!BUFFER_EQUAL_20(sig_acc, otxn_accid + 12))
                        NOPE("User: Wrong account for ticket.");

                    // check if the trustline exists
                    uint8_t keylet[34];
                    util_keylet(keylet, 34, KEYLET_LINE, hook_accid + 12, 20, OUTISS, 20, OUTCUR, 20);
                    if (slot_set(SBUF(keylet), 10) != 10)
                        DONE("User: Invalid trustline.");
                    
                    // check trustline balance
                    slot_subfield(10, sfBalance, 11);
                    if (slot_size(11) != 48)
                        NOPE("User: Could not fetch trustline balance.");
            
                    uint8_t low_limit[48];
                    if (slot_subfield(10, sfLowLimit, 13) != 13)
                        NOPE("User: Could not slot subfield `sfLowLimit`");

                    if (slot(SVAR(low_limit), 13) != 48)
                        NOPE("User: Could not slot `sfLowLimit`");

                    int64_t xfl_bal = slot_float(11);

                    // balance is negative and issuer is not low
                    if (float_sign(xfl_bal) && !BUFFER_EQUAL_20(OUTISS, low_limit + 28))
                    {
                        NOPE("User: Insane balance on trustline.");
                    }

                    xfl_bal = float_sign(xfl_bal) ? float_negate(xfl_bal) : xfl_bal;

                    if (xfl_bal <= 0 || !float_compare(xfl_bal, 0, COMPARE_GREATER))
                        NOPE("User: Insane balance on trustline.");

                    // check the nonce
                    uint32_t wth_seq;
                    state_foreign(SVAR(wth_seq), otxn_accid + 12, 20, SBUF(nonce_ns), hook_accid + 12, 20);
                    TRACEVAR(wth_seq);

                    if (wth_seq != sig_nce)
                        NOPE("User: Nonce out of sequence.");

                    // check bal can support withdraw
                    TRACEVAR(xfl_bal);
                    TRACEVAR(sig_amt);
                    if (float_compare(sig_amt, xfl_bal, COMPARE_GREATER))
                        NOPE("User: Balance not high enough for this withdrawal.");

                    ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);
                    COPY_20(sig_acc, DESTACC);

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
                        NOPE("User: Withdrawal Emitted Failure.");
                    }

                    // update nonce
                    wth_seq++;
                    if (state_foreign_set(SVAR(wth_seq), otxn_accid + 12, 20, SBUF(nonce_ns), hook_accid + 12, 20) != 4)
                        NOPE("User: Failed to set state.");

                    DONE("User: Emitted approval withdrawal.");
                }

                case 'I': // permissionless (intent)
                {
                    uint8_t amt[56];
                    if (otxn_param(amt, 48, "AMT", 3) != 48)
                        NOPE("User: Misconfigured. Missing AMT otxn parameter.");
                    int64_t sig_amt = *((int64_t *)amt);

                    uint32_t sig_nce;
                    if (otxn_param(SVAR(sig_nce), "SEQ", 3) != 4)
                        NOPE("User: Misconfigured. Missing SEQ otxn parameter.");
                    
                    // check if the trustline exists
                    uint8_t keylet[34];
                    util_keylet(keylet, 34, KEYLET_LINE, hook_accid + 12, 20, amt + 28, 20, amt + 8, 20);
                    if (slot_set(SBUF(keylet), 10) != 10)
                        DONE("User: Invalid trustline.");

                    // check trustline balance
                    slot_subfield(10, sfBalance, 11);
                    if (slot_size(11) != 48)
                        NOPE("User: Could not fetch trustline balance.");
            
                    uint8_t low_limit[48];
                    if (slot_subfield(10, sfLowLimit, 13) != 13)
                        NOPE("User: Could not slot subfield `sfLowLimit`");

                    if (slot(SVAR(low_limit), 13) != 48)
                        NOPE("User: Could not slot `sfLowLimit`");

                    int64_t xfl_bal = slot_float(11);

                    // balance is negative and issuer is not low
                    if (float_sign(xfl_bal) && !BUFFER_EQUAL_20(OUTISS, low_limit + 28))
                    {
                        NOPE("User: Insane balance on trustline.");
                    }

                    xfl_bal = float_sign(xfl_bal) ? float_negate(xfl_bal) : xfl_bal;

                    if (xfl_bal <= 0 || !float_compare(xfl_bal, 0, COMPARE_GREATER))
                        NOPE("User: Insane balance on trustline.");

                    // check the nonce
                    uint32_t wth_seq;
                    state_foreign(SVAR(wth_seq), otxn_accid + 12, 20, SBUF(nonce_ns), hook_accid + 12, 20);
                    TRACEVAR(wth_seq);

                    if (wth_seq != sig_nce)
                        NOPE("User: Nonce out of sequence.");

                    // check bal can support withdraw
                    if (float_compare(sig_amt, xfl_bal, COMPARE_GREATER))
                        NOPE("User: Balance not high enough for this withdrawal.");
                    
                    int64_t time = ledger_last_time() + 1;
                    uint8_t pending_buff[24];
                    COPY_20(otxn_accid + 12, pending_buff);
                    UINT32_TO_BUF(pending_buff + 20U, sig_nce);

                    TRACEHEX(pending_buff);

                    INT64_TO_BUF(amt + 48U, time);
                    if (state_set(SBUF(amt), SBUF(pending_buff)) != 56)
                        NOPE("User: Could not save pending withdrawal intent.");

                    // update nonce
                    wth_seq++;
                    if (state_foreign_set(SVAR(wth_seq), otxn_accid + 12, 20, SBUF(nonce_ns), hook_accid + 12, 20) != 4)
                        NOPE("User: Failed to set state.");

                    DONE("User: Created permissionless withdraw intent.");
                }

                case 'E': // permissionless (execution)
                {
                    uint8_t pending_buff[24];
                    if (otxn_param(SBUF(pending_buff), "WI", 2) != 24)
                        NOPE("User: Misconfigured. Missing WI otxn parameter.");

                    TRACEHEX(pending_buff);

                    uint8_t amount_buff[56];
                    if (state(SBUF(amount_buff), SBUF(pending_buff)) == DOESNT_EXIST)
                        NOPE("User: Misconfigured. Withdraw Intent does not exist.");
                    
                    int64_t delay_time = UINT64_FROM_BUF(amount_buff + 48);
                    int64_t time = ledger_last_time();
                    if (time < delay_time)
                        NOPE("User: Need to wait..");
                    
                    uint64_t sig_amt = *((uint64_t*)(amount_buff + 0U));

                    TRACEVAR(sig_amt);

                    ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);
                    COPY_20(pending_buff, DESTACC);

                    // write payment amount
                    float_sto(OUTAMT, 49, amount_buff + 8, 20, amount_buff + 28, 20, sig_amt, sfAmount);

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
                        NOPE("User: Withdrawal Emitted Failure.");
                    }

                    state_set(0, 0, SBUF(pending_buff));

                    DONE("User: Emitted permissionless withdrawal.");
                }

                default:
                {
                    NOPE("User: Unknown operation.");
                }
            }
        }
        
        default:
        {
            NOPE("User: Unknown operation.");
        }
    }

    return 0;
}