import {
  BaseModel,
  Metadata,
  UInt8,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class HookPosModel extends BaseModel {
  position: UInt8 // position of the hook

  constructor(position: UInt8) {
    super()
    this.position = position
  }

  getMetadata(): Metadata {
    return [{ field: 'position', type: 'uint8' }]
  }

  toJSON() {
    return {
      position: this.position,
    }
  }
}
