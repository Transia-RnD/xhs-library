//------------------------------------------------------------------------------
/*
    Copyright (c) 2023 Transia, LLC

    This financial product is intended for use by individuals or entities who
    possess the necessary licenses and qualifications to solicit and utilize
    such products in accordance with applicable laws and regulations.
    Unauthorized use or distribution of this product may be subject to legal
    consequences.

    The information provided in this financial product is for informational
    purposes only and should not be considered as financial advice or a
    recommendation to engage in any specific investment or financial strategy.
    It is important to consult with a qualified professional or financial
    advisor before making any investment decisions.
*/
//==============================================================================

#include <stdint.h>
#include "hookapi.h"

// -----------------------------------------------------------------------------

/**
 * 
 * These functions should be moved into the macro.c file
*/
#define ASSERT(x)\
    if (!(x))\
        rollback(SBUF("Govern: Assertion failed."),__LINE__);

#define DONE(x)\
    accept(SBUF(x),__LINE__);

#define SVAR(x) &x, sizeof(x)

#define INT8_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    buf[0] = (((uint8_t)i) >> 0) & 0xFFUL;\
    if (i < 0) buf[0] |= 0x80U;\
}

#define BUFFER_EQUAL_32(buf1, buf2)\
    (\
        *(((uint64_t*)(buf1)) + 0) == *(((uint64_t*)(buf2)) + 0) &&\
        *(((uint64_t*)(buf1)) + 1) == *(((uint64_t*)(buf2)) + 1) &&\
        *(((uint64_t*)(buf1)) + 2) == *(((uint64_t*)(buf2)) + 2) &&\
        *(((uint64_t*)(buf1)) + 3) == *(((uint64_t*)(buf2)) + 3))

#define BUFFER_EQUAL_64(buf1, buf2) \
    ( \
        (*((uint64_t*)(buf1) + 0) == *((uint64_t*)(buf2) + 0)) && \
        (*((uint64_t*)(buf1) + 1) == *((uint64_t*)(buf2) + 1)) && \
        (*((uint64_t*)(buf1) + 2) == *((uint64_t*)(buf2) + 2)) && \
        (*((uint64_t*)(buf1) + 3) == *((uint64_t*)(buf2) + 3)) && \
        (*((uint64_t*)(buf1) + 4) == *((uint64_t*)(buf2) + 4)) && \
        (*((uint64_t*)(buf1) + 5) == *((uint64_t*)(buf2) + 5)) && \
        (*((uint64_t*)(buf1) + 6) == *((uint64_t*)(buf2) + 6)) && \
        (*((uint64_t*)(buf1) + 7) == *((uint64_t*)(buf2) + 7)) \
    )

#define ACCOUNT_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    *(uint64_t*)(buf + 0) = *(uint64_t*)(i +  0);\
    *(uint64_t*)(buf + 8) = *(uint64_t*)(i +  8);\
    *(uint32_t*)(buf + 16) = *(uint32_t*)(i + 16);\
}

#define UINT256_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    *(uint64_t*)(buf + 0) = *(uint64_t*)(i +  0);\
    *(uint64_t*)(buf + 8) = *(uint64_t*)(i +  8);\
    *(uint64_t*)(buf + 16) = *(uint64_t*)(i + 16);\
    *(uint64_t*)(buf + 24) = *(uint64_t*)(i + 24);\
}

#define NOPE(x)                                \
{                                              \
    return rollback((x), sizeof(x), __LINE__); \
}

#define FLIP_ENDIAN_64(n) ((uint64_t)(((n & 0xFFULL) << 56ULL) |             \
                                      ((n & 0xFF00ULL) << 40ULL) |           \
                                      ((n & 0xFF0000ULL) << 24ULL) |         \
                                      ((n & 0xFF000000ULL) << 8ULL) |        \
                                      ((n & 0xFF00000000ULL) >> 8ULL) |      \
                                      ((n & 0xFF0000000000ULL) >> 24ULL) |   \
                                      ((n & 0xFF000000000000ULL) >> 40ULL) | \
                                      ((n & 0xFF00000000000000ULL) >> 56ULL)))

// -----------------------------------------------------------------------------


/*

"Admin"
Admin Managers

Topics:
- Seat: S
- Quote: Q
- Distribution: D
- - Distributions
- - Q-ID        + % of payment as XLF
- - HFUAEDUEODO + 6071852297695428608

Seat Count: 20
Quorum: 51%

*/

// BINARY MODEL
#define MAX_MODEL_BYTES 94

// HARD CODED
#define SEAT_COUNT 20
#define QUORUM 0.51

int64_t hook(uint32_t r)
{
    // HookOn: Invoke
    if (otxn_type() != ttINVOKE) // ttINVOKE only
        DONE("admin.c: Passing non-Invoke txn. HookOn should be changed to avoid this.");

    // ACCOUNT: Transaction
    uint8_t otxn_accid[32];
    otxn_field(otxn_accid + 12, 20, sfAccount);

    // ACCOUNT: Hook
    uint8_t hook_accid[32];
    hook_account(hook_accid + 12, 20);

    int64_t member_count = state(0, 0, "MC", 2);
    TRACEVAR(member_count);

    // initial execution, setup hook
    if (BUFFER_EQUAL_20(hook_accid + 12, otxn_accid + 12) && member_count == DOESNT_EXIST)
    {
        // uint64_t irr, ird;
        // gather hook parameters
        uint8_t imc;
        if (hook_param(SVAR(imc), "IMC", 3) < 0)
            NOPE("admin.c: initial Member Count Parameter missing (IMC).");

        if (imc == 0)
            NOPE("admin.c: initial Member Count must be > 0.");

        if (imc > SEAT_COUNT)
            NOPE("admin.c: initial Member Count must be <= Seat Count (20).");

        // set member count
        ASSERT(0 < state_set(SVAR(imc), "MC", 2));

        member_count = imc;

        for (uint8_t i = 0; GUARD(SEAT_COUNT), i < member_count; ++i)
        {
            uint8_t member_acc[20];
            uint8_t member_pkey[3] = {'I', 'S', i};
            if (hook_param(SBUF(member_acc), member_pkey, 3) != 20)
                NOPE("admin.c: one or more initial member account ID's is missing.");

            // 0... X where X is member id started from 1
            // maps to the member's account ID
            trace(SBUF("admin.c: Member: "), SBUF(member_acc), 1);
            // reverse key
            ASSERT(state_set(SBUF(member_acc), SVAR(i)) == 20);
            // 0, 0... ACCOUNT ID maps to member_id (as above)

            // forward key
            ASSERT(state_set(SVAR(i), SBUF(member_acc)) == 1);
        }

        DONE("admin.c: setup completed successfully.");
    }

    if (BUFFER_EQUAL_20(hook_accid + 12, otxn_accid + 12))
        NOPE("admin.c: outgoing tx on `Account`.");

    int64_t member_id = state(0, 0, otxn_accid + 12, 20);
    if (member_id < 0)
        NOPE("admin.c: you are not currently a governance member at this table.");

    TRACEVAR(member_id);

    // { 'S|D', '\0 + topicid' }
    uint8_t topic[5];
    int64_t result = otxn_param(SBUF(topic), "T", 1);
    TRACEHEX(topic);
    uint8_t t = topic[0]; // topic type
    uint8_t n = topic[1]; // number (seats | other)

    TRACEHEX(t);

    if (result != 5 || (t != 'S' && // topic type: seat (L1)
                        t != 'L'))  // topic type: distribution
        NOPE("admin.c: valid TOPIC must be specified as otxn parameter.");

    if (t == 'S' && n > (SEAT_COUNT - 1))
        NOPE("admin.c: valid seat topics are 0 through 19.");

    uint8_t topic_data[32];
    uint8_t topic_size = t == 'L' ? 28 : t == 'S' ? 20 : 0;

    uint8_t padding = 32 - topic_size;
    result = otxn_param(topic_data + padding, topic_size, "V", 1);
    if (result != topic_size)
        NOPE("admin.c: missing or incorrect size of VOTE data for TOPIC type.");

    // set this flag if the topic data is all zeros
    int topic_data_zero = 
        (*((uint64_t*)(topic_data +  0)) == 0) &&
        (*((uint64_t*)(topic_data +  8)) == 0) &&
        (*((uint64_t*)(topic_data + 16)) == 0) &&
        (*((uint64_t*)(topic_data + 24)) == 0);

    // reuse otxn_accid to create vote key
    otxn_accid[0] = 'V';
    otxn_accid[1] = t;
    otxn_accid[2] = n;
    otxn_accid[3] = topic[2];
    otxn_accid[4] = topic[3];
    otxn_accid[5] = topic[4];

    TRACEHEX(topic_data);
    TRACEVAR(topic_size);
    TRACEHEX(otxn_accid);

    // get their previous vote if any on this topic
    uint8_t previous_topic_data[32];
    int64_t previous_topic_size = state(previous_topic_data + padding, topic_size, SBUF(otxn_accid));
    TRACEHEX(previous_topic_data);

    if (previous_topic_size == topic_size && previous_topic_data[0] != 0)
    {
        DONE("admin.c: proposal is closed.");
    }

    uint8_t previous_vote_data[32];
    int64_t previous_vote_size = state(previous_vote_data + padding, topic_size, otxn_accid + 2, 4);
    if (previous_vote_size == topic_size && BUFFER_EQUAL_32(previous_vote_data, topic_data))
    {
        DONE("admin.c: loan is already approved.");
    }

    uint8_t previous_quote_data[32];
    uint8_t quote_key[5];
    quote_key[0] = 'L';
    quote_key[1] = n;
    quote_key[2] = topic[2];
    quote_key[3] = topic[3];
    quote_key[4] = topic[4];
    int64_t previous_quote_size = state(previous_quote_data, 28, quote_key, 5);
    TRACEHEX(quote_key);
    TRACEHEX(previous_quote_data);
    TRACEHEX(previous_vote_data);
    TRACEVAR(previous_quote_size);
    if (previous_quote_size != 28)
    {
        DONE("admin.c: loan doesn't exist.");
    }

    // check if the vote they're making has already been cast before,
    // if it is identical to their existing vote for this topic then just end with tesSUCCESS
    if (previous_topic_size == topic_size && BUFFER_EQUAL_32(previous_topic_data, topic_data))
        DONE("admin.c: your vote is already cast this way for this topic.");

    // execution to here means the vote is different
    // we might have to decrement the old voting if they voted previously
    // and we will have to increment the new voting

    // write vote to their voting key
    ASSERT(state_set(topic_data + padding, topic_size, SBUF(otxn_accid)) == topic_size);
    TRACESTR("admin.c: writing vote.");

    uint8_t previous_votes = 0;
    // decrement old vote counter for this option
    if (previous_topic_size > 0)
    {
        uint8_t votes = 0;
        // override the first two bytes to turn it into a vote count key
        previous_topic_data[0] = 'C';
        previous_topic_data[1] = t;
        previous_topic_data[2] = n;

        ASSERT(state(&votes, 1, SBUF(previous_topic_data)) == 1);
        ASSERT(votes > 0);
        previous_votes = votes;
        votes--;
        // delete the state entry if votes hit zero
        ASSERT(state_set(votes == 0 ? 0 : &votes, votes == 0 ? 0 : 1, SBUF(previous_topic_data)) >= 0);
    }
    
    // increment new counter 
    uint8_t votes = 0;
    {
        // we're going to clobber the topic data to turn it into a vote count key
        // so store the first bytes 
        uint64_t saved_data = *((uint64_t*)topic_data);
        topic_data[0] = 'C';
        topic_data[1] = t;
        topic_data[2] = n;

        state(&votes, 1, topic_data, 32);
        votes++;
        ASSERT(0 < state_set(&votes, 1, topic_data, 32));

        // restore the saved bytes
        *((uint64_t*)topic_data) = saved_data;
    }

    int64_t req_quorum = member_count * QUORUM;
    if (votes < req_quorum) // 51% threshold for all voting
        DONE("admin.c: vote recorded. Not yet enough votes to action.");

    switch (t)
    {
    case 'Q':
    {
        TRACESTR(SBUF("LOAN APPROVED"));
        uint8_t id[5];
        id[0] = 'L';
        id[1] = n;
        id[2] = topic[2];
        id[3] = topic[3];
        id[4] = topic[4];
        // save quote data
        ASSERT(state_set(topic_data + 4, 28, id, 5) == 28);
        DONE("admin.c: loan approved.");
    }
    case 'S':
    {
        // add / change member
        uint8_t previous_member[32];
        int previous_present = (state(previous_member + 12, 20, &n, 1) == 20);
        if (previous_present)
        {
            trace(SBUF("Previous present==:"), previous_member, 32, 1);
        }


        if (BUFFER_EQUAL_20((previous_member + 12), (topic_data + 12)))
            DONE("admin.c: Actioning seat change, but seat already contains the new member.");

        int64_t existing_member = state(0,0, topic_data + 12, 20);

        int existing_member_moving = existing_member >= 0;
        if (existing_member_moving)
            trace(SBUF("admin.c: Moving existing member to new seat."), 0,0,0);


        uint8_t op = ((!previous_present) << 2U) +
                (topic_data_zero << 1U) + existing_member_moving;

        ASSERT(op != 0b011U && op != 0b111U && op < 8);

        // logic table:
        // E/!E - seat is empty/filled
        // Z/!Z - topic data is zero non zero (zero = member deletion)
        // M/!M - topic is an existing member who is moving
        //
        // E|Z|M
        // -+-+-
        // 0 0 0    - seat is full, vote is for a member, an existing member is not moving              MC
        // 0 0 1    - seat is full, vote is for a member, an existing member is moving                  MC--
        // 0 1 0    - seat is full, vote is for deletion, an existing member is not moving              MC--
        // 0 1 1    - seat is full, vote is for deletion, an existing member is moving (impossible)
        // 1 0 0    - seat is empty, vote is for a member, member is not an existing member             MC++
        // 1 0 1    - seat is empty, vote is for a member, member is an existing member                 MC
        // 1 1 0    - seat is empty, vote is for deletion, not an existing member moving                MC
        // 1 1 1    - seat is empty, vote is for deletion, an existing member moving (impossible)

        TRACEVAR(op);
        trace_num(SBUF("E"), !previous_present);
        trace_num(SBUF("Z"), topic_data_zero);
        trace_num(SBUF("M"), existing_member_moving);

        // adjust member count
        {
            if (op == 0b001U || op == 0b010U)
                member_count--;
            else if (op == 0b100U)
                member_count++;

            TRACEVAR(previous_present);
            TRACEVAR(topic_data_zero); 
            TRACEVAR(member_count);

            ASSERT(member_count > 1);   // just bail out if the second last member is being removed

            uint8_t mc = member_count;
            ASSERT(state_set(&mc, 1, "MC", 2) == 1);
        }

        // if an existing member is moving we need to delete them before re-adding them
        if (existing_member_moving)
        {

            // delete the old member
            // reverse key 
            uint8_t m = (uint8_t)existing_member;
            ASSERT(state_set(0,0, &m, 1) == 0);
            
            // forward key
            ASSERT(state_set(0, 0, topic_data + 12, 20) == 0);
            
        }

        // we need to garbage collect all their votes
        if (previous_present)
        {
            previous_member[0] = 'V';

            for (int i = 1; GUARD(32), i < 32; ++i)
            {
                previous_member[1] = i < 12 ? 'L' : 'S';
                previous_member[2] = 
                    i < 12 ? i - 2 :
                    i - 12;

                uint8_t vote_key[32];
                if (state(SBUF(vote_key), SBUF(previous_member)) == 32)
                {
                    uint8_t vote_count = 0;

                    // find and decrement the vote counter
                    vote_key[0] = 'C';
                    vote_key[1] = previous_member[1];
                    vote_key[2] = previous_member[2];
                    if (state(&vote_count, 1, SBUF(vote_key)) == 1)
                    {
                        // if we're down to 1 vote then delete state
                        if (vote_count <= 1)
                        {
                            ASSERT(state_set(0,0, SBUF(vote_key)) == 0);
                            trace_num(SBUF("Decrement vote count deleted"), vote_count);
                        }
                        else    // otherwise decrement
                        {
                            vote_count--;
                            ASSERT(state_set(&vote_count, 1, SBUF(vote_key)) == 1);
                            trace_num(SBUF("Decrement vote count to"), vote_count);
                        }
                    }

                    // delete the vote entry
                    ASSERT(state_set(0,0, SBUF(previous_member)) == 0);
                    trace(SBUF("Vote entry deleted"), vote_key, 32, 1);
                }
            }

            // delete the old member
            // reverse key 
            ASSERT(state_set(0,0, &n, 1) == 0);
            
            // forward key
            ASSERT(state_set(0, 0, previous_member + 12, 20) == 0);
        }

        if (!topic_data_zero)
        {
            // add the new member
            // reverse key 
            ASSERT(state_set(topic_data + 12, 20, &n, 1) == 20);
            
            // forward key
            ASSERT(state_set(&n, 1, topic_data + 12, 20) == 1);
        }

        DONE("admin.c: Action member change.");
    }
    }

    accept(SBUF("admin.c: fallthrough."), __LINE__);
}