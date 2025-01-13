// xrpl
import { SetHookFlags, unixTimeToRippleTime } from '@transia/xrpl'
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
import {
  borrowerRemit,
  createP2PLoan,
  createPool,
  depositLiquidity,
  getAdminMemberCount,
  getPool,
  poolVote,
} from './utils'
import { LoanDetails, LoanFees, LoanModel, Rates } from './models/Loan'

describe('pool', () => {
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
      hookOnArray: ['Invoke', 'Payment', 'Remit'],
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

    const hook2Wallet = testContext.hook2
    const _hook2 = createHookPayload({
      version: 0,
      createFile: 'p2p_loan',
      namespace: 'p2p_loan',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: hook2Wallet.seed,
      hooks: [{ Hook: _hook2 }],
    } as SetHookParams)

    // Invoke - Create the pool
    const lender1Wallet = testContext.alice
    const lender2Wallet = testContext.bob
    const withdrawAmount = 10
    const managementFee = 100
    const minCover = 0.1
    const maxCover = 0.5
    if (!(await getPool(testContext, hookWallet))) {
      await createPool(
        testContext.client,
        hookWallet,
        testContext.gw,
        [], // lenders
        [], // borrowers
        withdrawAmount,
        managementFee,
        minCover,
        maxCover
      )
      // Payment - Deposit liquidity
      const depositIC: IssuedCurrencyAmount = {
        value: '100',
        currency: 'USD',
        issuer: testContext.gw.classicAddress,
      }
      await depositLiquidity(
        testContext.client,
        lender1Wallet,
        hookWallet.classicAddress,
        depositIC
      )
      await depositLiquidity(
        testContext.client,
        lender2Wallet,
        hookWallet.classicAddress,
        depositIC
      )
    }
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

  it('admin', async () => {
    const poolWallet = testContext.hook1
    const p2pWallet = testContext.hook2
    const borrowerWallet = testContext.carol
    const memberCount = await getAdminMemberCount(testContext, poolWallet)
    console.log(memberCount)
    // expect(memberCount).toBe(2)
    // Create Loan
    // Generate Random letter bewteen A-Z
    const random = Math.floor(Math.random() * 26)
    const letter = String.fromCharCode(65 + random)
    const url = `ipfs://QmRYA9TZ7ajm26Z1hEjaTUBDaJCRvicMfivGSgnFwusTD${letter}`

    // Loan Settings
    const originationFee = 100
    const serviceFee = 100
    const interestRate = 0.1
    const closingFee = 0.1
    const lateFee = 10
    const rateAdjust = 0
    const gracePeriod = 86400
    const paymentInterval = 1
    const loanStartDate = unixTimeToRippleTime(Date.now())
    const nextPaymentDueDate = unixTimeToRippleTime(Date.now()) + 300
    const lastPaymentDate = 0
    const totalPayments = 12
    const paymentsRemaining = 12
    const intervalAmount = 8.79
    const principalAmount = 100
    const endingAmount = 0
    const drawableAmount = 100
    const loanModel = new LoanModel(
      0,
      // 0,
      borrowerWallet.classicAddress,
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
        principalAmount,
        endingAmount,
        drawableAmount
      )
    )
    const uritokenId = await createP2PLoan(
      testContext.client,
      borrowerWallet,
      p2pWallet.classicAddress,
      url,
      loanModel
    )
    // Approve Loan (Lender/Pool)
    await poolVote(
      testContext.client,
      testContext.alice, // Lender # 1
      testContext.hook1.classicAddress,
      uritokenId
    )
    await poolVote(
      testContext.client,
      testContext.bob, // Lender # 2
      testContext.hook1.classicAddress,
      uritokenId
    )
    // Approve Loan (Borrower)
    const collateralIC: IssuedCurrencyAmount = {
      value: '100',
      currency: 'USD',
      issuer: testContext.gw.classicAddress,
    }
    await borrowerRemit(
      testContext.client,
      testContext.carol, // Borrower
      testContext.hook1.classicAddress,
      collateralIC,
      uritokenId
    )
  })
})
