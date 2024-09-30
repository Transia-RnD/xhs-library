// xrpl
import {
  Invoke,
  Payment,
  SetHookFlags,
  TransactionMetadata,
  Wallet,
} from '@transia/xrpl'
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
  Xrpld,
  SetHookParams,
  setHooksV3,
  createHookPayload,
  ExecutionUtility,
  // generateHash,
  // StateUtility,
  // hexNamespace,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  // clearHookStateV3,
  // clearAllHooksV3,
  // iHook,
} from '@transia/hooks-toolkit/dist/npm/src'
import {
  // hexToXfl,
  xflToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'

// Router: ACCEPT: success

export async function invoke(
  testContext: XrplIntegrationTestContext,
  issuerWallet: Wallet
): Promise<void> {
  // Invoke - Create the limit
  const param1 = new iHookParamEntry(
    new iHookParamName('MA'),
    new iHookParamValue(xflToHex(100), true)
  )
  const builtTx1: Invoke = {
    TransactionType: 'Invoke',
    Account: issuerWallet.classicAddress,
    HookParameters: [param1.toXrpl()],
  }

  const result1 = await Xrpld.submit(testContext.client, {
    wallet: issuerWallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    testContext.client,
    result1.meta as TransactionMetadata
  )
  expect(hookExecutions1.executions[0].HookReturnString).toMatch(
    'limitedIssue.js: Max Amount Added.'
  )
}

export async function payment(
  testContext: XrplIntegrationTestContext,
  issuerWallet: Wallet,
  bobWallet: Wallet
): Promise<void> {
  // Payment - Issue the amount
  const amount: IssuedCurrencyAmount = {
    value: '100',
    currency: 'ABC',
    issuer: testContext.gw.classicAddress,
  }
  const builtTx2: Payment = {
    TransactionType: 'Payment',
    Account: issuerWallet.classicAddress,
    Destination: bobWallet.classicAddress,
    Amount: amount,
  }

  const result2 = await Xrpld.submit(testContext.client, {
    wallet: issuerWallet,
    tx: builtTx2,
  })
  const hookExecutions2 = await ExecutionUtility.getHookExecutionsFromMeta(
    testContext.client,
    result2.meta as TransactionMetadata
  )
  expect(hookExecutions2.executions[0].HookReturnString).toMatch(
    'limitedIssue.c: Updated.'
  )
}

describe('limitedIssue', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const issuerWallet = testContext.gw
    const bobWallet = testContext.bob

    const USD = IC.gw('ABC', issuerWallet.classicAddress)
    await trust(testContext.client, USD.set(100000), ...[bobWallet])

    const acct1hook1 = createHookPayload({
      version: 1,
      createFile: 'limitedIssue',
      namespace: 'limitedIssue',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
      fee: '1000000',
    })
    await setHooksV3({
      client: testContext.client,
      seed: issuerWallet.seed,
      hooks: [{ Hook: acct1hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    // const clearHook = {
    //   Flags: SetHookFlags.hsfNSDelete,
    //   HookNamespace: hexNamespace('limitedIssue'),
    // } as iHook
    // await clearHookStateV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    //   hooks: [{ Hook: clearHook }, { Hook: clearHook }],
    // } as SetHookParams)

    // await clearAllHooksV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    // } as SetHookParams)
    teardownClient(testContext)
  })

  it('limit issue - success', async () => {
    const issuerWallet = testContext.gw
    const bobWallet = testContext.bob

    // // Invoke - Create the limit
    // await invoke(testContext, issuerWallet)

    // // Payment - Issue the amount
    // await payment(testContext, issuerWallet, bobWallet)

    await payment(testContext, issuerWallet, bobWallet)
  })
})
