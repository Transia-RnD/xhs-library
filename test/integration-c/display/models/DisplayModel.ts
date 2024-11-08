import {
  BaseModel,
  Hash256,
  Metadata,
  UInt64,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class DisplayModel extends BaseModel {
  id: Hash256
  lastLedger: UInt64

  constructor(id: Hash256, lastLedger: UInt64) {
    super()
    this.id = id
    this.lastLedger = lastLedger
  }

  getMetadata(): Metadata {
    return [
      { field: 'id', type: 'hash256' },
      { field: 'lastLedger', type: 'uint64' },
    ]
  }

  toJSON() {
    return {
      id: this.id,
      lastLedger: this.lastLedger,
    }
  }
}
