import {
  BaseModel,
  Metadata,
  UInt8,
  XRPAddress,
  Hash256,
  XFL,
  UInt32,
  UInt16,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class LoanModel extends BaseModel {
  isType: UInt8 //
  // nonce: UInt32 //
  borrower: XRPAddress //
  lender: XRPAddress //
  poolId: Hash256 //
  state: UInt8 //
  fees: LoanFees //
  rates: Rates //
  details: LoanDetails //
  // composition: Composition[] //

  // ?? bytes
  constructor(
    isType: UInt8, // 8 bytes / 0
    // nonce: UInt32, // ?? bytes / 8
    borrower: XRPAddress, // ?? bytes / 8
    // lender: XRPAddress, // ?? bytes / 8
    // poolId: Hash256, // ?? bytes / 8
    state: UInt8, // ?? bytes / 8
    fees: LoanFees, // ?? bytes / 8
    rates: Rates, // ?? bytes / 8
    details: LoanDetails // ?? bytes / 8
    // composition: Composition[] // ?? bytes / 8
  ) {
    super()
    this.isType = isType
    // this.nonce = nonce
    this.borrower = borrower
    // this.lender = lender
    // this.poolId = poolId
    this.state = state
    this.fees = fees
    this.rates = rates
    this.details = details
    // this.composition = composition
  }

  getMetadata(): Metadata {
    return [
      { field: 'isType', type: 'uint8' },
      // { field: 'nonce', type: 'uint32' },
      { field: 'borrower', type: 'xrpAddress' },
      // { field: 'lender', type: 'xrpAddress' },
      // { field: 'poolId', type: 'hash256' },
      { field: 'state', type: 'uint8' },
      { field: 'fees', type: 'model', modelClass: LoanFees },
      { field: 'rates', type: 'model', modelClass: Rates },
      { field: 'details', type: 'model', modelClass: LoanDetails },
      // {
      //   field: 'composition',
      //   type: 'varModelArray',
      //   modelClass: Composition,
      //   maxArrayLength: 8,
      // },
    ]
  }

  toJSON() {
    return {
      isType: this.isType,
      // nonce: this.nonce,
      borrower: this.borrower,
      // lender: this.lender,
      // poolId: this.poolId,
      state: this.state,
      fees: this.fees,
      rates: this.rates,
      details: this.details,
      // composition: this.composition,
    }
  }
}

export class LoanFees extends BaseModel {
  originationFee: XFL // originationFee
  serviceFee: XFL // serviceFee

  // ?? bytes
  constructor(
    originationFee: XFL, // ?? bytes / 0
    serviceFee: XFL // ?? bytes / 0
  ) {
    super()
    this.originationFee = originationFee
    this.serviceFee = serviceFee
  }

  getMetadata(): Metadata {
    return [
      { field: 'originationFee', type: 'xfl' },
      { field: 'serviceFee', type: 'xfl' },
    ]
  }

  toJSON() {
    return {
      originationFee: this.originationFee,
      serviceFee: this.serviceFee,
    }
  }
}

export class Rates extends BaseModel {
  interestRate: XFL // interest
  closingFee: XFL // closingFee
  lateFee: XFL // lateFee
  rateAdjust: XFL // rateAdjust

  // ?? bytes
  constructor(
    interestRate: XFL, // ?? bytes / 0
    closingFee: XFL, // ?? bytes / 0
    lateFee: XFL, // ?? bytes / 0
    rateAdjust: XFL // ?? bytes / 0
  ) {
    super()
    this.interestRate = interestRate
    this.closingFee = closingFee
    this.lateFee = lateFee
    this.rateAdjust = rateAdjust
  }

  getMetadata(): Metadata {
    return [
      { field: 'interestRate', type: 'xfl' },
      { field: 'closingFee', type: 'xfl' },
      { field: 'lateFee', type: 'xfl' },
      { field: 'rateAdjust', type: 'xfl' },
    ]
  }

  toJSON() {
    return {
      interestRate: this.interestRate,
      closingFee: this.closingFee,
      lateFee: this.lateFee,
      rateAdjust: this.rateAdjust,
    }
  }
}

export class LoanDetails extends BaseModel {
  gracePeriod: UInt32 // GracePeriod
  paymentInterval: UInt32 // PaymentInterval
  loanStartDate: UInt32 // LoanStartDate
  nextPaymentDueDate: UInt32 // NextPaymentDueDate
  lastPaymentDate: UInt32 // LastPaymentDate
  totalPayments: UInt16 // TotalPayments
  paymentsRemaining: UInt16 // PaymentsRemaining
  intervalAmount: XFL // IntervalAmount
  principalRequested: XFL // PrincipalRequested
  endingPrincipal: XFL // EndingPrincipal
  drawableFunds: XFL // DrawableFunds

  constructor(
    gracePeriod: UInt32, // GracePeriod
    paymentInterval: UInt32, // PaymentInterval
    loanStartDate: UInt32, // LoanStartDate
    nextPaymentDueDate: UInt32, // NextPaymentDueDate
    lastPaymentDate: UInt32, // LastPaymentDate
    totalPayments: UInt16, // TotalPayments
    paymentsRemaining: UInt16, // PaymentsRemaining
    intervalAmount: XFL, // IntervalAmount
    principalRequested: XFL, // PrincipalRequested
    endingPrincipal: XFL, // EndingPrincipal
    drawableFunds: XFL // DrawableFunds
  ) {
    super()
    this.gracePeriod = gracePeriod
    this.paymentInterval = paymentInterval
    this.loanStartDate = loanStartDate
    this.nextPaymentDueDate = nextPaymentDueDate
    this.lastPaymentDate = lastPaymentDate
    this.totalPayments = totalPayments
    this.paymentsRemaining = paymentsRemaining
    this.intervalAmount = intervalAmount
    this.principalRequested = principalRequested
    this.endingPrincipal = endingPrincipal
    this.drawableFunds = drawableFunds
  }

  getMetadata(): Metadata {
    return [
      { field: 'gracePeriod', type: 'uint32' },
      { field: 'paymentInterval', type: 'uint32' },
      { field: 'loanStartDate', type: 'uint32' },
      { field: 'nextPaymentDueDate', type: 'uint32' },
      { field: 'lastPaymentDate', type: 'uint32' },
      { field: 'totalPayments', type: 'uint16' },
      { field: 'paymentsRemaining', type: 'uint16' },
      { field: 'intervalAmount', type: 'xfl' },
      { field: 'principalRequested', type: 'xfl' },
      { field: 'endingPrincipal', type: 'xfl' },
      { field: 'drawableFunds', type: 'xfl' },
    ]
  }

  toJSON() {
    return {
      gracePeriod: this.gracePeriod,
      paymentInterval: this.paymentInterval,
      loanStartDate: this.loanStartDate,
      nextPaymentDueDate: this.nextPaymentDueDate,
      lastPaymentDate: this.lastPaymentDate,
      totalPayments: this.totalPayments,
      paymentsRemaining: this.paymentsRemaining,
      intervalAmount: this.intervalAmount,
      principalRequested: this.principalRequested,
      endingPrincipal: this.endingPrincipal,
      drawableFunds: this.drawableFunds,
    }
  }
}

export class Composition extends BaseModel {
  account: XRPAddress
  contribution: XFL

  // 20 bytes
  constructor(
    account: XRPAddress, // 20 byte / 0
    contribution: XFL // 20 byte / 0
  ) {
    super()
    this.account = account
    this.contribution = contribution
  }

  getMetadata(): Metadata {
    return [
      { field: 'account', type: 'xrpAddress' },
      { field: 'contribution', type: 'xfl' },
    ]
  }

  toJSON() {
    return {
      account: this.account,
      contribution: this.contribution,
    }
  }
}
