//------------------------------------------------------------------------------
/*
    Copyright (c) 2024 Transia, LLC

    This financial tool is designed for use by individuals or organizations 
    that hold the appropriate licenses and qualifications to engage with such 
    products in a manner that complies with all relevant laws and regulations. 
    Unauthorized use or redistribution of this tool may result in legal action.

    The content provided in this financial tool is for informational purposes 
    only and should not be interpreted as financial advice or an endorsement of 
    any particular investment or financial strategy. Users should seek advice 
    from a certified professional or financial advisor before making any 
    investment choices.

    By using this financial tool, the user agrees to indemnify Transia, LLC from 
    any claims, damages, or losses that may arise from their use or reliance on 
    the information contained within. The user acknowledges that they are utilizing 
    this tool at their own risk and that Transia, LLC will not be held responsible 
    for any direct, indirect, incidental, punitive, special, or consequential 
    damages, including but not limited to, damages for lost profits, goodwill, 
    usage, data, or other intangible losses.

*/
//==============================================================================

#include "hookapi.h"
#include <stdint.h>

#define LEDGER_DELAY 10
#define LEDGER_DELAY_STRING "10"

#define DEBUG 1

#define DONE(msg)\
    return accept(msg, sizeof(msg),__LINE__)

/**
    All integer values are marked for size and endianness

    High value Payments Block Hook
        Parameter Name: 485644 (HVD)
        Parameter Value: <8 byte xfl of drops threshold to block LE>
        Parameter Name: 485654 (HVT)
        Parameter Value: <8 byte xfl of trustline threshold to block LE>
**/

uint8_t drops_key[3] = {'H', 'V', 'D'};
uint8_t tl_key[3]    = {'H', 'V', 'T'};
uint8_t amount_buf[8];

int64_t hook(uint32_t r)
{
    _g(1,1);

    // pass anything that isn't a payment
    if (otxn_type() != 0)
        DONE("High value: Passing non-Payment txn");

    // get the account ids
    uint8_t otxn_acc[20];
    otxn_field(SBUF(otxn_acc), sfAccount);

    uint8_t hook_acc[20];
    hook_account(SBUF(hook_acc));

    // pass incoming txns
    if (!BUFFER_EQUAL_20(hook_acc, otxn_acc))
        DONE("High value: Ignoring incoming Payment");

    otxn_slot(1);
    slot_subfield(1, sfAmount, 2);

    int64_t threshold;
    if (hook_param(&threshold, sizeof(threshold), slot_type(2, 1) == 1 ? drops_key : tl_key, 3) != sizeof(threshold))
        DONE("High value: Passing outgoing Payment txn for which no threshold is set");

    if (DEBUG)
    {
        trace_float(SBUF("threshold"), threshold);
        trace_float(SBUF("amount"), slot_float(2));
    }

    if (float_compare(threshold, slot_float(2), COMPARE_LESS) == 1)
    {
        // check if they prepared for it
     
        /*
         * Packed state data:
         *   0-19 : destination acc
         *  20-20 : has dtag
         *  21-24 : dtag if any
         *  25-73 : amount tl/drops
         */
        
        #define DEST (data + 0)
        #define DTAG (data + 21)
        #define AMT (data + 25)

        uint8_t data[73];

        otxn_field(DEST, 20, sfDestination) == 20;
        uint8_t has_dtag = otxn_field(DTAG, 4, sfDestinationTag) == 4;
        int64_t amt_size = otxn_field(AMT, 48,  sfAmount);

        *(data + 20) = has_dtag;
        
        uint8_t hash[32];
        util_sha512h(SBUF(hash), SBUF(data));

        int64_t current_lgr = ledger_seq();
        int64_t prepare_lgr;

        if (state(&prepare_lgr, sizeof(prepare_lgr), SBUF(hash)) != sizeof(prepare_lgr))
            rollback(SBUF("High value: Payment exceeds threshold. Use Invoke to send."), __LINE__);
        
        if (current_lgr - prepare_lgr < LEDGER_DELAY)
            rollback(SBUF("High value: Too soon, wait until "LEDGER_DELAY_STRING" ledgers have passed."), __LINE__);

        // delete the state
        state(0,0, SBUF(hash));

        DONE("High value: Passing prepared high value txn");   
    }

    DONE("High value: Passing outgoing Payment less than threshold");
}