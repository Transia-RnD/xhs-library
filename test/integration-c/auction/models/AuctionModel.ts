import {
  BaseModel,
  Hash256,
  Metadata,
  UInt32,
  UInt64,
  XFL,
  XRPAddress,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class AuctionModel extends BaseModel {
  startTime: UInt32 // start ledger of the auction
  endTime: UInt32 // end ledger of the auction
  minBid: XFL // starting bid price
  numBids: UInt64 // number of bids
  winBid: XFL // winning bid
  winAddress: XRPAddress // winning address
  winId: Hash256 // hash id

  // 84 bytes
  constructor(
    startTime: UInt32, // 4 bytes
    endTime: UInt32, // 4 bytes
    minBid: XFL, // 8 bytes
    numBids: UInt64, // 8 bytes
    winBid: XFL, // 8 bytes
    winAddress: XRPAddress, // 20 bytes
    winId: Hash256 // 32 bytes
  ) {
    super()
    this.startTime = startTime
    this.endTime = endTime
    this.minBid = minBid
    this.numBids = numBids
    this.winBid = winBid
    this.winAddress = winAddress
    this.winId = winId
  }

  getMetadata(): Metadata {
    return [
      { field: 'startTime', type: 'uint32' },
      { field: 'endTime', type: 'uint32' },
      { field: 'minBid', type: 'xfl' },
      { field: 'numBids', type: 'uint64' },
      { field: 'winBid', type: 'xfl' },
      { field: 'winAddress', type: 'xrpAddress' },
      { field: 'winId', type: 'hash256' },
    ]
  }

  toJSON() {
    return {
      startTime: this.startTime,
      endTime: this.endTime,
      minBid: this.minBid,
      numBids: this.numBids,
      winBid: this.winBid,
      winAddress: this.winAddress,
      winId: this.winId,
    }
  }
}
