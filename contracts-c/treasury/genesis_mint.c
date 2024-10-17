#include "hookapi.h"

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
        rollback(SBUF("Genesis Mint: Destination Account not set as Hook parameter"), 1);

    otxn_slot(1);
    slot_subfield(1, sfGenesisMints, 1); 
    slot_subarray(1, 0, 1);
    slot_subfield(1, sfGenesisMint, 1);
    
    slot_subfield(1, sfAmount, 2);
    int64_t txn_amount = slot_float(2);

    uint8_t mint_dest[20];
    slot_subfield(1, sfDestination, 1);
    slot(SBUF(mint_dest), 1);

    if (!BUFFER_EQUAL_20(ACC_OUT, mint_dest))
        rollback(SBUF("Genesis mint: Invalid Mint Destination."), 2);

    etxn_reserve(1);
        
    uint32_t fls = (uint32_t)ledger_seq() + 1;
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);

    uint32_t lls = fls + 4;
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);


    etxn_details(EMIT_OUT, 116U);

    uint64_t drops = float_int(txn_amount, 6, 1);
    uint8_t *b = AMOUNT_OUT + 1;
    *b++ = 0b01000000 + ((drops >> 56) & 0b00111111);
    *b++ = (drops >> 48) & 0xFFU;
    *b++ = (drops >> 40) & 0xFFU;
    *b++ = (drops >> 32) & 0xFFU;
    *b++ = (drops >> 24) & 0xFFU;
    *b++ = (drops >> 16) & 0xFFU;
    *b++ = (drops >> 8) & 0xFFU;
    *b++ = (drops >> 0) & 0xFFU;

    // TXN PREPARE: Fee
    {
        int64_t fee = etxn_fee_base(SBUF(txn));
        uint8_t *b = FEE_OUT;
        *b++ = 0b01000000 + ((fee >> 56) & 0b00111111);
        *b++ = (fee >> 48) & 0xFFU;
        *b++ = (fee >> 40) & 0xFFU;
        *b++ = (fee >> 32) & 0xFFU;
        *b++ = (fee >> 24) & 0xFFU;
        *b++ = (fee >> 16) & 0xFFU;
        *b++ = (fee >> 8) & 0xFFU;
        *b++ = (fee >> 0) & 0xFFU;
    }

    uint8_t emithash[32]; 
    if(emit(SBUF(emithash), SBUF(txn)) != 32)
        rollback(SBUF("Genesis Mint: Failed To Emit."), 3);  

    accept(SBUF("Genesis Mint: Passing ClaimReward."), 4);    
    _g(1,1);            
    return 0;    
}