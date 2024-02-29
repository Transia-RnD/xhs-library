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

#define NOPE(x)\
    rollback(SBUF(x), __LINE__);

#define ACCOUNT_TO_BUF(buf_raw, i)                       \
    {                                                    \
        unsigned char *buf = (unsigned char *)buf_raw;   \
        *(uint64_t *)(buf + 0) = *(uint64_t *)(i + 0);   \
        *(uint64_t *)(buf + 8) = *(uint64_t *)(i + 8);   \
        *(uint32_t *)(buf + 16) = *(uint32_t *)(i + 16); \
    }

uint8_t txn[310] =
{
    /* size,upto */
    /*   3,  0  */ 0x12U, 0x00U, 0x07U,                                                             /* tt = OfferCreate */
    /*   5,  3  */ 0x22U, 0x80U, 0x00U, 0x00U, 0x00U,                                               /* flags = tfCanonical */
    /*   5,  8  */ 0x24U, 0x00U, 0x00U, 0x00U, 0x00U,                                               /* sequence = 0 */
    /*   5, 13  */ 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                                               /* dtag, flipped */
    /*   6, 18  */ 0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,                                        /* first ledger seq */
    /*   6, 24  */ 0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,                                        /* last ledger seq */
    /*  49, 30  */ 0x64U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* taker pays field 9 or 49 bytes */
                    0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* cont...  */
                    0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* cont...  */
                    0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* cont...  */
                    0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* cont...  */
                    0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* cont...  */
                    0x99,                                                                           /* cont...  */
    /*  49, 79  */ 0x65U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                          /* taker gets field 9 or 49 bytes */
                    0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* cont...  */
                    0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* cont...  */
                    0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* cont...  */
                    0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* cont...  */
                    0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,                         /* cont...  */
                    0x99,                                                                           /* cont...  */
    /*   9, 128 */ 0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,                          /* fee      */
                    0x00U,                                                                          /* cont...  */
    /*  35, 137 */ 0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* pubkey   */
    /*  22, 172 */ 0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                           /* src acc  */
    /* 116, 194 */                                                                                  /* emit details */
    /*   0, 310 */
};

// TX BUILDER
#define FLS_OUT (txn + 20U)
#define LLS_OUT (txn + 26U)
#define DTAG_OUT (txn + 14U)
#define TAKER_PAYS_OUT (txn + 30U)
#define TAKER_GETS_OUT (txn + 79U)
#define HOOK_ACC (txn + 174U)
#define FEE_OUT (txn + 129U)
#define EMIT_OUT (txn + 194U)

#define nodes 5

uint8_t book_in_curr[20] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x55U, 0x53U, 0x44U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
};

uint8_t book_out_curr[20] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x55U, 0x53U, 0x44U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
};

uint8_t book_in_acc[20] = {
    0xA4U, 0x07U, 0xAF, 0x58U, 0x56U, 0xCCU, 0xF3U, 0xC4U, 0x26U, 0x19U, 
    0xDAU, 0xA9U, 0x25U, 0x81U, 0x3FU, 0xC9U, 0x55U, 0xC7U, 0x29U, 0x83U
};
uint8_t book_out_acc[20] = {
    0xA4U, 0x07U, 0xAF, 0x58U, 0x56U, 0xCCU, 0xF3U, 0xC4U, 0x26U, 0x19U, 
    0xDAU, 0xA9U, 0x25U, 0x81U, 0x3FU, 0xC9U, 0x55U, 0xC7U, 0x29U, 0x83U
};

uint8_t book_keylet[34] = {
    0x00U, 0x64U, 0x7DU, 0x97U, 0xB4U, 0xB1U, 0x0DU, 0x3DU, 0x7CU, 0x21U, 
    0x53U, 0xE8U, 0xBCU, 0x39U, 0x07U, 0x23U, 0xC1U, 0x82U, 0xCFU, 0x89U, 
    0x1CU, 0x41U, 0xBCU, 0x15U, 0x74U, 0xCEU, 0xECU, 0x9EU, 0x14U, 0xF5U, 
    0x40U, 0x0BU, 0xFAU, 0xAEU
};

int64_t
hook(uint32_t r)
{
    _g(1,1);

    // ACCOUNT: Hook Account
    hook_account(HOOK_ACC, 20);

//     uint8_t data[81];
//     data[1] = 'B';
// #define BOOK_IN_CURR (data + 1U)
// #define BOOK_OUT_CURR (data + 21U)
// #define BOOK_IN_ACC (data + 41U)
// #define BOOK_OUT_ACC (data + 61U)
//     ACCOUNT_TO_BUF(BOOK_IN_CURR, book_in_curr);
//     ACCOUNT_TO_BUF(BOOK_OUT_CURR, book_out_curr);
//     ACCOUNT_TO_BUF(BOOK_IN_ACC, book_in_acc);
//     ACCOUNT_TO_BUF(BOOK_OUT_ACC, book_out_acc);
//     TRACEHEX(data);

//     uint8_t hash_out[32];
//     util_sha512h(hash_out, 32, SBUF(data));

    // TRACEHEX(hash_out);

    // book_keylet

    uint8_t quality_buf[34];
    int64_t keylet_response = util_keylet(SBUF(quality_buf), KEYLET_QUALITY, SBUF(book_keylet), 0, 1, 0, 0);
    TRACEVAR(keylet_response);
    TRACEHEX(quality_buf);
    // 00647D97B4B10D3D7C2153E8BC390723C182CF891C41BC1574CE0000000000000000
    // 00641C5C34DB7DBE43E1EA72EE080416E88A87C18B2AD29BD8C40000000000000001

    // SLOT SET:
    int64_t value1 = slot_set(SBUF(quality_buf), 1);
    TRACEVAR(value1);
    if (slot_set(SBUF(quality_buf), 1) != 1)
        accept(SBUF("keylet_quality: Could not slot keylet `sfQuality`"), __LINE__);

    if (slot_subfield(1, sfExchangeRate, 2) != 2)
        accept(SBUF("keylet_quality: Could not slot subfield `sfExchangeRate`"), __LINE__);

    uint8_t exchange_buff[8];
    int64_t value = slot(SBUF(exchange_buff), 2);
    TRACEVAR(value);
    TRACEHEX(exchange_buff);

    switch (r)
    {
    case 0:
        TRACESTR("tsh.c: Strong. Execute BEFORE transaction is applied to ledger");
        hook_again();
        break;
    case 1:
        TRACESTR("tsh.c: Weak. Execute AFTER transaction is applied to ledger");
        break;
    case 2:
        TRACESTR("tsh.c: Weak Again. Execute AFTER transaction is applied to ledger");
        uint8_t dump1[2048];
        meta_slot(1);
        int64_t len1 = slot(dump1, sizeof(dump1), 1);
        TRACEVAR(len1);
        TRACEHEX(dump1);

        if (slot_subfield(meta_slot, sfAffectedNodes, nodes) != nodes)
            NOPE("topup.c: Could not slot sfAffectedNodes");

        uint8_t dump2[1024];
        trace(SBUF("slot nodes"), dump2, slot(SBUF(dump2), nodes), 1);

        int64_t count = slot_count(nodes);
        if (count > 5)
            count = 5;

        int64_t found = 0;

        for (int i = 0; GUARD(5), i < count; ++i)
        {
            if (slot_subarray(nodes, i, 6) != 6)
                continue;

            if (slot_subfield(6, sfLedgerEntryType, 8) != 8)
                NOPE("topup.c: Could not slot LedgerEntryType");

            uint8_t buf[2];
            if (slot(SBUF(buf), 8) != 2)
                NOPE("topup.c: Could not dump LedgerEntryType");

            if (UINT16_FROM_BUF(buf) == 0x0061U)
            {
                found = 1;
                break;
            }
        }

        if (!found)
            NOPE("topup.c: Could not find Payment in xpop metadata");

        if (slot_subfield(6, sfPreviousFields, 9) != 9 || slot_subfield(6, sfFinalFields, 10) != 10)
            NOPE("topup.c: Could not slot sfPreviousFields or sfFinalFields");

        uint8_t prev_balance[8];
        uint8_t final_balance[8];
        if (slot_subfield(9, sfBalance, 11) != 11 || slot_subfield(10, sfBalance, 12) != 12)
            NOPE("topup.c: Could not slot sfBalance");

        if (slot(SVAR(prev_balance), 11) != 8 || slot(SVAR(final_balance), 12) != 8)
            NOPE("topup.c: Could not dump sfBalance");

        int64_t prev_bal_drops = AMOUNT_TO_DROPS(prev_balance);
        int64_t final_bal_drops = AMOUNT_TO_DROPS(final_balance);
        TRACEHEX(prev_balance)
        TRACEHEX(final_balance)
        int64_t diff_drops = prev_bal_drops - final_bal_drops;
        TRACEVAR(diff_drops)
        
        break;
    default:
        break;
    }

    return accept(SBUF("topup.c: Accept."), __LINE__);
}

