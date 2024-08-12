import {
  BaseModel,
  Metadata,
  UInt8,
  XFL,
  XRPAddress,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { AmountModel } from './utils/AmountModel'
import { IssueModel } from './utils/IssueModel'

export class PoolModel extends BaseModel {
  isType: UInt8 //
  issue: IssueModel //
  details: PoolDetails //
  withdraw: Withdraw //
  cover: PoolDelegateCover //

  // ?? bytes
  constructor(
    isType: UInt8, // 8 bytes / 0
    issue: IssueModel, // 40 bytes / 1
    // details: PoolDetails, // ?? bytes / 48
    withdraw: Withdraw, // ?? bytes / 8
    cover: PoolDelegateCover // ?? bytes / 8
  ) {
    super()
    this.isType = isType
    this.issue = issue
    // this.details = details
    this.withdraw = withdraw
    this.cover = cover
  }

  getMetadata(): Metadata {
    return [
      { field: 'isType', type: 'uint8' },
      { field: 'issue', type: 'model', modelClass: IssueModel },
      // { field: 'details', type: 'model', modelClass: PoolDetails },
      { field: 'withdraw', type: 'model', modelClass: Withdraw },
      { field: 'cover', type: 'model', modelClass: PoolDelegateCover },
    ]
  }

  toJSON() {
    return {
      isType: this.isType,
      issue: this.issue,
      // details: this.details,
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
  liquidityCap: AmountModel // AmountModel
  fees: Fees // AmountModel

  // 68 bytes
  constructor(
    isType: UInt8, // 8 bytes / 0
    liquidityCap: AmountModel, // 8 bytes / 0
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
      { field: 'liquidityCap', type: 'model', modelClass: AmountModel },
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

export class PoolDetails extends BaseModel {
  permissions: Permissions // permissions

  // 68 bytes
  constructor(
    permissions: Permissions // 8 bytes / 0
  ) {
    super()
    this.permissions = permissions
  }

  getMetadata(): Metadata {
    return [{ field: 'permissions', type: 'model', modelClass: Permissions }]
  }

  toJSON() {
    return {
      permissions: this.permissions,
    }
  }
}

export class Permissions extends BaseModel {
  lenders: Permission //
  borrowers: Permission //

  // ?? bytes
  constructor(
    lenders: Permission, // ?? bytes / 0
    borrowers: Permission // ?? bytes / ??
  ) {
    super()
    this.lenders = lenders
    this.borrowers = borrowers
  }

  getMetadata(): Metadata {
    return [
      { field: 'lenders', type: 'model', modelClass: Permission },
      { field: 'borrowers', type: 'model', modelClass: Permission },
    ]
  }

  toJSON() {
    return {
      lenders: this.lenders,
      borrowers: this.borrowers,
    }
  }
}

export class Permission extends BaseModel {
  isPublic: UInt8 // id
  accounts: AccountModel[] // ticket price

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
