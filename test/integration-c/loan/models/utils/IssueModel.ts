import {
  BaseModel,
  Metadata,
  XRPAddress,
  Currency,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class IssueModel extends BaseModel {
  issuer: XRPAddress
  currency: Currency

  // 48 bytes
  constructor(
    issuer: XRPAddress, // 20 byte / 0
    currency: Currency // 20 byte / 20
  ) {
    super()
    this.issuer = issuer
    this.currency = currency
  }

  getMetadata(): Metadata {
    return [
      { field: 'issuer', type: 'xrpAddress' },
      { field: 'currency', type: 'currency' },
    ]
  }

  toJSON() {
    return {
      issuer: this.issuer,
      currency: this.currency,
    }
  }
}
