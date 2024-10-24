#include "hookapi.h"

#define DONE(x)\
    accept(SVAR(x),(uint32_t)__LINE__);

#define NOPE(x)\
{\
    return rollback((x), sizeof(x), __LINE__);\
}

#define ACCOUNT_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    *(uint64_t*)(buf + 0) = *(uint64_t*)(i +  0);\
    *(uint64_t*)(buf + 8) = *(uint64_t*)(i +  8);\
    *(uint32_t*)(buf + 16) = *(uint32_t*)(i + 16);\
}

// #define MIN_LEDGER_LIMIT 50     // 324000 ledger is 15 days. Changed to 50 ledger for testing
// #define MAX_LEDGER_LIMIT 7884000 // 365 days
#define MIN_LEDGER_LIMIT 3 // 12 seconds
#define MAX_LEDGER_LIMIT 5 // 20 seconds
#define ttSET_HOOK 22
uint8_t msg_buf[30] = "You must wait 0000000 ledgers.";
uint8_t rmsg_buf[] = "You must wait 0000000 seconds";

uint8_t ctxn[251] =
{
/* size,upto */
/* 3,    0, tt = ClaimReward      */   0x12U, 0x00U, 0x62U,
/* 5,    3  flags                 */   0x22U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 5,    8, sequence              */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 6,   13, firstledgersequence   */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,
/* 6,   19, lastledgersequence    */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,
/* 9,   25, fee                   */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 35,  34, signingpubkey         */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 22,  69, account               */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 22,  91, issuer                */   0x84U, 0x14U, 0xB5U, 0xF7U, 0x62U, 0x79U, 0x8AU, 0x53U, 0xD5U, 0x43U, 0xA0U, 0x14U, 0xCAU, 0xF8U, 0xB2U, 0x97U, 0xCFU, 0xF8U, 0xF2U, 0xF9U, 0x37U, 0xE8U,
/* 138, 113  emit details          */  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 0,  251                        */ 
};

#define CFLS_OUT (ctxn + 15U) // + leading bytes (2)
#define CLLS_OUT (ctxn + 21U) // + leading bytes (2)
#define CFEE_OUT (ctxn + 26U) // + leading bytes (1)
#define CACCOUNT_OUT (ctxn + 71U) // + leading bytes (2)
#define CEMIT_OUT (ctxn + 113U) // + leading bytes (0)

uint8_t txn[260] =
{
/* size,upto */
/* 3,   0,   tt = Payment           */   0x12U, 0x00U, 0x00U,
/* 5,   3,   flags                  */   0x22U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 5,   8,   sequence               */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 6,   13,  firstledgersequence    */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,
/* 6,   19,  lastledgersequence     */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,
/* 9,   25,  amount                 */   0x61U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
/* 9,   34,  fee                    */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 35,  43,  signingpubkey          */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 22,  78,  account                */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 22,  100, destination            */   0x83U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 138, 122  emit details           */ 
/* 0,   260                         */ 
};

#define FLS_OUT    (txn + 15U) 
#define LLS_OUT    (txn + 21U) 
#define FEE_OUT    (txn + 35U) 
#define AMOUNT_OUT (txn + 26U)
#define ACC_OUT    (txn + 80U) 
#define DEST_OUT   (txn + 102U) 
#define EMIT_OUT   (txn + 122U) 

#define DEFAULT_REWARD_DELAY 6199553087261802496ULL

int64_t cbak(uint32_t reserve)
{
    // uint8_t type;
    // otxn_field(SVAR(type),sfTransactionType);

    // if(type != ttPAYMENT) {
    //     accept(SBUF(reserve), 1);
    // }

    uint32_t prev_release = 0;
    if(state(SVAR(prev_release), "PREV", 4) != 4)
        DONE(reserve);

    meta_slot(1);
    slot_subfield(1, sfTransactionResult, 1);
    int8_t result;
    slot(SVAR(result), 1);
    if(result != 0) {
        state_set(SVAR(prev_release), "LAST", 4);
    }

    state_set(0, 0, "PREV", 4);
    DONE(reserve);
    return 0;
}

int64_t hook(uint32_t reserved)
{
    int64_t tt = otxn_type();
    if (tt != ttINVOKE)
        NOPE("Treasury: HookOn field is incorrectly set.");

    uint32_t current_ledger =  ledger_seq();
    uint32_t fls = current_ledger + 1;
    uint32_t lls = fls + 4;
    etxn_reserve(1);
    uint8_t emithash[32];

    uint64_t amt_param;
    if(hook_param(SVAR(amt_param), "A", 1) != 8)
        NOPE("Treasury: Misconfigured. Amount 'A' not set as Hook parameter.");

    if(float_compare(amt_param, 0, COMPARE_LESS | COMPARE_EQUAL) == 1)
        NOPE("Treasury: Invalid amount.");

    if(float_compare(amt_param, AMOUNT_LIMIT, COMPARE_GREATER | COMPARE_EQUAL) == 1)
        NOPE("Treasury: You don't want to set it to 10M plus XAH!");

    uint32_t ledger_param;
    if(hook_param(SVAR(ledger_param), "L", 1) != 4)
        NOPE("Treasury: Misconfigured. Ledger limit 'L' not set as Hook parameter.");

    if(ledger_param < MIN_LEDGER_LIMIT)
        NOPE("Treasury: Ledger limit must be greater than 324,000(15 days).");

    if(ledger_param > MAX_LEDGER_LIMIT)
        NOPE("Treasury: Ledger limit must be less than 7,884,000(365 days).");

    uint8_t dest_param[20];
    if(hook_param(SBUF(dest_param), "D", 1) != 20)
        NOPE("Treasury: Misconfigured. Destination 'D' not set as Hook parameter.");

    uint8_t keylet[34];
    if (util_keylet(keylet, 34, KEYLET_ACCOUNT, dest_param, 20, 0, 0, 0, 0) != 34)
        NOPE("Treasury: Fetching Keylet Failed.");

    if (slot_set(SBUF(keylet), 1) == DOESNT_EXIST)
        NOPE("Treasury: The Set Destination Account Does Not Exist.");

    uint8_t claim[1];
    if(otxn_param(claim, 1, "C", 1) == 1)
    {
        hook_account(CACCOUNT_OUT, 20);
        *((uint32_t *)(CFLS_OUT)) = FLIP_ENDIAN(fls);
        *((uint32_t *)(CLLS_OUT)) = FLIP_ENDIAN(lls);

        etxn_details(CEMIT_OUT, 138U);
        {
            int64_t fee = etxn_fee_base(SBUF(ctxn));
            uint8_t *b = CFEE_OUT;
            *b++ = 0b01000000 + ((fee >> 56) & 0b00111111);
            *b++ = (fee >> 48) & 0xFFU;
            *b++ = (fee >> 40) & 0xFFU;
            *b++ = (fee >> 32) & 0xFFU;
            *b++ = (fee >> 24) & 0xFFU;
            *b++ = (fee >> 16) & 0xFFU;
            *b++ = (fee >> 8) & 0xFFU;
            *b++ = (fee >> 0) & 0xFFU;
        }

        int64_t xfl_rd = 6215967485771284480LLU;
        state(&xfl_rd, 8, "RD", 2);
        int64_t required_delay = float_int(xfl_rd, 0, 0);
        slot_subfield(1, sfRewardTime, 6);
        int64_t time = slot(0,0,6);
        int64_t time_elapsed = ledger_last_time() - time;
        if (time_elapsed < required_delay)
        {
            time_elapsed = required_delay - time_elapsed;
            rmsg_buf[14] += (time_elapsed / 1000000) % 10;
            rmsg_buf[15] += (time_elapsed /  100000) % 10;
            rmsg_buf[16] += (time_elapsed /   10000) % 10;
            rmsg_buf[17] += (time_elapsed /    1000) % 10;
            rmsg_buf[18] += (time_elapsed /     100) % 10;
            rmsg_buf[19] += (time_elapsed /      10) % 10;
            rmsg_buf[20] += (time_elapsed          ) % 10;
            rollback(SBUF(rmsg_buf), __LINE__);
        }

        if(emit(SBUF(emithash), SBUF(ctxn)) != 32)
            NOPE("Treasury: Failed To Emit.");

        DONE("Treasury: Claimed successfully.");
    }

    uint64_t amount_xfl;
    if(otxn_param(SVAR(amount_xfl), "W", 1) != 8)
        NOPE("Treasury: Specify The Amount To Withdraw.");

    if(float_compare(amount_xfl, amt_param, COMPARE_GREATER) == 1)
        NOPE("Treasury: Outgoing transaction exceeds the amount limit set by you.");

    {
        uint64_t drops = float_int(amount_xfl, 6, 1);
        uint8_t *b = AMOUNT_OUT;
        *b++ = 0b01000000 + ((drops >> 56) & 0b00111111);
        *b++ = (drops >> 48) & 0xFFU;
        *b++ = (drops >> 40) & 0xFFU;
        *b++ = (drops >> 32) & 0xFFU;
        *b++ = (drops >> 24) & 0xFFU;
        *b++ = (drops >> 16) & 0xFFU;
        *b++ = (drops >> 8) & 0xFFU;
        *b++ = (drops >> 0) & 0xFFU;
    }

    hook_account(ACC_OUT, 20);
    ACCOUNT_TO_BUF(DEST_OUT, dest_param);

    uint32_t last_release = 0;
    state(SVAR(last_release), "LAST", 4);

    uint32_t lgr_elapsed = last_release + ledger_param;
    if (lgr_elapsed > current_ledger)
    {
        lgr_elapsed = last_release + ledger_param - current_ledger;
        msg_buf[14] += (lgr_elapsed / 1000000) % 10;
        msg_buf[15] += (lgr_elapsed /  100000) % 10;
        msg_buf[16] += (lgr_elapsed /   10000) % 10;
        msg_buf[17] += (lgr_elapsed /    1000) % 10;
        msg_buf[18] += (lgr_elapsed /     100) % 10;
        msg_buf[19] += (lgr_elapsed /      10) % 10;
        msg_buf[20] += (lgr_elapsed          ) % 10;
        NOPE(msg_buf);
    }

    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);
    etxn_details(EMIT_OUT, 138U);
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

    if(emit(SBUF(emithash), SBUF(txn)) != 32)
        NOPE("Treasury: Failed To Emit.");

    if (state_set(SVAR(current_ledger), "LAST", 4) != 4)
        NOPE("Treasury: Could not update state entry, bailing.");

    if (state_set(SVAR(last_release), "PREV", 4) != 4)
        NOPE("Treasury: Could not update state entry, bailing.");

    DONE("Treasury: Released successfully.");
    _g(1,1);
    return 0;
}