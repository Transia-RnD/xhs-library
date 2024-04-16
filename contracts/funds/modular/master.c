//------------------------------------------------------------------------------
/*

HookParameters -> State:

ADM: The admin account (role)
STL: The settler account (role)
RFD: The refunder account (role)
WKEY: The withdrawer pubkey (role)

Asset States
KEY: <hash>(<20 bytes currency><20 bytes issuer>)
DATA: <settlement address(<20 byte address>)

Operations:

// invoke ops are: (role)
    // I - initalize (any)
    // S - settle (settler)
    // U/P - un/pause (admin)
    // M - modify (admin)
    // A - asset (admin)
    // R - refund (refunder)

// invoke (Modify) sub ops are:
    // A - modify admin account (role)
    // S - modify settler account (role)
    // W - modify withdraw pubkey (role)
    // D - modify withdraw delay

// invoke (Asset) sub ops are:
    // C - create asset (integration) - Set TrustLine ~
    // U - update asset (integration) - Update Settlement Address
    // D - delete asset (integration) - Set Trustline 0

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
        DONE("Master: passing outgoing txn");

    int64_t tt = otxn_type();
    if (tt != ttINVOKE)
        NOPE("Master: Rejecting non-Invoke txn.");

    // get admin account (role)
    uint8_t _admin[20];
    if (hook_param(SBUF(_admin), "ADM", 3) != 20)
        NOPE("Master: Misconfigured. Missing ADM install parameter.");

    // get settlement account (role)
    uint8_t _stl[20];
    if (hook_param(SBUF(_stl), "STL", 3) != 20)
        NOPE("Master: Misconfigured. Missing STL install parameter.");

    // get the withdrawal signing key
    uint8_t _wkey[33];
    if (hook_param(SBUF(_wkey), "WKEY", 4) != 33)
        NOPE("Master: Misconfigured. Missing WKEY install parameter.");

    // get withdraw delay
    int64_t _delay;
    if (hook_param(SVAR(_delay), "DLY", 3) != 4)
        NOPE("Master: Misconfigured. Missing DLY install parameter.");

    // Operation
    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1)
        NOPE("Master: Missing OP parameter on Invoke.");
    
    // Sub Operation
    uint8_t sop;
    if ((op == 'M' || op == 'A') && otxn_param(&sop, 1, "SOP", 3) != 1)
        NOPE("Master: Missing SOP parameter on Invoke.");

    int64_t xfl_in;
    uint32_t flags;

    // enforced pausedness
    if (op != 'U')
    {
        uint8_t paused;
        state(&paused, 1, "P", 1);
        if (paused)
            NOPE("Master: Paused.");
    }

    // // enforce initalisation
    // if (!already_setup && op != 'I')
    //     NOPE("Master: Send op=I initalisation first.");
    
    uint8_t admin[20];
    uint8_t stl[20];
    uint8_t wkey[33];
    uint8_t acc[20];
    int64_t delay;
    if (state(SBUF(admin), "ADM", 3) == DOESNT_EXIST)
    {
        *admin = _admin;
        state_set(_admin, 20, "ADM", 3);
        *stl = _stl;
        state_set(_stl, 20, "STL", 3);
        *wkey = _wkey;
        state_set(_wkey, 33, "WKEY", 4);
        delay = _delay;
        state_set(SVAR(_delay), "DLY", 3);
    }
    else
    {
        state(SBUF(admin), "ADM", 3);
        state(SBUF(stl), "STL", 3);
        state(SBUF(wkey), "WKEY", 4);
        state(SBUF(acc), "ACC", 3);
        state(SVAR(delay), "DLY", 3);
    }

    int64_t is_admin = BUFFER_EQUAL_20(otxn_accid + 12, admin);
    int64_t is_stl = BUFFER_EQUAL_20(otxn_accid + 12, stl);

    // permission check
    if (!is_admin && (op == 'U' || op == 'P' || op == 'M'))
        NOPE("Master: Admin only operation.");
    
    if (!is_stl && (op == 'S' || op == 'R'))
        NOPE("Master: Settler only operation.");

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

                    uint8_t account[20];
                    if (otxn_param(SBUF(account), "ACC", 3) != 20)
                        NOPE("Master: Misconfigured. Missing ACC asset parameter.");

                    uint8_t amt[48];
                    if (otxn_param(SBUF(amt), "AMT", 3) != 48)
                        NOPE("Master: Misconfigured. Missing AMT asset parameter.");

                    uint8_t hash[32];
                    if (otxn_param(SBUF(hash), "AHS", 3) != 32)
                        NOPE("Master: Misconfigured. Missing AHS asset parameter.");

                    // check if the trustline exists
                    uint8_t keylet[34];
                    util_keylet(keylet, 34, KEYLET_LINE, hook_accid + 12, 20, amt + 28, 20, amt + 8, 20);
                    int64_t already_setup = (slot_set(SBUF(keylet), 10) == 10);
                    TRACEVAR(already_setup);
                    if (already_setup)
                        DONE("Master: Already setup trustline.");

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
                    
                    TRACEHEX(hash);
                    state_set(account, 20, SBUF(hash));
                    DONE("Master: Created Asset Integration.");
                }

                case 'U': // create asset (integration)
                {
                    uint8_t account[20];
                    if (otxn_param(SBUF(account), "ACC", 3) != 20)
                        NOPE("Master: Misconfigured. Missing ACC modify parameter.");
                    
                    uint8_t hash[32];
                    if (otxn_param(SBUF(hash), "AHS", 3) != 32)
                        NOPE("Master: Misconfigured. Missing AHS asset parameter.");

                    state_set(account, 20, SBUF(hash));
                    DONE("Master: Updated Asset Integration.");
                }

                case 'D': // delete asset (integration)
                {
                    ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);
                    
                    uint8_t amt[48];
                    if (otxn_param(SBUF(amt), "AMT", 3) != 48)
                        NOPE("Master: Misconfigured. Missing AMT asset parameter.");

                    uint8_t hash[32];
                    if (otxn_param(SBUF(hash), "AHS", 3) != 32)
                        NOPE("Master: Misconfigured. Missing AHS asset parameter.");

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
                    
                    TRACEHEX(hash);
                    state_set(0, 0, SBUF(hash));
                    DONE("Master: Deleted Asset Integration.");
                }

                default:
                {
                    NOPE("Master: Unknown operation.");
                }
            }
        }

        case 'U':
        case 'P':
        {
            // pause
            uint8_t paused = (op == 'P' ? 1 : 0);
            state_set(&paused, 1, "P", 1);
            DONE("Master: Paused/Unpaused.");
        }

        // settlement
        case 'S':
        {
            ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);

            uint8_t amt[48];
            if (otxn_param(SBUF(amt), "AMT", 3) != 48)
                NOPE("Master: Misconfigured. Missing AMT otxn parameter.");
            int64_t sig_amt = *((int64_t *)amt);
            
            uint8_t hash[32];
            if (otxn_param(SBUF(hash), "AHS", 3) != 32)
                NOPE("Master: Misconfigured. Missing AHS otxn parameter.");

            uint32_t sig_nce;
            if (otxn_param(SVAR(sig_nce), "SEQ", 3) != 4)
                NOPE("Master: Misconfigured. Missing SEQ otxn parameter.");

            // check if the trustline exists
            uint8_t keylet[34];
            util_keylet(keylet, 34, KEYLET_LINE, hook_accid + 12, 20, amt + 28, 20, amt + 8, 20);
            if (slot_set(SBUF(keylet), 10) != 10)
                DONE("Master: Invalid trustline.");
            
            // check trustline balance
            slot_subfield(10, sfBalance, 11);
            if (slot_size(11) != 48)
                NOPE("Master: Could not fetch trustline balance.");
    
            uint8_t low_limit[48];
            if (slot_subfield(10, sfLowLimit, 13) != 13)
                NOPE("Master: Could not slot subfield `sfLowLimit`");

            if (slot(SVAR(low_limit), 13) != 48)
                NOPE("Master: Could not slot `sfLowLimit`");

            int64_t xfl_bal = slot_float(11);

            // balance is negative and issuer is not low
            if (float_sign(xfl_bal) && !BUFFER_EQUAL_20(amt + 28, low_limit + 28))
            {
                NOPE("Master: Insane balance on trustline.");
            }

            xfl_bal = float_sign(xfl_bal) ? float_negate(xfl_bal) : xfl_bal;

            if (xfl_bal <= 0 || !float_compare(xfl_bal, 0, COMPARE_GREATER))
                NOPE("Master: Insane balance on trustline.");

            // set the destination addr to the settlement addr
            uint8_t stl_account[20];
            state_set(SBUF(stl_account), SBUF(hash));
            COPY_20(stl_account, DESTACC);

            // check the nonce
            uint32_t stl_seq;
            state(SVAR(stl_seq), otxn_accid + 12, 20);

            if (stl_seq != sig_nce)
                NOPE("Master: Settlement nonce out of sequence.");

            if (sig_amt <= 0)
                NOPE("Master: Settlement amount must be greater than 0.");

            TRACEVAR(sig_amt);
            TRACEVAR(xfl_bal);
            if (float_compare(sig_amt, xfl_bal, COMPARE_GREATER))
                NOPE("Master: Balance not high enough for this settlement.");

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
                NOPE("Master: Settle Emitted Failure.");
            }

            stl_seq++;
            if (state_set(SVAR(stl_seq), otxn_accid + 12, 20) != 4)
                NOPE("Master: Failed to set state.");

            DONE("Master: Emitted settlement.");
            break;
        }

        // refund
        case 'R':
        {

            ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);

            uint8_t amt[48];
            if (otxn_param(SBUF(amt), "AMT", 3) != 48)
                NOPE("Master: Misconfigured. Missing AMT otxn parameter.");
            int64_t sig_amt = *((int64_t *)amt);

            uint32_t sig_nce;
            if (otxn_param(SVAR(sig_nce), "SEQ", 3) != 4)
                NOPE("Master: Misconfigured. Missing SEQ otxn parameter.");
            
            uint8_t acct_buff[20];
            if (otxn_param(SVAR(acct_buff), "ACC", 3) != 20)
                NOPE("Master: Misconfigured. Missing ACC otxn parameter.");

            COPY_20(acct_buff, DESTACC);

            // check the nonce
            uint32_t rfd_seq;
            state(SVAR(rfd_seq), otxn_accid + 12, 20);
            TRACEVAR(rfd_seq);

            if (rfd_seq != sig_nce)
                NOPE("Master: Refund nonce out of sequence.");

            // check if the trustline exists
            uint8_t keylet[34];
            util_keylet(keylet, 34, KEYLET_LINE, hook_accid + 12, 20, amt + 28, 20, amt + 8, 20);
            if (slot_set(SBUF(keylet), 10) != 10)
                DONE("User: Invalid trustline.");

            // check trustline balance
            slot_subfield(10, sfBalance, 11);
            if (slot_size(11) != 48)
                NOPE("Master: Could not fetch trustline balance.");
    
            uint8_t low_limit[48];
            if (slot_subfield(10, sfLowLimit, 13) != 13)
                NOPE("Master: Could not slot subfield `sfLowLimit`");

            if (slot(SVAR(low_limit), 13) != 48)
                NOPE("Master: Could not slot `sfLowLimit`");

            int64_t xfl_bal = slot_float(11);

            // balance is negative and issuer is not low
            if (float_sign(xfl_bal) && !BUFFER_EQUAL_20(amt + 28, low_limit + 28))
            {
                NOPE("Master: Insane balance on trustline.");
            }

            xfl_bal = float_sign(xfl_bal) ? float_negate(xfl_bal) : xfl_bal;

            // check dtag balance
            if (xfl_bal <= 0 || !float_compare(xfl_bal, 0, COMPARE_GREATER))
                NOPE("Master: Insane balance on balance.");

            if (sig_amt <= 0)
                NOPE("Master: Must provide AMT param when performing refund.");

            if (float_compare(sig_amt, xfl_bal, COMPARE_GREATER))
                NOPE("Master: Balance not high enough for this debit.");

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
                NOPE("Master: Refund Emitted Failure.");
            }

            rfd_seq++;
            if (state_set(SVAR(rfd_seq), otxn_accid + 12, 20) != 4)
                NOPE("Master: Failed to set state.");

            DONE("Master: Refunded.");
            break;
        }

        case 'M':
        {
            switch (sop)
            {
                case 'A': // admin (role)
                {
                    uint8_t account[20];
                    if (otxn_param(SBUF(account), "ACC", 3) != 20)
                        NOPE("Master: Misconfigured. Missing ACC modify parameter.");

                    state_set(account, 20, "ADM", 3);
                    DONE("Master: Admin Modified.");
                }

                case 'S': // settler (role)
                {
                    uint8_t account[20];
                    if (otxn_param(SBUF(account), "ACC", 3) != 20)
                        NOPE("Master: Misconfigured. Missing ACC modify parameter.");

                    state_set(account, 20, "STL", 3);
                    DONE("Master: Settlement Verifier Modified.");
                }

                case 'W': // withdraw (role)
                {
                    uint8_t key[33];
                    if (otxn_param(SBUF(key), "KEY", 3) != 33)
                        NOPE("Master: Misconfigured. Missing KEY modify parameter.");
                    
                    state_set(key, 33, "WKEY", 4);
                    DONE("Master: Withdraw Verifier Modified.");
                }
                
                case 'D': // delay (time)
                {
                    int64_t delay;
                    if (otxn_param(SVAR(delay), "DLY", 3) != 4)
                        NOPE("Master: Misconfigured. Missing DLY modify parameter.");
                    
                    state_set(SVAR(delay), "DLY", 3);
                    DONE("Master: Withdraw Delay Modified.");
                }

                default:
                {
                    NOPE("Master: Unknown operation.");
                }
            }
        }
        
        default:
        {
            NOPE("Master: Unknown operation.");
        }
    }

    return 0;
}