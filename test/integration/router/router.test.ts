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
  clearAllHooksV3,
  hexNamespace,
  clearHookStateV3,
  iHook,
} from '@transia/hooks-toolkit'
import { HookPosModel } from './models/HookPosModel'
import { HookPosArray } from './models/HookPosArray'

// Router: ACCEPT: success

describe('router', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1
    const hook1 = createHookPayload({
      version: 0,
      createFile: 'router_base',
      namespace: 'router',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
    })
    const hook2 = createHookPayload({
      version: 0,
      createFile: 'router_check_1',
      namespace: 'router',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
    })
    const hook3 = createHookPayload({
      version: 0,
      createFile: 'router_check_2',
      namespace: 'router',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
    })
    const hook4 = createHookPayload({
      version: 0,
      createFile: 'router_check_3',
      namespace: 'router',
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
    await clearAllHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
    } as SetHookParams)

    const clearHook = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('router'),
    } as iHook
    await clearHookStateV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [{ Hook: clearHook }],
    } as SetHookParams)
    teardownClient(testContext)
  })

  it('router - success', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice

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
    expect(hookExecutions1.executions.length).toBe(3)
    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'router.c: Accept.'
    )
  })
})
