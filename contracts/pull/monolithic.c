//------------------------------------------------------------------------------
/*

HookParameters -> State:

STL: The settler account (role)
CUR: The currency allowed in the funding source
ISS: The issuer account allowed in the funding source
DTAG: The destination tag allowed in the funding source

Operations:

// invoke ops are: (role)
    // B - debit (settler)

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

uint8_t ns[32] = {
    0x21, 0x2C, 0xCE, 0x06, 0x94, 0xF0, 0x63, 0xCC, 
    0x1D, 0xB8, 0xCF, 0xA3, 0x46, 0xE2, 0x96, 0xF3, 
    0xE1, 0xF9, 0xE1, 0xF0, 0xFE, 0x93, 0x12, 0xF6, 
    0x87, 0x58, 0x00, 0xBA, 0x70, 0x57, 0x8F, 0xFE
};

#define DELAY 1U

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
    if (tt != ttINVOKE)
        DONE("Monolithic: passing incoming non Invoke txn");

    otxn_slot(1);

    slot_subfield(1, sfAmount, 2);

    uint8_t amt[48];

    if (slot_size(2) == 8)
        DONE("Monolithic: Passing incoming XAH payment.");

    // get settlement account (role)
    uint8_t _stl[20];
    if (hook_param(SBUF(_stl), "STL", 3) != 20)
        NOPE("Monolithic: Misconfigured. Missing STL install parameter.");

    // get currency
    if (hook_param(OUTCUR, 20, "CUR", 3) != 20)
        NOPE("Monolithic: Misconfigured. Missing CUR install parameter.");

    // get currency issuer
    if (hook_param(OUTISS, 20, "ISS", 3) != 20)
        NOPE("Monolithic: Misconfigured. Missing ISS install parameter.");

    // get dest tag
    uint32_t dtag;
    if (hook_param(&dtag, 4, "DTG", 3) != 4)
        NOPE("Monolithic: Misconfigured. Missing DTG install parameter.");

    // Operation
    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1)
        NOPE("Monolithic: Missing OP parameter on Invoke.");

    // check if the trustline exists
    uint8_t keylet[34];
    util_keylet(keylet, 34, KEYLET_LINE, hook_accid + 12, 20, OUTISS, 20, OUTCUR, 20);
    int64_t already_setup = (slot_set(SBUF(keylet), 10) == 10);
    if (!already_setup)
        NOPE("Monolithic: Trustline not setup.");

    int64_t is_stl = BUFFER_EQUAL_20(otxn_accid + 12, _stl);

    // sanity check
    if (!is_stl && (op == 'D'))
        NOPE("Monolithic: Settler only operation.");

    // current ledger seq is used when emitting a txn
    int64_t seq = ledger_seq() + 1;

    // action
    switch (op)
    {
        // debit
        case 'D':
        {
            int64_t sig_amt;
            if (otxn_param(SVAR(sig_amt), "AMT", 3) != 8 || sig_amt < 0)
                NOPE("Monolithic: Misconfigured. Missing AMT otxn parameter.");

            TRACEVAR(sig_amt);

            ACCOUNT_TO_BUF(HOOKACC, hook_accid + 12);
            
            // check trustline balance
            slot_subfield(10, sfBalance, 11);
            if (slot_size(11) != 48)
                NOPE("Monolithic: Could not fetch trustline balance.");
    
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

            // set the destination addr to the settlement addr
            COPY_20(OUTISS, DESTACC);

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

            DONE("Monolithic: Debited.");
            break;
        }
        
        default:
        {
            NOPE("Monolithic: Unknown operation.");
        }
    }

    return 0;
}