import {
  BaseModel,
  Metadata,
  UInt32,
  UInt64,
  XFL,
  XRPAddress,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class AuctionModel extends BaseModel {
  startLedger: UInt32 // start ledger of the auction
  endLedger: UInt32 // end ledger of the auction
  minBid: XFL // starting bid price
  winBid: XFL // winning bid
  winAddress: XRPAddress // winning address
  numBids: UInt64 // number of bids

  constructor(
    startLedger: UInt32, // 4 bytes
    endLedger: UInt32, // 4 bytes
    minBid: XFL, // 8 bytes
    winBid: XFL, // 8 bytes
    winAddress: XRPAddress, // 8 bytes
    numBids: UInt64 // 8 bytes
  ) {
    super()
    this.startLedger = startLedger
    this.endLedger = endLedger
    this.minBid = minBid
    this.winBid = winBid
    this.winAddress = winAddress
    this.numBids = numBids
  }

  getMetadata(): Metadata {
    return [
      { field: 'startLedger', type: 'uint32' },
      { field: 'endLedger', type: 'uint32' },
      { field: 'minBid', type: 'xfl' },
      { field: 'winBid', type: 'xfl' },
      { field: 'winAddress', type: 'xrpAddress' },
      { field: 'numBids', type: 'uint64' },
    ]
  }

  toJSON() {
    return {
      startLedger: this.startLedger,
      endLedger: this.endLedger,
      minBid: this.minBid,
      winBid: this.winBid,
      winAddress: this.winAddress,
      numBids: this.numBids,
    }
  }
}
