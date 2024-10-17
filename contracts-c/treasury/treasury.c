#include "hookapi.h"

#define DONE(x)\
    accept(SVAR(x),(uint32_t)__LINE__);

#define NOPE(x)\
{\
    return rollback((x), sizeof(x), __LINE__);\
}

#define OTXN_AMT_TO_XFL(buf) float_set(-6, (AMOUNT_TO_DROPS(buf)))
#define ttHOOK_SET 22U
#define MAX_AMOUNT 6215967485771284480ULL // 10 Million XAH
// #define MIN_LEDGER_LIMIT 324000 // 15 days
// #define MAX_LEDGER_LIMIT 7884000 // 365 days
#define MIN_LEDGER_LIMIT 3 // 12 seconds
#define MAX_LEDGER_LIMIT 5 // 20 seconds

uint8_t msg_buf[] = "You must wait 0000000 ledgers";
int64_t hook(uint32_t reserved)
{

    int64_t type = otxn_type();
    uint32_t last_release = 0;
    state(SVAR(last_release), "LAST", 4);
    if (last_release == 0 && type == ttHOOK_SET)
        DONE("Treasury: Hook Set Successfully.");

    if (type == ttCLAIM_REWARD)
        DONE("Treasury: ClaimReward Successful.");

    if (type != ttPAYMENT)
        NOPE("Treasury: Only ClaimReward and Payment txns are allowed.");

    uint64_t amt_param;
    if(hook_param(SVAR(amt_param), "A", 1) != 8)
        NOPE("Treasury: Misconfigured. Amount 'A' not set as Hook parameter.");

    if(float_compare(amt_param, 0, COMPARE_LESS | COMPARE_EQUAL) == 1)
        NOPE("Treasury: Invalid amount.");

    if(float_compare(amt_param, MAX_AMOUNT, COMPARE_GREATER | COMPARE_EQUAL) == 1)
        NOPE("Treasury: You don't want to set it to 10M plus XAH!");

    uint32_t ledger_param;
    if(hook_param(SVAR(ledger_param), "L", 1) != 4)
        NOPE("Treasury: Misconfigured. Ledger limit 'L' not set as Hook parameter.");

    // 324000: changed to 50 for testing
    TRACEVAR(ledger_param);
    if(ledger_param < MIN_LEDGER_LIMIT || ledger_param > MAX_LEDGER_LIMIT)
        NOPE("Treasury: Ledger limit must be between 324,000(15 days) and 7,884,000(365 days).");

    uint8_t dest_param[20];
    if(hook_param(SBUF(dest_param), "D", 1) != 20)
        NOPE("Treasury: Misconfigured. Destination 'D' not set as Hook parameter.");

    uint8_t keylet[34];
    if (util_keylet(keylet, 34, KEYLET_ACCOUNT, dest_param, 20, 0, 0, 0, 0) != 34)
        NOPE("Treasury: Fetching Keylet Failed.");

    if (slot_set(SBUF(keylet), 1) == DOESNT_EXIST)
        NOPE("Treasury: The Set Destination Account Does Not Exist.");

    uint8_t account[20];
    otxn_field(SBUF(account), sfAccount);

    uint8_t hook_acc[20];
    hook_account(hook_acc, 20);
    if(!BUFFER_EQUAL_20(hook_acc, account))
        DONE("Treasury: Incoming Transaction.");

    uint8_t dest[20];
    otxn_field(SBUF(dest), sfDestination);
    if(!BUFFER_EQUAL_20(dest, dest_param))
        NOPE("Treasury: Destination does not match.");

    uint32_t current_ledger =  ledger_seq();
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
        // NOPE("Treasury: You need to wait longer to withdraw.");
    }

    uint8_t amount[8];
    if(otxn_field(SBUF(amount), sfAmount) != 8)
        NOPE("Treasury: Non XAH currency payments are forbidden.");

    int64_t amount_xfl = OTXN_AMT_TO_XFL(amount);
    if(float_compare(amount_xfl, amt_param, COMPARE_GREATER) == 1)
        NOPE("Treasury: Outgoing transaction exceeds the limit set by you.");             

    if (state_set(SVAR(current_ledger), "LAST", 4) != 4)
        NOPE("Treasury: Could not update state entry.");

    DONE("Treasury: Released successfully.");
    _g(1,1);
    return 0;
}