#include "hookapi.h"

#define DONE(x) accept(SBUF(x), __LINE__)
#define NOPE(x) rollback(SBUF(x), __LINE__)

#define ACCOUNT_TO_BUF(buf_raw, i)                       \
    {                                                    \
        unsigned char *buf = (unsigned char *)buf_raw;   \
        *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0);   \
        *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8);   \
        *(uint32_t *)(buf + 16) = *(uint32_t *)(i + 16); \
    }

#define AMOUNT_LIMIT 6215967485771284480LLU // 10M XAH
#define MIN_LEDGER_LIMIT 50     // 324000 ledger is 15 days. Changed to 50 ledger for testing
#define MAX_LEDGER_LIMIT 7884000 // 365 days

#define DEFAULT_REWARD_DELAY 6199553087261802496ULL // 2600000
#define DEFAULT_REWARD_RATE 6038156834009797973ULL  // 0.00333333333f
int8_t genesis_acc[20] = {
    0xB5U, 0xF7U, 0x62U, 0x79U, 0x8AU, 0x53U, 0xD5U,
    0x43U, 0xA0U, 0x14U, 0xCAU, 0xF8U, 0xB2U, 0x97U,
    0xCFU, 0xF8U, 0xF2U, 0xF9U, 0x37U, 0xE8U};
int8_t reward_ns[32] = {0x00U};

uint8_t rmsg_buf[30] = "You must wait 0000000 seconds.";
uint8_t lmsg_buf[30] = "You must wait 0000000 ledgers.";

// clang-format off
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
// clang-format on

#define CFLS_OUT (ctxn + 15U)
#define CLLS_OUT (ctxn + 21U)
#define CFEE_OUT (ctxn + 26U)
#define CACCOUNT_OUT (ctxn + 71U)
#define CEMIT_OUT (ctxn + 113U)

// clang-format off
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
// clang-format on

#define FLS_OUT (txn + 15U)
#define LLS_OUT (txn + 21U)
#define FEE_OUT (txn + 35U)
#define AMOUNT_OUT (txn + 26U)
#define ACC_OUT (txn + 80U)
#define DEST_OUT (txn + 102U)
#define EMIT_OUT (txn + 122U)

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

int64_t cbak(uint32_t reserve)
{
    uint32_t prev_release = 0;
    if (state(SVAR(prev_release), "PREV", 4) != 4)
        DONE("Success");

    meta_slot(1);
    slot_subfield(1, sfTransactionResult, 1);
    int8_t result;
    slot(SVAR(result), 1);
    if (result != 0)
        state_set(SVAR(prev_release), "LAST", 4);

    state_set(0, 0, "PREV", 4);
    DONE("Success");
    return 0;
}

int64_t hook(uint32_t reserved)
{
    int64_t tt = otxn_type();
    if (tt != ttINVOKE)
        NOPE("Treasury: HookOn field is incorrectly set.");

    uint32_t current_ledger = ledger_seq();
    uint32_t fls = current_ledger + 1;
    uint32_t lls = fls + 4;
    etxn_reserve(1);
    uint8_t emithash[32];

    uint64_t amt_param;
    if (hook_param(SVAR(amt_param), "A", 1) != 8)
        NOPE("Treasury: Misconfigured. Amount 'A' not set as Hook parameter.");

    if (float_compare(amt_param, 0, COMPARE_LESS | COMPARE_EQUAL) == 1)
        NOPE("Treasury: Invalid amount.");

    if (float_compare(amt_param, AMOUNT_LIMIT, COMPARE_GREATER | COMPARE_EQUAL) == 1)
        NOPE("Treasury: You don't want to set it to 10M plus XAH!");

    uint32_t ledger_param;
    if (hook_param(SVAR(ledger_param), "L", 1) != 4)
        NOPE("Treasury: Misconfigured. Ledger limit 'L' not set as Hook parameter.");

    if (ledger_param < MIN_LEDGER_LIMIT)
        NOPE("Treasury: Ledger limit must be greater than 324,000(15 days).");

    if (ledger_param > MAX_LEDGER_LIMIT)
        NOPE("Treasury: Ledger limit must be less than 7,884,000(365 days).");

    uint8_t dest_param[20];
    if (hook_param(SBUF(dest_param), "D", 1) != 20)
        NOPE("Treasury: Misconfigured. Destination 'D' not set as Hook parameter.");

    uint8_t keylet[34];
    if (util_keylet(keylet, 34, KEYLET_ACCOUNT, dest_param, 20, 0, 0, 0, 0) != 34)
        NOPE("Treasury: Fetching Keylet Failed.");

    if (slot_set(SBUF(keylet), 1) == DOESNT_EXIST)
        NOPE("Treasury: The Set Destination Account Does Not Exist.");

    uint8_t claim[1];
    if (otxn_param(claim, 1, "C", 1) == 1)
    {
        hook_account(CACCOUNT_OUT, 20);
        *((uint32_t *)(CFLS_OUT)) = FLIP_ENDIAN(fls);
        *((uint32_t *)(CLLS_OUT)) = FLIP_ENDIAN(lls);

        etxn_details(CEMIT_OUT, 138U);

        int64_t fee = etxn_fee_base(SBUF(ctxn));
        BE_DROPS(fee);
        *((uint64_t*)(CFEE_OUT)) = fee;

        uint8_t HOOK_ROOT[34];
        if (util_keylet(HOOK_ROOT, 34, KEYLET_ACCOUNT, CACCOUNT_OUT, 20, 0, 0, 0, 0) != 34)
            NOPE("Treasury: Fetching Keylet Failed.");

        slot_set(SBUF(HOOK_ROOT), 2);

        // this is a first time claim reward has run and will setup these fields
        if (slot_subfield(2, sfRewardAccumulator, 3) != 3)
        {
            if (emit(SBUF(emithash), SBUF(ctxn)) != 32)
                NOPE("Treasury: Reward Claim Setup Failed.");
            DONE("Treasury: Reward Claim Setup Passed.");
        }

        slot_subfield(2, sfRewardTime, 4);
        int64_t time = slot(0, 0, 4);
        int64_t time_elapsed = ledger_last_time() - time;

        int64_t xfl_rr = DEFAULT_REWARD_RATE;
        int64_t xfl_rd = DEFAULT_REWARD_DELAY;

        // load state if it exists
        state_foreign(&xfl_rr, 8, "RR", 2, SBUF(reward_ns), SBUF(genesis_acc));
        state_foreign(&xfl_rd, 8, "RD", 2, SBUF(reward_ns), SBUF(genesis_acc));

        // if either of these is 0 that's disabled
        if (xfl_rr <= 0 || xfl_rd <= 0)
            NOPE("Treasury: Rewards are disabled by governance.");

        int64_t required_delay = float_int(xfl_rd, 0, 0);
        if (required_delay < 0 || float_sign(xfl_rr) != 0 ||
            float_compare(xfl_rr, float_one(), COMPARE_GREATER) ||
            float_compare(xfl_rd, float_one(), COMPARE_LESS))
            NOPE("Treasury: Rewards incorrectly configured by governance or unrecoverable error.");

        if (time_elapsed < required_delay)
        {
            time_elapsed = required_delay - time_elapsed;
            rmsg_buf[14] += (time_elapsed / 1000000) % 10;
            rmsg_buf[15] += (time_elapsed / 100000) % 10;
            rmsg_buf[16] += (time_elapsed / 10000) % 10;
            rmsg_buf[17] += (time_elapsed / 1000) % 10;
            rmsg_buf[18] += (time_elapsed / 100) % 10;
            rmsg_buf[19] += (time_elapsed / 10) % 10;
            rmsg_buf[20] += (time_elapsed) % 10;
            NOPE(rmsg_buf);
        }

        if (emit(SBUF(emithash), SBUF(ctxn)) != 32)
            NOPE("Treasury: Failed To Emit.");

        DONE("Treasury: Claimed successfully.");
    }

    uint64_t amount_xfl;
    if (otxn_param(SVAR(amount_xfl), "W", 1) != 8)
        NOPE("Treasury: Specify The Amount To Withdraw.");

    if (float_compare(amount_xfl, amt_param, COMPARE_GREATER) == 1)
        NOPE("Treasury: Outgoing transaction exceeds the amount limit set by you.");

    uint64_t drops = float_int(amount_xfl, 6, 1);
    BE_DROPS(drops);
    *((uint64_t*)(AMOUNT_OUT)) = drops;

    hook_account(ACC_OUT, 20);
    ACCOUNT_TO_BUF(DEST_OUT, dest_param);

    uint32_t last_release = 0;
    state(SVAR(last_release), "LAST", 4);

    uint32_t lgr_elapsed = last_release + ledger_param;
    if (lgr_elapsed > current_ledger)
    {
        lgr_elapsed = lgr_elapsed - current_ledger;
        lmsg_buf[14] += (lgr_elapsed / 1000000) % 10;
        lmsg_buf[15] += (lgr_elapsed / 100000) % 10;
        lmsg_buf[16] += (lgr_elapsed / 10000) % 10;
        lmsg_buf[17] += (lgr_elapsed / 1000) % 10;
        lmsg_buf[18] += (lgr_elapsed / 100) % 10;
        lmsg_buf[19] += (lgr_elapsed / 10) % 10;
        lmsg_buf[20] += (lgr_elapsed) % 10;
        NOPE(lmsg_buf);
    }

    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);
    etxn_details(EMIT_OUT, 138U);

    int64_t fee = etxn_fee_base(SBUF(txn));
    BE_DROPS(fee);
    *((uint64_t*)(FEE_OUT)) = fee;

    if (emit(SBUF(emithash), SBUF(txn)) != 32)
        NOPE("Treasury: Failed To Emit.");

    if (state_set(SVAR(current_ledger), "LAST", 4) != 4)
        NOPE("Treasury: Could not update state entry, bailing.");

    if (state_set(SVAR(last_release), "PREV", 4) != 4)
        NOPE("Treasury: Could not update state entry, bailing.");

    DONE("Treasury: Released successfully.");
    _g(1, 1);
    return 0;
}
