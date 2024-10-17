#include "hookapi.h"

#define OTXN_AMT_TO_XFL(buf) float_set(-6, (AMOUNT_TO_DROPS(buf)))

int64_t hook(uint32_t reserved)
{  
    // ROLLBACK: Because not setting this would be a misconfiguration
    uint64_t limit_amt;
    if(hook_param(SVAR(limit_amt), "A", 1) != 8) 
        rollback(SBUF("Treasury: Misconfigured. Amount 'A' not set as Hook parameter"), 1);

    // ROLLBACK: Because not setting this would be a misconfiguration
    int32_t limit_ledger;
    if(hook_param(SVAR(limit_ledger), "L", 1) != 4)
        rollback(SBUF("Treasury: Misconfigured. Ledger limit 'L' not set as Hook parameter"), 2);

    uint8_t otxn_account[20];
    otxn_field(SBUF(otxn_account), sfAccount);

    uint8_t account[20];
    hook_account(SBUF(otxn_account));
    if(!BUFFER_EQUAL_20(otxn_account, account)) 
        accept(SBUF("Treasury: Incoming Transaction."), 7);     

    int64_t type = otxn_type();
    if (type == ttCLAIM_REWARD)
        accept(SBUF("Treasury: ClaimReward Successful."), 4);

    // MUST ROLLBACK
    uint8_t amount_buf[8];
    if(otxn_field(amount_buf, 8, sfAmount) != 8)
        rollback(SBUF("Treasury: Non XAH currency payments are forbidden."), 6);

    int32_t last_release = 0;
    state(SVAR(last_release), "LAST", 4);

    int32_t current_ledger =  ledger_seq();
    
    // MUST ROLLBACK
    if ((last_release + limit_ledger) > current_ledger)
        rollback(SBUF("Treasury: You need to wait longer to withdraw."), 8);

    if (last_release == 0 && type == ttSET_HOOK)
        accept(SBUF("Treasury: Hook Set Successfully."), 13);

    // MUST ROLLBACK
    if (type != ttPAYMENT)
        rollback(SBUF("Treasury: Only ClaimReward and Payment txns are allowed."), 5);

    // MUST ROLLBACK
    int64_t amount_xfl = OTXN_AMT_TO_XFL(amount_buf);  
    if(float_compare(amount_xfl, limit_amt, COMPARE_GREATER) == 1)
        rollback(SBUF("Treasury: Outgoing transaction exceeds the limit set by you."), 9);          

    if (state_set(SVAR(current_ledger), "LAST", 4) != 4)
        rollback(SBUF("Treasury: Could not update state entry, bailing."), 11);

    accept(SBUF("Treasury: Released successfully."), 12);
    _g(1,1);
    return 0;    
}