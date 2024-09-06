// xrpl
import {
  Client,
  Wallet,
  Invoke,
  Payment,
  SetHookFlags,
  TransactionMetadata,
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
  balance,
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
  Fees,
  // Permission,
  PoolDelegateCover,
  PoolModel,
  Withdraw,
} from './models/Pool'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { IssueModel } from './models/utils/IssueModel'

export async function getPool(
  testContext: XrplIntegrationTestContext,
  hookWallet: Wallet
) {
  try {
    const state = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      padHexString(xrpAddressToHex(hookWallet.classicAddress)),
      hexNamespace('pool')
    )
    const decoded = decodeModel(state.HookStateData, PoolModel)
    console.log(decoded)
    return true
  } catch (error) {
    return false
  }
}

export async function getAdminMemberCount(
  testContext: XrplIntegrationTestContext,
  hookWallet: Wallet
) {
  try {
    const state = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      padHexString(xrpAddressToHex(hookWallet.classicAddress)),
      hexNamespace('pool')
    )
    const decoded = decodeModel(state.HookStateData, PoolModel)
    console.log(decoded)
    return true
  } catch (error) {
    return false
  }
}

export async function createPool(
  client: Client,
  wallet: Wallet,
  issuer: Wallet,
  lenders: Wallet[],
  borrowers: Wallet[],
  withdrawAmount: number,
  managementFee: number,
  coverMin: number,
  coverMax: number
) {
  // const lendgerPermissions = new Permission(0, [])
  // const borrowPermissions = new Permission(0, [])
  const poolWithdraw = new Withdraw(0, withdrawAmount, new Fees(managementFee))
  const issue = new IssueModel(issuer.classicAddress, 'USD')
  const cover = new PoolDelegateCover(coverMin, coverMax)
  const poolModel = new PoolModel(0, 0, issue, poolWithdraw, cover)
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
    'pool.c: Transaction Complete (Deposit Liquidity).'
  )

  await close(client)
}

export async function withdrawLiquidity(
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
    new iHookParamValue('W')
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
    'pool.c: Transaction Complete (Withdraw Liquidity).'
  )

  await close(client)
}

export async function validateBalance(
  client: Client,
  wallet: Wallet,
  ic: IC,
  shouldBe: number
) {
  const navBalance = await balance(client, wallet.classicAddress, ic)
  expect(navBalance).toBe(shouldBe)
}

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

  it('pool - (create|deposit|withdraw)', async () => {
    // Invoke - Create the pool
    const hookWallet = testContext.hook1
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
    }

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

    const NAV = IC.gw('NAV', hookWallet.classicAddress)
    validateBalance(testContext.client, lender1Wallet, NAV, 100)
    validateBalance(testContext.client, lender2Wallet, NAV, 100)

    // Invoke - Withdraw liquidity
    const withdrawIC: IssuedCurrencyAmount = {
      value: '100',
      currency: 'NAV',
      issuer: hookWallet.classicAddress,
    }
    await withdrawLiquidity(
      testContext.client,
      lender1Wallet,
      hookWallet.classicAddress,
      withdrawIC
    )
    await close(testContext.client)
    const navBalance = await balance(
      testContext.client,
      lender1Wallet.classicAddress,
      NAV
    )
    console.log(navBalance)
    validateBalance(testContext.client, lender1Wallet, NAV, 0)
    validateBalance(testContext.client, lender2Wallet, NAV, 100)
  })
})
