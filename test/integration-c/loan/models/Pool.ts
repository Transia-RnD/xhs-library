import {
  BaseModel,
  Metadata,
  UInt8,
  XFL,
  XRPAddress,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { IssueModel } from './utils/IssueModel'

// Pool Type:
// 0 - Private: Only invited users can join
// 1 - Public: Anyone can join

// Manager Type:
// 0 - Passive: Managed by a group of users
// 1 - Active: Managed by the contributors

export class PoolModel extends BaseModel {
  poolType: UInt8 //
  managerType: UInt8 //
  issue: IssueModel //
  withdraw: Withdraw //
  cover: PoolDelegateCover //

  // ?? bytes
  constructor(
    poolType: UInt8, // 8 bytes / 0
    managerType: UInt8, // 8 bytes / 0
    issue: IssueModel, // 40 bytes / 1
    withdraw: Withdraw, // ?? bytes / 8
    cover: PoolDelegateCover // ?? bytes / 8
  ) {
    super()
    this.poolType = poolType
    this.managerType = managerType
    this.issue = issue
    this.withdraw = withdraw
    this.cover = cover
  }

  getMetadata(): Metadata {
    return [
      { field: 'poolType', type: 'uint8' },
      { field: 'managerType', type: 'uint8' },
      { field: 'issue', type: 'model', modelClass: IssueModel },
      { field: 'withdraw', type: 'model', modelClass: Withdraw },
      { field: 'cover', type: 'model', modelClass: PoolDelegateCover },
    ]
  }

  toJSON() {
    return {
      poolType: this.poolType,
      managerType: this.managerType,
      issue: this.issue,
      withdraw: this.withdraw,
      cover: this.cover,
    }
  }
}

export class PoolDelegateCover extends BaseModel {
  min: XFL
  max: XFL

  // 48 bytes
  constructor(
    min: XFL, // 20 byte / 0
    max: XFL // 20 byte / 20
  ) {
    super()
    this.min = min
    this.max = max
  }

  getMetadata(): Metadata {
    return [
      { field: 'min', type: 'xfl' },
      { field: 'max', type: 'xfl' },
    ]
  }

  toJSON() {
    return {
      min: this.min,
      max: this.max,
    }
  }
}

export class Withdraw extends BaseModel {
  isType: UInt8 // type
  liquidityCap: XFL // AmountModel
  fees: Fees // AmountModel

  // 68 bytes
  constructor(
    isType: UInt8, // 8 bytes / 0
    liquidityCap: XFL, // 8 bytes / 0
    fees: Fees // 8 bytes / 0
  ) {
    super()
    this.isType = isType
    this.liquidityCap = liquidityCap
    this.fees = fees
  }

  getMetadata(): Metadata {
    return [
      { field: 'isType', type: 'uint8' },
      { field: 'liquidityCap', type: 'xfl' },
      { field: 'fees', type: 'model', modelClass: Fees },
    ]
  }

  toJSON() {
    return {
      isType: this.isType,
      liquidityCap: this.liquidityCap,
      fees: this.fees,
    }
  }
}

export class Fees extends BaseModel {
  managementFee: XFL // managementFee

  // ?? bytes
  constructor(
    managementFee: XFL // ?? bytes / 0
  ) {
    super()
    this.managementFee = managementFee
  }

  getMetadata(): Metadata {
    return [{ field: 'managementFee', type: 'xfl' }]
  }

  toJSON() {
    return {
      managementFee: this.managementFee,
    }
  }
}

export class Permission extends BaseModel {
  isPublic: UInt8 // 0 is public, 1 is private
  accounts: AccountModel[] // array of accounts

  // ?? bytes
  constructor(
    isPublic: UInt8, // 8 bytes / 0
    accounts: AccountModel[] // ?? bytes / 8
  ) {
    super()
    this.isPublic = isPublic
    this.accounts = accounts
  }

  getMetadata(): Metadata {
    return [
      { field: 'isPublic', type: 'uint8' },
      {
        field: 'accounts',
        type: 'varModelArray',
        modelClass: AccountModel,
        maxArrayLength: 8,
      },
    ]
  }

  toJSON() {
    return {
      isPublic: this.isPublic,
      accounts: this.accounts,
    }
  }
}

export class AccountModel extends BaseModel {
  account: XRPAddress

  // 20 bytes
  constructor(
    account: XRPAddress // 20 byte / 0
  ) {
    super()
    this.account = account
  }

  getMetadata(): Metadata {
    return [{ field: 'account', type: 'xrpAddress' }]
  }

  toJSON() {
    return {
      account: this.account,
    }
  }
}
