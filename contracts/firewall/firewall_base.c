#include "hookapi.h"
#include <stdint.h>
/**
    This hook is an omnibus hook that contains 2 different hooks' functionalities. Each of these
    can be enabled or disabled and configured using the provided install-time hook parameter as
    described below:

    All integer values are little endian unless otherwise marked

    Firewall Hook
        Parameter Name: 0x4650 ('FP')
        Parameter Value: <20 byte account ID of blocklist provider>
        Parameter Name: 0x4649 ('FI')
        Parameter Value: <uint256 bit field of allowable transaction types in>
        Parameter Name: 0x464F ('FO')
        Parameter Value: <uint256 bit field of allowable transaction types out>
        Parameter Name: 0x4644 ('FD')
        Parameter Value: minimum drops threshold for incoming XRP payments (xfl LE)
        Parameter Name: 0x4654 ('FT')
        Parameter Value: minimum threshold for incoming trustline payments (xfl LE)

**/

uint8_t tts[32] = {
    0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
    0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU
};
            
int64_t hook(uint32_t r)
{
    _g(1,1);
    
    // get the account id
    uint8_t otxn_account[20];
    otxn_field(SBUF(otxn_account), sfAccount);

    uint8_t hook_acc[20];
    hook_account(SBUF(hook_acc));

    uint8_t outgoing = BUFFER_EQUAL_20(hook_acc, otxn_account);

    uint32_t tt = otxn_type();

    if (tt == 22)
        accept(SBUF("Firewall: Passing SetHook txn"), __LINE__);

    // get the relevant amount, if any
    int64_t amount = -1;
    int64_t amount_native;
    otxn_slot(1);
    
    if (slot_subfield(1, sfAmount, 1) == 1)
    {
        amount = slot_float(1);
        amount_native = slot_size(1) == 9;
    }

    // check flags
    uint8_t flagbuf[4];
    otxn_field(SBUF(flagbuf), sfFlags);

    // Blocklist
    {
        uint8_t param_name[2] = {'F', 'P'};
        uint8_t provider[20];
        hook_param(SBUF(provider), SBUF(param_name));
        uint8_t dummy[64];
        if (state_foreign(dummy, 32, SBUF(otxn_account), dummy + 32, 32, SBUF(provider)) > 0)
            rollback(SBUF("Blocklist match"), __LINE__);
    }

    // Firewall
    {
        // check allowable txn types
        {
            uint8_t param_name[2] = {'F', outgoing ? 'O' : 'I'};
            
            hook_param(tts, 32, SBUF(param_name));

            // check if its on the list of blocked txn types
            if (tts[tt >> 3] & (tt % 8))
                rollback(SBUF("Firewall blocked txn type"), __LINE__);

        }

        // if its an incoming payment ensure it passes the threshold
        if (!outgoing && amount >= 0)
        {
        
            if (flagbuf[2] & 2U)
                rollback(SBUF("Firewall blocked incoming partial payment"), __LINE__);

            // threshold for drops
            uint8_t param_name[2] = {'F', amount_native ? 'D' : 'T'};

            // if the parameter doesn't exist then the threshold is unlimited or rather 9.999999999999999e+95
            int64_t threshold = 7810234554605699071LL;
            hook_param(&threshold, 8, SBUF(param_name));
            if (float_compare(amount, threshold, COMPARE_LESS) == 1)
                rollback(SBUF("Firewall blocked amount below threshold"), __LINE__);

        }

    }

    return accept(SBUF("Firewall: Passing txn within thresholds"), __LINE__);
}