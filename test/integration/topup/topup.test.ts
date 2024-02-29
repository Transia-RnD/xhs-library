// xrpl
import {
  AccountSet,
  AccountSetAsfFlags,
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
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
// src
import {
  Xrpld,
  SetHookParams,
  setHooksV3,
  createHookPayload,
  ExecutionUtility,
  // clearAllHooksV3,
  // hexNamespace,
  // clearHookStateV3,
  // iHook,
} from '@transia/hooks-toolkit'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'

// Router: ACCEPT: success

describe('topup', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const aliceWallet = testContext.alice

    // Set Weak TSH
    const asTx: AccountSet = {
      TransactionType: 'AccountSet',
      Account: aliceWallet.classicAddress,
      SetFlag: AccountSetAsfFlags.asfTshCollect,
    }
    await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: asTx,
    })

    const hook1 = createHookPayload({
      version: 0,
      createFile: 'topup',
      namespace: 'topup',
      flags: SetHookFlags.hsfCollect + SetHookFlags.hsfOverride,
      hookOnArray: ['Payment'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: aliceWallet.seed,
      hooks: [{ Hook: hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    // await clearAllHooksV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    // } as SetHookParams)

    // const clearHook = {
    //   Flags: SetHookFlags.hsfNSDelete,
    //   HookNamespace: hexNamespace('topup'),
    // } as iHook
    // await clearHookStateV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    //   hooks: [{ Hook: clearHook }],
    // } as SetHookParams)
    teardownClient(testContext)
  })

  it('router - success', async () => {
    const aliceWallet = testContext.alice
    const bobWallet = testContext.bob

    const amount: IssuedCurrencyAmount = {
      value: String(1),
      currency: testContext.ic.currency,
      issuer: testContext.ic.issuer,
    }
    const builtTx: Payment = {
      TransactionType: 'Payment',
      Account: aliceWallet.classicAddress,
      Destination: bobWallet.classicAddress,
      Amount: amount,
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx,
    })

    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    console.log(hookExecutions)

    expect(hookExecutions.executions[0].HookReturnString).toMatch(
      'topup.c: Accept.'
    )
  })
})
