import {
  BaseModel,
  Metadata,
  XRPAddress,
  VarString,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class DIDModel extends BaseModel {
  validator: XRPAddress
  did: VarString

  // 52 bytes
  constructor(
    validator: XRPAddress, // 20 byte / 0
    did: VarString // 31 byte + 1 byte / 20
  ) {
    super()
    this.validator = validator
    this.did = did
  }

  getMetadata(): Metadata {
    return [
      { field: 'validator', type: 'xrpAddress' },
      { field: 'did', type: 'varString', maxStringLength: 31 },
    ]
  }

  toJSON() {
    return {
      validator: this.validator,
      did: this.did,
    }
  }
}
