import {
  BaseModel,
  Metadata,
  XFL,
  XRPAddress,
  UInt32,
  PublicKey,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class FirewallModel extends BaseModel {
  publicKey: PublicKey
  backupAccount: XRPAddress
  timePeriod: UInt32
  startTime: UInt32
  amount: XFL
  totalOut: XFL

  // 77 bytes
  constructor(
    publicKey: PublicKey, // 33 bytes / 0
    backupAccount: XRPAddress, // 20 bytes / 33
    timePeriod: UInt32, // 4 bytes / 53
    startTime: UInt32, // 4 bytes / 57
    amount: XFL, // 8 bytes / 61
    totalOut: XFL // 8 bytes / 69
  ) {
    super()
    this.publicKey = publicKey
    this.backupAccount = backupAccount
    this.timePeriod = timePeriod
    this.startTime = startTime
    this.amount = amount
    this.totalOut = totalOut
  }

  getMetadata(): Metadata {
    return [
      { field: 'publicKey', type: 'publicKey' },
      { field: 'backupAccount', type: 'xrpAddress' },
      { field: 'timePeriod', type: 'uint32' },
      { field: 'startTime', type: 'uint32' },
      { field: 'amount', type: 'xfl' },
      { field: 'totalOut', type: 'xfl' },
    ]
  }

  toJSON() {
    return {
      publicKey: this.publicKey,
      backupAccount: this.backupAccount,
      timePeriod: this.timePeriod,
      startTime: this.startTime,
      amount: this.amount,
      totalOut: this.totalOut,
    }
  }
}
