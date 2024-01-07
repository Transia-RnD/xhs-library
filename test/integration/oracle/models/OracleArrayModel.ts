import {
  BaseModel,
  Metadata,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { OracleModel } from './OracleModel'

export class OracleArrayModel extends BaseModel {
  oracles: OracleModel[]

  constructor(oracles: OracleModel[]) {
    super()
    this.oracles = oracles
  }

  getMetadata(): Metadata {
    return [
      {
        field: 'oracles',
        type: 'varModelArray',
        modelClass: OracleModel,
        maxArrayLength: 100,
      },
    ]
  }

  toJSON() {
    return {
      oracles: this.oracles,
    }
  }
}
