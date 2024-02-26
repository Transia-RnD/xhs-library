#include "hookapi.h"
#include <stdint.h>

#define LEDGER_DELAY 10
#define LEDGER_DELAY_STRING "10"

#define DEBUG 1

/**
    All integer values are marked for size and endianness

    High value Payments Block Hook
        Parameter Name: 485644 (HVD)
        Parameter Value: <8 byte big endian xfl of drops threshold to block LE>
        Parameter Name: 485654 (HVT)
        Parameter Value: 48 bytes
            <8 byte big endian amount>
            [20 byte currency code, not present if xrp]
            [20 byte issuer code not present is xrp]
**/

uint8_t drops_key[3] = {'H', 'V', 'D'};
uint8_t tl_key[3] = {'H', 'V', 'T'};
uint8_t amount_buf[8];

int64_t hook(uint32_t r)
{
    _g(1,1);

    // FILTER: Transation Type
    if (otxn_type() != ttPAYMENT)
        DONEMSG("High value: Passing non-Payment txn");

    // ACCOUNT:
    uint8_t otxn_acc[20];
    otxn_field(SBUF(otxn_acc), sfAccount);

    // ACCOUNT:
    uint8_t hook_acc[20];
    hook_account(SBUF(hook_acc));

    // FILTER: I/O
    if (!BUFFER_EQUAL_20(hook_acc, otxn_acc))
        DONEMSG("High value: Ignoring incoming Payment");

    otxn_slot(1);
    slot_subfield(1, sfAmount, 2);

    if (DEBUG)
    {
        trace_float(SBUF("amount"), slot_float(2));
    }

    // HOOK PARAM: <8 byte xfl of drops threshold to block LE>
    int64_t threshold;
    if (hook_param(&threshold, sizeof(threshold), slot_type(2, 1) == 1 ? drops_key : tl_key, 3) != sizeof(threshold))
        DONEMSG("High value: Passing outgoing Payment txn for which no threshold is set");

    if (DEBUG)
    {
        trace_float(SBUF("threshold"), threshold);
    }

    // VALIDATION: Math
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

        // STATE: Get
        if (state(&prepare_lgr, sizeof(prepare_lgr), SBUF(hash)) != sizeof(prepare_lgr))
            rollback(SBUF("High value: Payment exceeds threshold"), __LINE__);

        // VALIDATION: Math
        if (current_lgr - prepare_lgr < LEDGER_DELAY)
            rollback(SBUF("High value: Too soon, wait until "LEDGER_DELAY_STRING" ledgers have passed"), __LINE__);

        // STATE: Delete
        state_set(0,0, SBUF(hash));

        DONEMSG("High value: Passing prepared high value txn");   
    }

    DONEMSG("High value: Passing outgoing Payment less than threshold");
}
