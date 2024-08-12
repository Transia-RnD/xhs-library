// xrpl
import {
  Client,
  Wallet,
  Invoke,
  Payment,
  SetHookFlags,
  TransactionMetadata,
  // unixTimeToRippleTime,
} from '@transia/xrpl'
// xrpl-helpers
import {
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  serverUrl,
  IC,
  trust,
  close,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
// src
import {
  Xrpld,
  SetHookParams,
  setHooksV3,
  createHookPayload,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  ExecutionUtility,
  StateUtility,
  xrpAddressToHex,
  padHexString,
  hexNamespace,
  decodeModel,
  // iHook,
  // clearHookStateV3,
  // clearAllHooksV3,
} from '@transia/hooks-toolkit/dist/npm/src'
import {
  // AccountModel,
  Fees,
  // Permission,
  // Permissions,
  PoolDelegateCover,
  // PoolDetails,
  PoolModel,
  Withdraw,
} from './models/Pool'
import { AmountModel } from './models/utils/AmountModel'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { IssueModel } from './models/utils/IssueModel'
import {
  // Composition,
  LoanDetails,
  LoanFees,
  LoanModel,
  Rates,
} from './models/Loan'

export async function getPool(
  testContext: XrplIntegrationTestContext,
  hookWallet: Wallet
) {
  const state = await StateUtility.getHookState(
    testContext.client,
    hookWallet.classicAddress,
    padHexString(xrpAddressToHex(hookWallet.classicAddress)),
    hexNamespace('pool')
  )
  const decoded = decodeModel(state.HookStateData, PoolModel)
  console.log(decoded)
}

export async function getLoan(
  testContext: XrplIntegrationTestContext,
  hookWallet: Wallet
) {
  const state = await StateUtility.getHookState(
    testContext.client,
    hookWallet.classicAddress,
    padHexString(xrpAddressToHex(hookWallet.classicAddress)),
    hexNamespace('loan')
  )
  const decoded = decodeModel(state.HookStateData, LoanModel)
  console.log(decoded)
}

export async function createPool(
  client: Client,
  wallet: Wallet,
  issuer: Wallet,
  lender: Wallet,
  borrower: Wallet,
  withdrawAmount: number,
  managementFee: number,
  coverMin: number,
  coverMax: number
) {
  // const lenders = new Permission(1, [new AccountModel(lender.classicAddress)])
  // const borrowers = new Permission(1, [
  //   new AccountModel(borrower.classicAddress),
  // ])
  // const poolPermissions = new Permissions(lenders, borrowers)
  // const poolDetails = new PoolDetails(poolPermissions)
  const poolWithdraw = new Withdraw(
    0,
    new AmountModel(withdrawAmount, 'USD', issuer.classicAddress),
    new Fees(managementFee)
  )
  const issue = new IssueModel(issuer.classicAddress, 'USD')
  const cover = new PoolDelegateCover(coverMin, coverMax)
  const poolModel = new PoolModel(0, issue, poolWithdraw, cover)
  console.log(poolModel.encode().length / 2)

  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01010000000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('P')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('C')
  )
  const otxn1param4 = new iHookParamEntry(
    new iHookParamName('PM'),
    new iHookParamValue(poolModel.encode().toUpperCase(), true)
  )
  const builtTx1: Invoke = {
    TransactionType: 'Invoke',
    Account: wallet.classicAddress,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
      otxn1param4.toXrpl(),
    ],
  }

  console.log(JSON.stringify([otxn1param1.toXrpl(), otxn1param2.toXrpl()]))
  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'pool.c: Created Pool.'
  )
}

export async function createLoan(
  client: Client,
  wallet: Wallet,
  issuer: Wallet,
  lender: Wallet,
  borrower: Wallet,
  originationFee: number,
  serviceFee: number,
  interestRate: number,
  closingFee: number,
  lateFee: number,
  rateAdjust: number,
  gracePeriod: number, // 4
  paymentInterval: number, // 4
  loanStartDate: number, // 4
  nextPaymentDueDate: number, // 4
  lastPaymentDate: number, // 4
  totalPayments: number, // 2
  paymentsRemaining: number, // 2
  intervalAmount: number, // 8
  principalRequested: number, // 8
  endingPrincipal: number, // 8
  drawableFunds: number // 8
) {
  const loanModel = new LoanModel(
    0,
    // 0,
    borrower.classicAddress,
    // lender.classicAddress,
    // '27CAC5503836765CD10751D27AB4A6E17D7A80D4C948430A5A81513973F9B51E',
    0,
    new LoanFees(originationFee, serviceFee),
    new Rates(interestRate, closingFee, lateFee, rateAdjust),
    new LoanDetails(
      gracePeriod,
      paymentInterval,
      loanStartDate,
      nextPaymentDueDate,
      lastPaymentDate,
      totalPayments,
      paymentsRemaining,
      intervalAmount,
      principalRequested,
      endingPrincipal,
      drawableFunds
    )
  )
  console.log(decodeModel(loanModel.encode(), LoanModel))
  console.log(loanModel.encode().length / 2)

  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01000100000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('L')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('C')
  )
  const otxn1param4 = new iHookParamEntry(
    new iHookParamName('LM'),
    new iHookParamValue(loanModel.encode().toUpperCase(), true)
  )
  const builtTx1: Invoke = {
    TransactionType: 'Invoke',
    Account: wallet.classicAddress,
    Blob: loanModel.encode().toUpperCase(),
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
      otxn1param4.toXrpl(),
    ],
  }

  console.log(JSON.stringify([otxn1param1.toXrpl(), otxn1param2.toXrpl()]))
  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'loan.c: Created Loan.'
  )
}

export async function depositLiquidity(
  client: Client,
  wallet: Wallet,
  hook: string,
  amount: IssuedCurrencyAmount
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01010000000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('L')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('D')
  )
  const builtTx1: Payment = {
    TransactionType: 'Payment',
    Account: wallet.classicAddress,
    Destination: hook,
    Amount: amount,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
    ],
  }

  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'pool.c: Transaction Complete.'
  )

  await close(client)
}

export async function depositCollateral(
  client: Client,
  wallet: Wallet,
  hook: string,
  amount: IssuedCurrencyAmount
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01000100000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('C')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('D')
  )
  const builtTx1: Payment = {
    TransactionType: 'Payment',
    Account: wallet.classicAddress,
    Destination: hook,
    Amount: amount,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
    ],
  }

  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'loan.c: Deposited Collateral.'
  )

  await close(client)
}

export async function drawDown(
  client: Client,
  wallet: Wallet,
  hook: string,
  amount: IssuedCurrencyAmount
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01000100000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('L')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('R')
  )
  const otxn1param4 = new iHookParamEntry(
    new iHookParamName('AMT'),
    new iHookParamValue(
      new AmountModel(Number(amount.value), amount.currency, amount.issuer)
        .encode()
        .toUpperCase(),
      true
    )
  )
  const builtTx1: Invoke = {
    TransactionType: 'Invoke',
    Account: wallet.classicAddress,
    Destination: hook,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
      otxn1param4.toXrpl(),
    ],
  }

  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'loan.c: Transaction Success (Receive Loan).'
  )

  await close(client)
}

export async function makePayment(
  client: Client,
  wallet: Wallet,
  hook: string,
  amount: IssuedCurrencyAmount
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01000100000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('L')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('P')
  )
  const builtTx1: Payment = {
    TransactionType: 'Payment',
    Account: wallet.classicAddress,
    Destination: hook,
    Amount: amount,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
    ],
  }

  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'loan.c: Payback Loan.'
  )

  await close(client)
}

export async function createPoolAndDepositLiquidity(
  testContext: XrplIntegrationTestContext,
  hookWallet: Wallet,
  lenderWallet: Wallet,
  borrowWallet: Wallet,
  withdrawAmount: number,
  managementFee: number,
  minCover: number,
  maxCover: number,
  depositAmount: number
) {
  await createPool(
    testContext.client,
    hookWallet,
    testContext.gw,
    lenderWallet,
    borrowWallet,
    withdrawAmount,
    managementFee,
    minCover,
    maxCover
  )

  // Payment - Deposit liquidity
  const amount: IssuedCurrencyAmount = {
    value: String(depositAmount),
    currency: 'USD',
    issuer: testContext.gw.classicAddress,
  }
  await depositLiquidity(
    testContext.client,
    lenderWallet,
    hookWallet.classicAddress,
    amount
  )
}

describe('loanPool', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)

    const USD = IC.gw('USD', testContext.gw.classicAddress)
    await trust(testContext.client, USD.set(100000), ...[testContext.hook1])

    const hookWallet = testContext.hook1
    const acct1hook1 = createHookPayload({
      version: 0,
      createFile: 'router_base',
      namespace: 'router',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
    })
    const acct1hook2 = createHookPayload({
      version: 0,
      createFile: 'pool',
      namespace: 'pool',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
    })
    const acct1hook3 = createHookPayload({
      version: 0,
      createFile: 'loan',
      namespace: 'loan',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: acct1hook2 }, { Hook: acct1hook3 }],
    } as SetHookParams)
    // console.log(hexNamespace('pool'))
    // console.log(hexNamespace('loan'))

    // console.log(
    //   JSON.stringify([
    //     { Hook: acct1hook1 },
    //     { Hook: acct1hook2 },
    //     { Hook: acct1hook3 },
    //     { Hook: acct1hook4 },
    //   ])
    // )
  })
  afterAll(async () => {
    // await clearAllHooksV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    // } as SetHookParams)

    // const clearHook1 = {
    //   Flags: SetHookFlags.hsfNSDelete,
    //   HookNamespace: hexNamespace('pool'),
    // } as iHook
    // const clearHook2 = {
    //   Flags: SetHookFlags.hsfNSDelete,
    //   HookNamespace: hexNamespace('loan'),
    // } as iHook
    // const clearHook3 = {
    //   Flags: SetHookFlags.hsfNSDelete,
    //   HookNamespace: hexNamespace('collateral'),
    // } as iHook
    // const clearHook4 = {
    //   Flags: SetHookFlags.hsfNSDelete,
    //   HookNamespace: hexNamespace('liquidity'),
    // } as iHook
    // await clearHookStateV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    //   hooks: [
    //     { Hook: clearHook1 },
    //     { Hook: clearHook2 },
    //     { Hook: clearHook3 },
    //     { Hook: clearHook4 },
    //   ],
    // } as SetHookParams)
    teardownClient(testContext)
  })

  // it('pool - success', async () => {
  //   // Invoke - Create the pool
  //   const hookWallet = testContext.hook1
  //   const lenderWallet = testContext.alice
  //   const borrowWallet = testContext.bob
  //   const withdrawAmount = 10
  //   const managementFee = 100
  //   const minCover = 0.1
  //   const maxCover = 0.5
  //   await createPool(
  //     testContext.client,
  //     hookWallet,
  //     testContext.gw,
  //     lenderWallet,
  //     borrowWallet,
  //     withdrawAmount,
  //     managementFee,
  //     minCover,
  //     maxCover
  //   )

  //   // Payment - Deposit liquidity
  //   const amount: IssuedCurrencyAmount = {
  //     value: '100',
  //     currency: 'USD',
  //     issuer: testContext.gw.classicAddress,
  //   }
  //   await depositLiquidity(
  //     testContext.client,
  //     lenderWallet,
  //     hookWallet.classicAddress,
  //     amount
  //   )
  // })

  it('loan - success', async () => {
    // const hookWallet = testContext.hook1
    // const lenderWallet = testContext.alice
    // const borrowWallet = testContext.bob

    // // Pool Settings
    // const withdrawAmount = 10
    // const managementFee = 100
    // const minCover = 0.1
    // const maxCover = 0.5
    // const initDeposit = 1000
    // // // Loan Settings
    // const originationFee = 100
    // const serviceFee = 100
    // const interestRate = 0.1
    // const closingFee = 0.1
    // const lateFee = 10
    // const rateAdjust = 0
    // const gracePeriod = 86400
    // const paymentInterval = 1
    // const loanStartDate = unixTimeToRippleTime(Date.now())
    // const nextPaymentDueDate = unixTimeToRippleTime(Date.now()) + 300
    // const lastPaymentDate = 0
    // const totalPayments = 12
    // const paymentsRemaining = 12
    // const intervalAmount = 8.79
    // const principalAmount = 100
    // const endingAmount = 0
    // const drawableAmount = 100
    // // Collateral Settings
    // const collateralAmount = principalAmount * interestRate

    // await createPoolAndDepositLiquidity(
    //   testContext,
    //   hookWallet,
    //   lenderWallet,
    //   borrowWallet,
    //   withdrawAmount,
    //   managementFee,
    //   minCover,
    //   maxCover,
    //   initDeposit
    // )
    // await getPool(testContext, hookWallet)

    // // Payment - Deposit collateral
    // const amount: IssuedCurrencyAmount = {
    //   value: String(collateralAmount),
    //   currency: 'USD',
    //   issuer: testContext.gw.classicAddress,
    // }
    // await depositCollateral(
    //   testContext.client,
    //   borrowWallet,
    //   hookWallet.classicAddress,
    //   amount
    // )

    // await createLoan(
    //   testContext.client,
    //   testContext.hook1,
    //   testContext.gw,
    //   testContext.alice,
    //   testContext.bob,
    //   originationFee,
    //   serviceFee,
    //   interestRate,
    //   closingFee,
    //   lateFee,
    //   rateAdjust,
    //   gracePeriod,
    //   paymentInterval,
    //   loanStartDate,
    //   nextPaymentDueDate,
    //   lastPaymentDate,
    //   totalPayments,
    //   paymentsRemaining,
    //   intervalAmount,
    //   principalAmount,
    //   endingAmount,
    //   drawableAmount
    // )

    // await getLoan(testContext, hookWallet)

    // // Drawdown the loan
    // await drawDown(
    //   testContext.client,
    //   testContext.bob,
    //   testContext.hook1.classicAddress,
    //   {
    //     value: String(principalAmount),
    //     currency: 'USD',
    //     issuer: testContext.gw.classicAddress,
    //   } as IssuedCurrencyAmount
    // )

    // // Make a payment
    const intervalAmount = 8.79
    await makePayment(
      testContext.client,
      testContext.bob,
      testContext.hook1.classicAddress,
      {
        value: String(intervalAmount),
        currency: 'USD',
        issuer: testContext.gw.classicAddress,
      } as IssuedCurrencyAmount
    )

    // simulate time passing
    // await new Promise((resolve) => setTimeout(resolve, 5000))
    // // Make a payment
    // const intervalAmount = 8.79
    // await makePayment(
    //   testContext.client,
    //   testContext.bob,
    //   testContext.hook1.classicAddress,
    //   {
    //     value: String(intervalAmount),
    //     currency: 'USD',
    //     issuer: testContext.gw.classicAddress,
    //   } as IssuedCurrencyAmount
    // )
  })
})
