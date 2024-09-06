// xrpl
import { SetHookFlags } from '@transia/xrpl'
// xrpl-helpers
import {
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  serverUrl,
  IC,
  trust,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
// src
import {
  SetHookParams,
  setHooksV3,
  createHookPayload,
  // iHook,
  // clearHookStateV3,
  // clearAllHooksV3,
} from '@transia/hooks-toolkit/dist/npm/src'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { makePayment } from './utils'

describe('loanPool', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)

    const USD = IC.gw('USD', testContext.gw.classicAddress)
    await trust(testContext.client, USD.set(100000), ...[testContext.hook1])

    const hookWallet = testContext.hook1
    const hook1 = createHookPayload({
      version: 0,
      createFile: 'router_base',
      namespace: 'router',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
    })
    const hook2 = createHookPayload({
      version: 0,
      createFile: 'pool',
      namespace: 'pool',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
    })
    const hook3 = createHookPayload({
      version: 0,
      createFile: 'loan',
      namespace: 'loan',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
    })
    const hook4 = createHookPayload({
      version: 0,
      createFile: 'admin',
      namespace: 'admin',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [
        { Hook: hook1 },
        { Hook: hook2 },
        { Hook: hook3 },
        { Hook: hook4 },
      ],
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
