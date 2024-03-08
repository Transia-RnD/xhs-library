import {
  BaseModel,
  Metadata,
  XFL,
  XRPAddress,
  UInt64,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class LotteryModel extends BaseModel {
  id: UInt64 // ticket price
  price: XFL // ticket price
  fee: XFL // fee
  feeAddress: XRPAddress // fee address
  maxAmount: XFL // maximum amount
  maxTickets: UInt64 // maximum tickets
  duration: UInt64 // lottery duration (seconds)

  // 68 bytes
  constructor(
    id: UInt64, // 8 bytes / 0
    price: XFL, // 8 bytes / 8
    fee: XFL, // 8 bytes / 16
    feeAddress: XRPAddress, // 20 bytes / 24
    maxAmount: XFL, // 8 bytes / 44
    maxTickets: UInt64, // 8 bytes / 52
    duration: UInt64 // 8 bytes / 60
  ) {
    super()
    this.id = id
    this.price = price
    this.fee = fee
    this.feeAddress = feeAddress
    this.maxAmount = maxAmount
    this.maxTickets = maxTickets
    this.duration = duration
  }

  getMetadata(): Metadata {
    return [
      { field: 'id', type: 'uint64' },
      { field: 'price', type: 'xfl' },
      { field: 'fee', type: 'xfl' },
      { field: 'feeAddress', type: 'xrpAddress' },
      { field: 'maxAmount', type: 'xfl' },
      { field: 'maxTickets', type: 'uint64' },
      { field: 'duration', type: 'uint64' },
    ]
  }

  toJSON() {
    return {
      id: this.id,
      price: this.price,
      fee: this.fee,
      feeAddress: this.feeAddress,
      maxAmount: this.maxAmount,
      maxTickets: this.maxTickets,
      duration: this.duration,
    }
  }
}
