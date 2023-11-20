import {
  BaseModel,
  Metadata,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { SellerModel } from './SellerModel'

export const SellerArray = class extends BaseModel {
  nestedModelArray: SellerModel[]

  constructor(nestedModelArray: SellerModel[]) {
    super()
    this.nestedModelArray = nestedModelArray
  }

  getMetadata(): Metadata {
    return [
      {
        field: 'nestedModelArray',
        type: 'varModelArray',
        modelClass: SellerModel,
        maxArrayLength: 10,
      },
    ]
  }
}
