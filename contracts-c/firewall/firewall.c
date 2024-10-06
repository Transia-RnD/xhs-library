//------------------------------------------------------------------------------
/*

Operations:

// invoke ops are:
    // C - create (user)
    // U - update (verified)
    // A - add (verified)
    // R - remove (verified)

*/
//==============================================================================

#include "hookapi.h"

#define SVAR(x) &(x), sizeof(x)

#define DONE(x)\
    return accept(SBUF(x), __LINE__)

#define NOPE(x)\
    return rollback(SBUF(x), __LINE__)

#define FLIP_ENDIAN_64(n) ((uint64_t)(((n & 0xFFULL) << 56ULL) |             \
                                      ((n & 0xFF00ULL) << 40ULL) |           \
                                      ((n & 0xFF0000ULL) << 24ULL) |         \
                                      ((n & 0xFF000000ULL) << 8ULL) |        \
                                      ((n & 0xFF00000000ULL) >> 8ULL) |      \
                                      ((n & 0xFF0000000000ULL) >> 24ULL) |   \
                                      ((n & 0xFF000000000000ULL) >> 40ULL) | \
                                      ((n & 0xFF00000000000000ULL) >> 56ULL)))

#define ttSET_HOOK 22

#define FIREWALL_MODEL 77
#define BACKUP_OFFSET 33
#define AMOUNT_OFFSET 61

uint8_t nonce_ns[32] = {
    0x21, 0x2C, 0xCE, 0x06, 0x94, 0xF0, 0x63, 0xCC, 
    0x1D, 0xB8, 0xCF, 0xA3, 0x46, 0xE2, 0x96, 0xF3, 
    0xE1, 0xF9, 0xE1, 0xF0, 0xFE, 0x93, 0x12, 0xF6, 
    0x87, 0x58, 0x00, 0xBA, 0x70, 0x57, 0x8F, 0xFE
};

int64_t hook(uint32_t r)
{
    _g(1,1);

    uint8_t txn_id[32];
    otxn_id(txn_id, 32, 0);

    uint8_t hook_accid[32];
    hook_account(hook_accid + 12, 20);

    uint8_t otxn_accid[32];
    otxn_field(otxn_accid + 12, 20, sfAccount);

    uint8_t otxn_dstid[32];
    otxn_field(otxn_dstid + 12, 20, sfDestination);

    // Incoming
    if (!BUFFER_EQUAL_20(hook_accid + 12, otxn_accid + 12))
        DONE("Firewall.c: Accepting Incoming Txn.");

    int64_t tt = otxn_type();

    // Operation
    uint8_t op;
    if (otxn_param(&op, 1, "OP", 2) != 1 && tt != ttPAYMENT)
        NOPE("Firewall.c: Missing OP parameter on Invoke.");

    uint8_t firewall_model[FIREWALL_MODEL];
    int64_t firewall_state = state(SBUF(firewall_model), hook_accid + 12, 20);
    if ((op == 'U' || op == 'A' || op == 'R') && firewall_state == DOESNT_EXIST)
        NOPE("Firewall.c: Firewall does not exist.");

    if (tt != ttINVOKE && tt != ttPAYMENT)
        NOPE("Firewall.c: Rejecting Outgoing Txn. (TT)");

    // Validate the incoming txn
    if (tt == ttINVOKE && op != 'C' && op != 'U' && op != 'A' && op != 'R')
        NOPE("Firewall.c: Rejecting Outgoing Txn. (OP)");

    uint32_t nonce_seq;
    if (tt == ttINVOKE && (op == 'U' || op == 'A' || op == 'R'))
    {
        state_foreign(SVAR(nonce_seq), hook_accid + 12, 20, SBUF(nonce_ns), hook_accid + 12, 20);
        uint8_t sig_buf[256];
        int64_t sig_len = otxn_param(SBUF(sig_buf), "SIG", 3);
        uint8_t nonce_buf[4];
        UINT32_TO_BUF(nonce_buf, nonce_seq)
        if (util_verify(nonce_buf, 4, sig_buf, sig_len, firewall_model + 0u, 33) != 1)
            NOPE("Firewall.c: Signature verification failed.");
    }

    if (tt == ttPAYMENT)
    {
        otxn_slot(1);
        slot_subfield(1, sfAmount, 2);
        uint8_t amt[48];
        if (slot_size(2) == 48)
            DONE("Firewall.c: Allowing Outgoing Txn. (IOU)");

        int64_t amt_xfl = slot_float(2);
        int64_t firewall_xfl = FLIP_ENDIAN_64(UINT64_FROM_BUF(firewall_model + AMOUNT_OFFSET));

        if (amt_xfl <= 0 || float_compare(amt_xfl, firewall_xfl, COMPARE_LESS | COMPARE_EQUAL))
            DONE("Firewall.c: Allowing Outgoing Txn. (Amount)");

        if (BUFFER_EQUAL_20(firewall_model + BACKUP_OFFSET, otxn_dstid + 12) == 1)
            DONE("Firewall.c: Allowing Outgoing Txn. (Backup)");

        uint8_t whitelist_accid[32];
        if (state(SBUF(whitelist_accid), otxn_dstid + 12, 20) != DOESNT_EXIST)
            DONE("Firewall.c: Allowing Outgoing Txn. (Whitelisted)");

        NOPE("Firewall.c: Rejecting Outgoing Txn.");
    }

    // action
    switch (op)
    {
        // create
        case 'C':
        {
            uint8_t firewall_model[FIREWALL_MODEL];
            if (state(SBUF(firewall_model), hook_accid + 12, 20) != DOESNT_EXIST)
                NOPE("Firewall.c: Firewall already exists.");

            otxn_param(SBUF(firewall_model), "FM", 2);

            state_set(SBUF(firewall_model), hook_accid + 12, 20);
            DONE("Firewall.c: Set Firewall.");
        }

        // update
        case 'U':
        {
            // TODO: Validate Which Fields Are Allowed To Be Updated
            uint8_t firewall_model[FIREWALL_MODEL];
            otxn_param(SBUF(firewall_model), "FM", 2);
            state_set(SBUF(firewall_model), hook_accid + 12, 20);
            DONE("Firewall.c: Update Firewall.");
            break;
        }

        // add whitelist
        case 'A':
        {
            uint8_t whitelist_accid[20];
            otxn_param(SBUF(whitelist_accid), "ACC", 3);
            state_set(SBUF(txn_id), whitelist_accid, 20);
            nonce_seq++;
            if (state_foreign_set(SVAR(nonce_seq), hook_accid + 12, 20, SBUF(nonce_ns), hook_accid + 12, 20) != 4)
                NOPE("Firewall.c: Failed to set state.");
            
            DONE("Firewall.c: Add Whitelist Account.");
        }

        // add whitelist
        case 'R':
        {
            uint8_t whitelist_accid[20];
            otxn_param(SBUF(whitelist_accid), "ACC", 3);
            state_set(0, 0, whitelist_accid, 20);
            nonce_seq++;
            if (state_foreign_set(SVAR(nonce_seq), hook_accid + 12, 20, SBUF(nonce_ns), hook_accid + 12, 20) != 4)
                NOPE("Firewall.c: Failed to set state.");
            
            DONE("Firewall.c: Remove Whitelist Account.");
        }
        
        default:
        {
            DONE("Firewall.c: Unknown operation.");
        }
    }

    return 0;
}