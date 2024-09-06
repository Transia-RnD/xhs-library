// xrpl
import { Payment, SetHookFlags, TransactionMetadata } from '@transia/xrpl'
// xrpl-helpers
import {
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  serverUrl,
  trust,
  close,
  pay,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
// src
import {
  Xrpld,
  SetHookParams,
  setHooksV3,
  createHookPayload,
  ExecutionUtility,
  hexNamespace,
  clearHookStateV3,
  clearAllHooksV3,
  iHook,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
} from '@transia/hooks-toolkit'
import {
  currencyToHex,
  xrpAddressToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'

// Router: ACCEPT: success

describe('redirect', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const redirectWallet = testContext.alice

    await trust(
      testContext.client,
      testContext.ic.set(100000),
      ...[testContext.hook1]
    )
    await pay(
      testContext.client,
      testContext.ic.set(1000),
      testContext.gw,
      ...[testContext.hook1.classicAddress]
    )
    await trust(
      testContext.client,
      testContext.ic.set(100000),
      ...[testContext.hook2]
    )

    const hook1param1 = new iHookParamEntry(
      new iHookParamName('C'),
      new iHookParamValue(currencyToHex('USD'), true)
    )
    const hook1param2 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue(xrpAddressToHex(redirectWallet.classicAddress), true)
    )
    const acct1hook1 = createHookPayload({
      version: 0,
      createFile: 'redirect',
      namespace: 'redirect',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment'],
      hookParams: [hook1param1.toXrpl(), hook1param2.toXrpl()],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [{ Hook: acct1hook1 }],
    } as SetHookParams)
    await setHooksV3({
      client: testContext.client,
      seed: testContext.hook2.seed,
      hooks: [{ Hook: acct1hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    const clearHook = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('redirect'),
    } as iHook
    await clearHookStateV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [{ Hook: clearHook }, { Hook: clearHook }],
    } as SetHookParams)

    await clearAllHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
    } as SetHookParams)
    teardownClient(testContext)
  })

  it('redirect - success', async () => {
    const hookWallet1 = testContext.hook1
    const aliceWallet = testContext.alice

    const amount: IssuedCurrencyAmount = {
      issuer: testContext.ic.issuer as string,
      currency: testContext.ic.currency as string,
      value: '1',
    }
    const builtTx1: Payment = {
      TransactionType: 'Payment',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet1.classicAddress,
      Amount: amount,
    }

    const result1 = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx1,
    })
    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'redirect.c: Tx emitted success.'
    )
    await close(testContext.client)
  })
})
