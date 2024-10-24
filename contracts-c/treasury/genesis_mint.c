#include "hookapi.h"

#define DONE(x)\
    accept(SVAR(x),(uint32_t)__LINE__);

#define NOPE(x)\
{\
    return rollback((x), sizeof(x), __LINE__);\
}

#define BE_DROPS(drops)\
{\
        uint64_t drops_tmp = drops;\
        uint8_t* b = (uint8_t*)&drops;\
        *b++ = 0b01000000 + (( drops_tmp >> 56 ) & 0b00111111 );\
        *b++ = (drops_tmp >> 48) & 0xFFU;\
        *b++ = (drops_tmp >> 40) & 0xFFU;\
        *b++ = (drops_tmp >> 32) & 0xFFU;\
        *b++ = (drops_tmp >> 24) & 0xFFU;\
        *b++ = (drops_tmp >> 16) & 0xFFU;\
        *b++ = (drops_tmp >>  8) & 0xFFU;\
        *b++ = (drops_tmp >>  0) & 0xFFU;\
}

#define sfGenesisMints ((15U << 16U) + 96U)
#define sfGenesisMint ((14U << 16U) + 96U)

uint8_t txn[238] =
{
/* size,upto */
/* 3,   0,   tt = Payment           */   0x12U, 0x00U, 0x00U,
/* 5,   3,   flags                  */   0x22U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 5,   8,   sequence               */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 6,   13,  firstledgersequence    */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,
/* 6,   19,  lastledgersequence     */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,
/* 9,   25,  amount                 */   0x61U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
/* 9,   34,  fee                    */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 35,  43,  signingpubkey          */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 22,  78,  account                */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 22,  100, destination            */   0x83U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 116, 122  emit details           */ 
/* 0,   238                         */ 
};

#define FLS_OUT    (txn + 15U)
#define LLS_OUT    (txn + 21U)
#define FEE_OUT    (txn + 35U)
#define AMOUNT_OUT (txn + 25U)
#define ACC_OUT    (txn + 80U)
#define DEST_OUT   (txn + 102U)
#define EMIT_OUT   (txn + 122U)

int64_t hook(uint32_t reserved)
{
    hook_account(ACC_OUT, 20);
    if(hook_param(DEST_OUT, 20, "D", 1) != 20)
        NOPE("Genesis Mint: Destination Account not set as Hook parameter.");

    otxn_slot(1);
    slot_subfield(1, sfGenesisMints, 1);
    slot_subarray(1, 0, 1);
    slot_subfield(1, sfGenesisMint, 1);
    
    slot_subfield(1, sfAmount, 2);
    int64_t txn_amount = slot_float(2);

    uint8_t mint_dest[20];
    slot_subfield(1, sfDestination, 1);
    slot(SBUF(mint_dest), 1);

    // INTERNAL ERROR - Defensive Check
    if (!BUFFER_EQUAL_20(ACC_OUT, mint_dest))
        NOPE("Genesis mint: Invalid Mint Destination.");

    etxn_reserve(1);

    uint32_t fls = (uint32_t)ledger_seq() + 1;
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);

    uint32_t lls = fls + 4;
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);

    etxn_details(EMIT_OUT, 116U);

    uint64_t drops = float_int(txn_amount, 6, 1);
    BE_DROPS(drops);
    *((uint64_t*)(AMOUNT_OUT + 1)) = drops;

    int64_t fee = etxn_fee_base(SBUF(txn));
    BE_DROPS(fee);
    *((uint64_t*)(FEE_OUT)) = fee;

    // INTERNAL ERROR - Defensive Check
    uint8_t emithash[32];
    if(emit(SBUF(emithash), SBUF(txn)) != 32)
        NOPE("Genesis Mint: Failed To Emit.");

    DONE("Genesis Mint: Passing ClaimReward.");
    _g(1,1);
    return 0;
}