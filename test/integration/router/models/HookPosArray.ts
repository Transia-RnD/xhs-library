import {
  BaseModel,
  Metadata,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { HookPosModel } from './HookPosModel'

export const HookPosArray = class extends BaseModel {
  nestedModelArray: HookPosModel[]

  constructor(nestedModelArray: HookPosModel[]) {
    super()
    this.nestedModelArray = nestedModelArray
  }

  getMetadata(): Metadata {
    return [
      {
        field: 'nestedModelArray',
        type: 'varModelArray',
        modelClass: HookPosModel,
        maxArrayLength: 10,
      },
    ]
  }
}
