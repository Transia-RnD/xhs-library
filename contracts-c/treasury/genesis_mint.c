#include "hookapi.h"

#define DONE(x)\
    accept(SVAR(x),(uint32_t)__LINE__);

#define NOPE(x)\
{\
    return rollback((x), sizeof(x), __LINE__);\
}

#define sfGenesisMints ((15U << 16U) + 96U)
#define sfGenesisMint ((14U << 16U) + 96U)
#define ttGENESIS_MINT 96U

// clang-format off
uint8_t txn[238] =
{
/* size,upto */
/* 3,   0, tt = Payment           */   0x12U, 0x00U, 0x00U,
/* 5,   3, flags                  */   0x22U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 5,   8, sequence               */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 6,   13, firstledgersequence   */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,
/* 6,   19, lastledgersequence    */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,
/* 9,   25, amount                */   0x61U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
/* 9,   34, fee                   */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/* 35,  43, signingpubkey         */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 22,  78, account               */   0x81U, 0x14U, 0x88U, 0x59U, 0x00U, 0x4AU, 0xA9U, 0xEEU, 0xDCU, 0x00U, 0xB9U, 0xBDU, 0x5FU, 0xA0U, 0x9AU, 0xAEU, 0x43U, 0x25U, 0x52U, 0xECU, 0x84U, 0x3DU,
/* 22,  100, destination          */   0x83U, 0x14U, 0x88U, 0x58U, 0xFDU, 0x4DU, 0x78U, 0xCEU, 0x5EU, 0x97U, 0x2CU, 0x8CU, 0x1EU, 0x63U, 0xD5U, 0x1BU, 0x83U, 0x4FU, 0xF3U, 0x6FU, 0x14U, 0x11U,
/* 116, 122  emit details         */ 
/* 0,   238                       */ 
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
    _g(1,1);

    hook_account(ACC_OUT, 20);

    hook_param(DEST_OUT, 20, "DST", 3);

    int64_t tt = otxn_type();
    if (tt != ttGENESIS_MINT)
        DONE("genesis_mint.c: Pass Invalid Transaction Type.");

    otxn_slot(1);
    
    slot_subfield(1, sfGenesisMints, 1); 
    slot_subarray(1, 0, 1);
    slot_subfield(1, sfGenesisMint, 1);
    
    slot_subfield(1, sfAmount, 2);
    int64_t txn_amount = slot_float(2);

    uint8_t mint_dest[20];
    slot_subfield(1, sfDestination, 3);
    slot(SBUF(mint_dest), 3);

    if (BUFFER_EQUAL_20(DEST_OUT, mint_dest))
        NOPE("genesis_mint.c: Invalid Mint Destination.");

    // if(slot_type(2, 1) != 0)
    //     NOPE("genesis_mint.c: Invalid Issued Currency.");             

    // TXN: PREPARE: Init
    etxn_reserve(1);
        
    // TXN PREPARE: FirstLedgerSequence
    uint32_t fls = (uint32_t)ledger_seq() + 1;
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN(fls);

    // TXN PREPARE: LastLedgerSequense
    uint32_t lls = fls + 4;
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN(lls);

    // TXN PREPARE: Emit Metadata
    etxn_details(EMIT_OUT, 116U);

    // TXN PREPARE: Amount
    uint64_t drops = float_int(txn_amount, 6, 1);
    TRACEVAR(drops);
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

    TRACEHEX(txn);

    uint8_t emithash[32]; 
    if(emit(SBUF(emithash), SBUF(txn)) != 32)
        rollback(SBUF("genesis_mint.c: Failed To Emit."), 9);  

    DONE("genesis_mint.c: Passing ClaimReward.");                
    return 0;    
}