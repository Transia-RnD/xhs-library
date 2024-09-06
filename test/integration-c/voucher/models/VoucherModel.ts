import {
  BaseModel,
  Metadata,
  XFL,
  PublicKey,
  UInt32,
  Currency,
  XRPAddress,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class VoucherModel extends BaseModel {
  limit: UInt32 // number of claims
  cancelAfter: UInt32 // cannot cancel before
  finishAfter: UInt32 // cannot claim before
  amount: XFL // amount
  currency: Currency // currency
  issuer: XRPAddress // issuer
  publicKey: PublicKey // public key

  constructor(
    limit: UInt32, // 4 bytes / 0
    cancelAfter: UInt32, // 4 bytes / 4
    finishAfter: UInt32, // 4 bytes / 8
    amount: XFL, // 8 bytes / 12
    currency: Currency, // 20 bytes / 20
    issuer: XRPAddress, // 20 bytes / 40
    publicKey: PublicKey // 33 bytes / 60
  ) {
    super()
    this.limit = limit
    this.cancelAfter = cancelAfter
    this.finishAfter = finishAfter
    this.amount = amount
    this.currency = currency
    this.issuer = issuer
    this.publicKey = publicKey
  }

  getMetadata(): Metadata {
    return [
      { field: 'limit', type: 'uint32' },
      { field: 'cancelAfter', type: 'uint32' },
      { field: 'finishAfter', type: 'uint32' },
      { field: 'amount', type: 'xfl' },
      { field: 'currency', type: 'currency' },
      { field: 'issuer', type: 'xrpAddress' },
      { field: 'publicKey', type: 'publicKey' },
    ]
  }

  toJSON() {
    return {
      limit: this.limit,
      cancelAfter: this.cancelAfter,
      finishAfter: this.finishAfter,
      amount: this.amount,
      currency: this.currency,
      issuer: this.issuer,
      publicKey: this.publicKey,
    }
  }
}
