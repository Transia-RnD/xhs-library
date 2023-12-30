import {
  BaseModel,
  Metadata,
  XFL,
  XRPAddress,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class LotteryModel extends BaseModel {
  price: XFL // ticket price
  fee: XFL // fee
  feeAddress: XRPAddress // fee address

  // 36 bytes
  constructor(
    price: XFL, // 8 bytes / 0
    fee: XFL, // 8 bytes / 8
    feeAddress: XRPAddress // 20 bytes / 16
  ) {
    super()
    this.price = price
    this.fee = fee
    this.feeAddress = feeAddress
  }

  getMetadata(): Metadata {
    return [
      { field: 'price', type: 'xfl' },
      { field: 'fee', type: 'xfl' },
      { field: 'feeAddress', type: 'xrpAddress' },
    ]
  }

  toJSON() {
    return {
      price: this.price,
      fee: this.fee,
      feeAddress: this.feeAddress,
    }
  }
}
