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
  ExecutionUtility,
  generateHash,
  StateUtility,
  hexNamespace,
  clearHookStateV3,
  clearAllHooksV3,
  iHook,
} from '@transia/hooks-toolkit'
import { OracleModel } from './models/OracleModel'
import { OracleArrayModel } from './models/OracleArrayModel'
import { hexToXfl } from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

// Router: ACCEPT: success

describe('oracle', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1
    const acct1hook1 = createHookPayload({
      version: 0,
      createFile: 'oracle',
      namespace: 'oracle',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: acct1hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    const clearHook = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('oracle'),
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

  it('oracle - success', async () => {
    // Invoke - Update the oracle
    const hookWallet = testContext.hook1
    const oracleModel: OracleModel = new OracleModel(
      testContext.ic.issuer,
      testContext.ic.currency,
      1.2
    )
    const oracleArray: OracleArrayModel = new OracleArrayModel([oracleModel])

    const builtTx1: Invoke = {
      TransactionType: 'Invoke',
      Account: hookWallet.classicAddress,
      Blob: oracleArray.encode().slice(2, oracleArray.encode().length),
    }

    const result1 = await Xrpld.submit(testContext.client, {
      wallet: hookWallet,
      tx: builtTx1,
    })
    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'oracle.c: Updated.'
    )
    const ns = generateHash(
      Buffer.from(
        oracleArray.encode().slice(2, oracleArray.encode().length - 16),
        'hex'
      )
    )
    const state = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      ns,
      hexNamespace('oracle')
    )
    expect(hexToXfl(state.HookStateData) as number).toBe(Number(1.2))
  })
})
