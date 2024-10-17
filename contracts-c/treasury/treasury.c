#include "hookapi.h"

#define OTXN_AMT_TO_XFL(buf) float_set(-6, (AMOUNT_TO_DROPS(buf)))

#define ttHOOK_SET 22U

#define MAX_AMOUNT 6215967485771284480ULL // 10 Million XAH
// #define MIN_LEDGER_LIMIT 324000 // 15 days
// #define MAX_LEDGER_LIMIT 7884000 // 365 days
#define MIN_LEDGER_LIMIT 3 // 12 seconds
#define MAX_LEDGER_LIMIT 5 // 20 seconds

int64_t hook(uint32_t reserved)
{  

    int64_t type = otxn_type(); 
    uint32_t last_release = 0;
    state(SVAR(last_release), "LAST", 4);
    if (last_release == 0 && type == ttHOOK_SET)
        accept(SBUF("Treasury: Hook Set Successfully."), 13);

    if (type == ttCLAIM_REWARD)
        accept(SBUF("Treasury: ClaimReward Successful."), 9);

    if (type != ttPAYMENT)
        rollback(SBUF("Treasury: Only ClaimReward and Payment txns are allowed."), 10);
    
    uint64_t amt_param;
    if(hook_param(SVAR(amt_param), "A", 1) != 8) 
        rollback(SBUF("Treasury: Misconfigured. Amount 'A' not set as Hook parameter."), 1);

    if(float_compare(amt_param, 0, COMPARE_LESS | COMPARE_EQUAL) == 1)
        rollback(SBUF("Treasury: Invalid amount."), 2); 

    if(float_compare(amt_param, MAX_AMOUNT, COMPARE_GREATER | COMPARE_EQUAL) == 1)
        rollback(SBUF("Treasury: You don't want to set it to 10M plus XAH!"), 3);         

    uint32_t ledger_param;
    if(hook_param(SVAR(ledger_param), "L", 1) != 4)
        rollback(SBUF("Treasury: Misconfigured. Ledger limit 'L' not set as Hook parameter."), 4);
    
    // 324000: changed to 50 for testing
    TRACEVAR(ledger_param);
    if(ledger_param < MIN_LEDGER_LIMIT || ledger_param > MAX_LEDGER_LIMIT)
        rollback(SBUF("Treasury: Ledger limit must be between 324,000(15 days) and 7,884,000(365 days)."), 5);

    uint8_t dest_param[20];
    if(hook_param(SBUF(dest_param), "D", 1) != 20)
        rollback(SBUF("Treasury: Misconfigured. Destination 'D' not set as Hook parameter."), 6);   

    uint8_t keylet[34];
    if (util_keylet(keylet, 34, KEYLET_ACCOUNT, dest_param, 20, 0, 0, 0, 0) != 34)
        rollback(SBUF("Treasury: Fetching Keylet Failed."), 7);

    if (slot_set(SBUF(keylet), 1) == DOESNT_EXIST)
        rollback(SBUF("Treasury: The Set Destination Account Does Not Exist."), 8);

    uint8_t account[20];
    otxn_field(SBUF(account), sfAccount);

    uint8_t hook_acc[20];
    hook_account(hook_acc, 20);
    if(!BUFFER_EQUAL_20(hook_acc, account)) 
        accept(SBUF("Treasury: Incoming Transaction."), 12);

    uint8_t dest[20];
    otxn_field(SBUF(dest), sfDestination);        
    if(!BUFFER_EQUAL_20(dest, dest_param)) 
        rollback(SBUF("Treasury: Destination does not match."), 14);

    uint32_t current_ledger =  ledger_seq();
    if ((last_release + ledger_param) > current_ledger)
        rollback(SBUF("Treasury: You need to wait longer to withdraw."), 13);

    uint8_t amount[8];
    if(otxn_field(SBUF(amount), sfAmount) != 8) 
        rollback(SBUF("Treasury: Non XAH currency payments are forbidden."), 11);

    int64_t amount_xfl = OTXN_AMT_TO_XFL(amount);          

    if(float_compare(amount_xfl, amt_param, COMPARE_GREATER) == 1)
        rollback(SBUF("Treasury: Outgoing transaction exceeds the limit set by you."), 14);               

    if (state_set(SVAR(current_ledger), "LAST", 4) != 4)
        rollback(SBUF("Treasury: Could not update state entry."), 16);
  
    accept(SBUF("Treasury: Released successfully."), 18);
    _g(1,1);
    return 0;    
}