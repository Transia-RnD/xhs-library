import {
  BaseModel,
  Metadata,
  UInt32,
  XFL,
  XRPAddress,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class LotteryModel extends BaseModel {
  startTime: UInt32 // start ledger of the auction
  endTime: UInt32 // end ledger of the auction
  price: XFL // ticket price
  fee: XFL // fee
  feeAddress: XRPAddress // fee address
  charity: XFL // charity
  charityAddress: XRPAddress // charity address

  // 82 bytes
  constructor(
    startTime: UInt32, // 4 bytes / 0
    endTime: UInt32, // 4 bytes / 4
    price: XFL, // 8 bytes / 12
    fee: XFL, // 8 bytes / 20
    feeAddress: XRPAddress, // 20 bytes / 40
    charity: XFL, // 8 bytes / 48
    charityAddress: XRPAddress // 20 bytes / 68
  ) {
    super()
    this.startTime = startTime
    this.endTime = endTime
    this.price = price
    this.fee = fee
    this.feeAddress = feeAddress
    this.charity = charity
    this.charityAddress = charityAddress
  }

  getMetadata(): Metadata {
    return [
      { field: 'startTime', type: 'uint32' },
      { field: 'endTime', type: 'uint32' },
      { field: 'price', type: 'xfl' },
      { field: 'fee', type: 'xfl' },
      { field: 'feeAddress', type: 'xrpAddress' },
      { field: 'charity', type: 'xfl' },
      { field: 'charityAddress', type: 'xrpAddress' },
    ]
  }

  toJSON() {
    return {
      startTime: this.startTime,
      endTime: this.endTime,
      price: this.price,
      fee: this.fee,
      feeAddress: this.feeAddress,
      charity: this.charity,
      charityAddress: this.charityAddress,
    }
  }
}
