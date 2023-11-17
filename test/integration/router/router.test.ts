// xrpl
import { Invoke, SetHookFlags, TransactionMetadata } from '@transia/xrpl'
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
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  ExecutionUtility,
} from '@transia/hooks-toolkit'
import { HookPosModel } from './models/HookPosModel'
import { HookPosArray } from './models/HookPosArray'

// Router: ACCEPT: success

describe('router', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
  })
  afterAll(async () => teardownClient(testContext))

  it('router - success', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice

    const hook1 = createHookPayload(
      0,
      'router_base',
      'router',
      SetHookFlags.hsfOverride,
      ['Invoke']
    )
    const hook2 = createHookPayload(
      0,
      'router_check_1',
      'router',
      SetHookFlags.hsfOverride,
      ['Invoke']
    )
    const hook3 = createHookPayload(
      0,
      'router_check_2',
      'router',
      SetHookFlags.hsfOverride,
      ['Invoke']
    )
    const hook4 = createHookPayload(
      0,
      'router_check_3',
      'router',
      SetHookFlags.hsfOverride,
      ['Invoke']
    )
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

    const hookPos1 = new HookPosModel(1)
    const hookPos2 = new HookPosModel(0)
    const hookPos3 = new HookPosModel(1)
    const hookPos4 = new HookPosModel(1)
    const hookPos5 = new HookPosModel(0)
    const hookPos6 = new HookPosModel(0)
    const hookPos7 = new HookPosModel(0)
    const hookPos8 = new HookPosModel(0)
    const hookPos9 = new HookPosModel(0)
    const hookPos10 = new HookPosModel(0)

    const hookPosArray = new HookPosArray([
      hookPos1,
      hookPos2,
      hookPos3,
      hookPos4,
      hookPos5,
      hookPos6,
      hookPos7,
      hookPos8,
      hookPos9,
      hookPos10,
    ])
    const otxn1param1 = new iHookParamEntry(
      new iHookParamName('HPA'),
      new iHookParamValue(hookPosArray.encode().toUpperCase(), true)
    )

    console.log(hookPosArray.encode())

    const builtTx1: Invoke = {
      TransactionType: 'Invoke',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [otxn1param1.toXrpl()],
    }
    const result1 = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx1,
    })

    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    console.log(hookExecutions1)

    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'router.c: Accept.'
    )
  })
})
