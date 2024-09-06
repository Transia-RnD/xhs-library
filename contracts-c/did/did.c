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

#include <stdint.h>
#include "hookapi.h"

#define DONE(x)\
    return accept(SBUF(x), __LINE__)

#define NOPE(x)\
    return rollback(SBUF(x), __LINE__)

#define SIG_OFFSET 52U

const uint8_t dids_namespace[] = {
    0x43U, 0xF6U, 0xCBU, 0x20U, 0x14U, 0x01U, 0x66U, 0xCCU,
    0x9EU, 0x7AU, 0x5EU, 0x07U, 0xFCU, 0xA8U, 0xE8U, 0x4CU,
    0xB7U, 0xE5U, 0x76U, 0x21U, 0x92U, 0x78U, 0x8BU, 0x5DU,
    0x4CU, 0xD0U, 0x8FU, 0x1FU, 0xD0U, 0x5FU, 0xF1U, 0x58U
};

int64_t hook(uint32_t r)
{
    _g(1,1);

    uint8_t hook_acc[32];
    hook_account(hook_acc + 12, 20);

    uint8_t otx_acc[32];
    otxn_field(otx_acc + 12, 20, sfAccount);

    if (BUFFER_EQUAL_20(hook_acc, otx_acc))
        DONE("did.c: passing outgoing txn");

    int64_t tt = otxn_type();
    if (tt != ttINVOKE)
        NOPE("did.c: Rejecting non-Invoke, non-Payment txn.");

    // get admin account
    uint8_t admin[20];
    if (hook_param(SBUF(admin), "ADM", 3) != 20)
        NOPE("did.c: Misconfigured. Missing ADM install parameter.");

    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1)
        NOPE("did.c: Missing OP parameter on Invoke.");

    TRACEHEX(op);

    uint8_t so;
    if (otxn_param(&so, 1, "SO", 2) != 1)
        NOPE("did.c: Missing SO parameter on Invoke.");

    TRACEHEX(so);

    // admin invoke ops are: 
    // VC - validator create (admin)
    // VU - validator update (admin)
    // VD - validator delete (admin)
    
    // validator invoke ops are: 
    // DC - did create (validator)
    // DU - did update (validator)
    // DD - did delete (validator)

    // admin permission check
    int64_t is_admin = BUFFER_EQUAL_20(otx_acc + 12, admin);
    if (!is_admin && op == 'V')
        NOPE("did.c: Admin only operation.");

    // validator permission check
    uint8_t acc[20];
    uint8_t key[33];
    if ((op == 'D') && state(SBUF(key), otx_acc + 12, 20) == DOESNT_EXIST)
    {
        NOPE("did.c: Validator does not exist.");
    }

    // get signature if any
    // Signature format is packed binary data of the form:
    // <20 byte dest accid><32 byte var string><signature>
    uint8_t sig_buf[256];
    int64_t sig_len = otxn_param(SBUF(sig_buf), "SIG", 3);
    if (sig_len > 0)
    {
        if (sig_len < 80)
            NOPE("did.c: Signature too short.");

        if (!util_verify(sig_buf, SIG_OFFSET, sig_buf + SIG_OFFSET, sig_len - SIG_OFFSET, SBUF(key)))
            NOPE("did.c: Signature verification failed.");
    }
    // action
    switch (op)
    {
        case 'V':
        {
            otxn_param(SBUF(acc), "VA", 2);
            otxn_param(SBUF(key), "VK", 2);
            switch (so)
            {
                case 'C':
                case 'U':
                {
                    state_set(SBUF(key), acc, 20);
                    DONE("did.c: Validator Created.");
                }
                case 'D':
                {
                    state_set(0, 0, acc, 20);
                    DONE("did.c: Validator Deleted.");
                }
                default:
                    NOPE("did.c: Unknown sub operation.");
            }
        }
        case 'D':
        {
            otxn_param(SBUF(acc), "CA", 2);
            switch (so)
            {
                case 'C':
                case 'U':
                {
                    state_foreign_set(SBUF(sig_buf), acc, 20, dids_namespace, 32, hook_acc + 12, 20);
                    DONE("did.c: DID Create.");
                }
                case 'D':
                {
                    state_foreign_set(0, 0, acc, 20, dids_namespace, 32, hook_acc + 12, 20);
                    DONE("did.c: DID Create.");
                }
            
            default:
                NOPE("did.c: Unknown sub operation.");
            }
        }

        default:
        {
            NOPE("did.c: Unknown operation.");
        }
    }

    return 0;
}