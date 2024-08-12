import {
  BaseModel,
  Metadata,
  XRPAddress,
  XFL,
  Currency,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class AmountModel extends BaseModel {
  value: XFL
  issuer: XRPAddress
  currency: Currency

  // 48 bytes
  constructor(
    value: XFL, // 8 byte / 0
    currency: Currency, // 20 byte / 20
    issuer: XRPAddress // 20 byte / 0
  ) {
    super()
    this.value = value
    this.currency = currency
    this.issuer = issuer
  }

  getMetadata(): Metadata {
    return [
      { field: 'value', type: 'xfl' },
      { field: 'currency', type: 'currency' },
      { field: 'issuer', type: 'xrpAddress' },
    ]
  }

  toJSON() {
    return {
      value: this.value,
      currency: this.currency,
      issuer: this.issuer,
    }
  }
}
