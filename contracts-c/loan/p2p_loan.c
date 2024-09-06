//------------------------------------------------------------------------------
/*

Operations:

// (Loan) ops are:
    // C - Create: create loan - Invoke
    // U - Update: update loan - Invoke
    // D - Delete: delete loan - Invoke


*/
//==============================================================================

#include "hookapi.h"

#define DONE(x)\
    return accept(SBUF(x), __LINE__)

#define NOPE(x)\
    return rollback(SBUF(x), __LINE__)

#define ACCOUNT_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    *(uint64_t*)(buf + 0) = *(uint64_t*)(i +  0);\
    *(uint64_t*)(buf + 8) = *(uint64_t*)(i +  8);\
    *(uint32_t*)(buf + 16) = *(uint32_t*)(i + 16);\
}

#define UINT256_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    *(uint64_t*)(buf + 0) = *(uint64_t*)(i +  0);\
    *(uint64_t*)(buf + 8) = *(uint64_t*)(i +  8);\
    *(uint64_t*)(buf + 16) = *(uint64_t*)(i + 16);\
    *(uint64_t*)(buf + 24) = *(uint64_t*)(i + 24);\
}

#define VL_TO_BUF(buf_raw, uri, len)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    for (int i = 0; GUARD(32), i < 32; ++i) \
        *(((uint64_t*)buf) + i) = *(((uint64_t*)uri) + i); \
    buf[len + 1] += 0xE1U; \
}

#define ENCODE_VL(vl, vl_len)\
    {\
        if (vl_len <= 193) \
        {\
            vl[0] = vl_len;\
        }\
        else if (vl_len <= 12480) \
        {\
            vl_len -= 193;\
            int byte1 = (vl_len >> 8) + 193;\
            int byte2 = vl_len & 0xFFU;\
            vl[0] = byte1;\
            vl[1] = byte2;\
        }\
        else if (vl_len > 12480) \
        {\
            vl_len -= 193;\
            int byte1 = (vl_len >> 8) + 193;\
            int byte2 = vl_len & 0xFFU;\
            vl[0] = byte1;\
            vl[1] = byte2;\
            vl[2] = byte2;\
        }\
    }

// clang-format off
uint8_t rtxn[60000] =
{
/* size,upto */
/*   3,   0 */   0x12U, 0x00U, 0x5FU,                                                           /* tt = Remit       */
/*   5,   3 */   0x22U, 0x80U, 0x00U, 0x00U, 0x00U,                                          /* flags = tfCanonical */
/*   5,   8 */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                                 /* sequence = 0 */
/*   5,  13 */   0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                                /* dtag, flipped */
/*   6,  18 */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                      /* first ledger seq */
/*   6,  24 */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                       /* last ledger seq */
/*   9,  30 */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                         /* fee      */
/*  35,  39 */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       /* pubkey   */
/*  22,  74 */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                  /* srcacc  */
/*  22,  96 */   0x83U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                  /* dstacc  */
/* 116, 118 */   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    /* emit detail */
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   2, 234 */   0xE0U, 0x5CU, 
/*   5, 236 */   0x22U, 0x00U, 0x00U, 0x00U, 0x01U,                                            /* flags = tfBurnable  */
/*  34, 241 */   0x50U, 0x15U,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,            /* digest   */
/*   1, 275 */   0x75U,
/*   1, 276 */   0xE1U,
/*   0, 277 */                
};
// clang-format on

// TX BUILDER
#define BYTES_LEN 277U
#define RFLS_OUT (rtxn + 20U)
#define RLLS_OUT (rtxn + 26U)
#define RDTAG_OUT (rtxn + 14U)
#define RFEE_OUT (rtxn + 31U)
#define RHOOK_ACC (rtxn + 76U)
#define ROTX_ACC (rtxn + 98U)
#define REMIT_OUT (rtxn + 118U)
#define RDIGEST_OUT (rtxn + 243U)
#define RURI_OUT (rtxn + 276)

// clang-format off
#define PREPARE_REMIT_TXN(account_buffer, dest_buffer, uri_buffer, uri_len) do { \ 
    if (otxn_field(RDTAG_OUT, 4, sfSourceTag) == 4) \
        *(RDTAG_OUT - 1) = 0x2EU; \
    uint32_t fls = (uint32_t)ledger_seq() + 1; \ 
    *((uint32_t *)(RFLS_OUT)) = FLIP_ENDIAN(fls); \ 
    uint32_t lls = fls + 4; \ 
    *((uint32_t *)(RLLS_OUT)) = FLIP_ENDIAN(lls); \
    ACCOUNT_TO_BUF(RHOOK_ACC, account_buffer); \ 
    ACCOUNT_TO_BUF(ROTX_ACC, dest_buffer); \ 
    etxn_details(REMIT_OUT, 116U); \ 
    VL_TO_BUF(RURI_OUT, uri_buffer, uri_len); \
    TRACEHEX(rtxn); \
    int64_t fee = etxn_fee_base(rtxn, BYTES_LEN + uri_len + 1); \ 
    uint8_t *b = RFEE_OUT; \ 
    *b++ = 0b01000000 + ((fee >> 56) & 0b00111111); \ 
    *b++ = (fee >> 48) & 0xFFU; \ 
    *b++ = (fee >> 40) & 0xFFU; \ 
    *b++ = (fee >> 32) & 0xFFU; \ 
    *b++ = (fee >> 24) & 0xFFU; \ 
    *b++ = (fee >> 16) & 0xFFU; \ 
    *b++ = (fee >> 8) & 0xFFU; \ 
    *b++ = (fee >> 0) & 0xFFU; \
} while(0) 
// clang-format on

#define LOAN_MODEL 126U
#define LOAN_STATE 0U
#define GRACE_OFFSET 70U // 72 - 75
#define INTERVAL_OFFSET 74U // 76 - 79
#define START_OFFSET 78U // 80 - 83
#define NEXT_OFFSET 82U // 84 - 87
#define LAST_OFFSET 86U // 88 - 91
#define TOTAL_OFFSET 90U // 90 - 91
#define REMAINING_OFFSET 92U // 92 - 93
#define AMOUNT_OFFSET 94U // 96 - 101
#define REQUESTED_OFFSET 102U // 102 - 109
#define ENDING_OFFSET 110U // 110 - 117
#define DRAWABLE_OFFSET 118U // 118 - 126


int64_t hook(uint32_t r)
{
    _g(1,1);

    uint8_t hook_accid[32];
    hook_account(hook_accid + 12, 20);

    uint8_t otxn_accid[32];
    otxn_field(otxn_accid + 12, 20, sfAccount);

    int64_t tt = otxn_type();
    if (tt != ttINVOKE)
        NOPE("p2p_loan.c: Rejecting non-Invoke, non-Payment txn.");

    // Operation
    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1)
        NOPE("p2p_loan.c: Missing OP parameter on Invoke.");

    uint8_t loan_hash[32];
    if (otxn_param(SBUF(loan_hash), "LH", 2) == DOESNT_EXIST)
    {
        NOPE("p2p_loan.c: Missing LH parameter.");
    }

    // TODO: Block U|D if loan is not owned by the p2p loan hook.

    // action
    switch (op)
    {
        case 'C': // create loan
        {
            uint8_t _loan_model[LOAN_MODEL];
            if (state(SBUF(_loan_model), SBUF(loan_hash)) != DOESNT_EXIST)
            {
                NOPE("p2p_loan.c: Loan already exists.");
            }

            uint8_t uril_buffer[8];
            uint8_t uril_key[4] = {'U', 'R', 'I', 'L'};
            if (otxn_param(SBUF(uril_buffer), SBUF(uril_key)) != 8)
            {
                rollback(SBUF("p2p_loan.c: Invalid Txn Parameter `URIL`"), __LINE__);
            }
            uint64_t vl_len = UINT64_FROM_BUF(uril_buffer);

            uint8_t uri_buffer[vl_len + 1];
            uint8_t uri_key[3] = {'U', 'R', 'I'};
            if (otxn_param(uri_buffer + 1, vl_len + 1, SBUF(uri_key)) != vl_len)
            {
                rollback(SBUF("p2p_loan.c: Invalid Txn Parameter `URI`"), __LINE__);
            }

            uint8_t urih_key[4] = {'U', 'R', 'I', 'H'};
            uint8_t urih_buffer[32];
            if (otxn_param(SBUF(urih_buffer), SBUF(urih_key)) != 32)
            {
                rollback(SBUF("p2p_loan.c: Invalid Txn Parameter `URIH`"), __LINE__);
            }
            
            if (otxn_param(SBUF(_loan_model), "LM", 2) == DOESNT_EXIST)
            {
                NOPE("p2p_loan.c: Missing LM parameter.");
            }

            if(state_set(SBUF(_loan_model), SBUF(loan_hash)) < 0)
            {
                NOPE("p2p_loan.c: Could not create Loan.");
            }

            // TXN: PREPARE: Init
            etxn_reserve(1);
            ENCODE_VL(uri_buffer, vl_len);
            UINT256_TO_BUF(RDIGEST_OUT, loan_hash);
            PREPARE_REMIT_TXN(hook_accid + 12, otxn_accid + 12, uri_buffer, vl_len);
            

            // TXN: Emit/Send Txn
            uint8_t remithash[32];
            int64_t remit_result = emit(SBUF(remithash), rtxn, BYTES_LEN + vl_len + 1);
            if (remit_result < 0)
            {
                NOPE("p2p_loan.c: Emit Failure");
            }

            DONE("p2p_loan.c: Created Loan.");
        }

        case 'U': // update loan
        {
            uint8_t _loan_model[LOAN_MODEL];
            if (otxn_param(SBUF(_loan_model), "LM", 2) == DOESNT_EXIST)
            {
                NOPE("p2p_loan.c: Missing LM parameter.");
            }

            if(state_set(SBUF(_loan_model), SBUF(loan_hash)) < 0)
            {
                NOPE("p2p_loan.c: Could not update Loan.");
            }

            DONE("p2p_loan.c: Updated Loan.");
        }

        case 'D': // delete loan
        {
            if(state_set(0, 0, SBUF(loan_hash)) < 0)
            {
                NOPE("p2p_loan.c: Could not delete Loan.");
            }

            // TODO: Burn Token
            DONE("p2p_loan.c: Deleted Loan.");
        }

        default:
        {
            NOPE("p2p_loan.c: Unknown Loan operation.");
        }
    }

    return 0;
}