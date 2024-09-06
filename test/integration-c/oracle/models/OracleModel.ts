import {
  BaseModel,
  Metadata,
  Currency,
  XRPAddress,
  XFL,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class OracleModel extends BaseModel {
  issuer: XRPAddress
  currency: Currency
  value: XFL

  // 48 bytes
  constructor(
    issuer: XRPAddress, // 20 byte / 0
    currency: Currency, // 20 byte / 20
    value: XFL // 8 byte / 40
  ) {
    super()
    this.issuer = issuer
    this.currency = currency
    this.value = value
  }

  getMetadata(): Metadata {
    return [
      { field: 'issuer', type: 'xrpAddress' },
      { field: 'currency', type: 'currency' },
      { field: 'value', type: 'xfl' },
    ]
  }

  toJSON() {
    return {
      issuer: this.issuer,
      currency: this.currency,
      value: this.value,
    }
  }
}
