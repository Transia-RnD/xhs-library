// xrpl
import {
  AccountSet,
  Invoke,
  SetHookFlags,
  TransactionMetadata,
  AccountSetAsfFlags,
} from '@transia/xrpl'
// xrpl-helpers
import {
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  serverUrl,
  IC,
  close,
  sell,
  trust,
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
  floatToLEXfl,
} from '@transia/hooks-toolkit'
import {
  currencyToHex,
  xrpAddressToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

describe('portfolio', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1

    // Set Weak TSH
    const asTx: AccountSet = {
      TransactionType: 'AccountSet',
      Account: hookWallet.classicAddress,
      SetFlag: AccountSetAsfFlags.asfTshCollect,
    }
    await Xrpld.submit(testContext.client, {
      wallet: hookWallet,
      tx: asTx,
    })

    // Setup Hook
    const XYZ = new IC(testContext.gw.classicAddress, 'XYZ', 100000)
    await trust(testContext.client, XYZ.set(100000), ...[testContext.hook1])
    await sell(testContext.client, XYZ.set(20000), testContext.gw, 100)
    await pay(
      testContext.client,
      XYZ.set(10000),
      testContext.gw,
      ...[testContext.hook1.classicAddress]
    )

    const hook1 = createHookPayload({
      version: 0,
      createFile: 'trailing_stop',
      namespace: 'portfolio',
      flags: SetHookFlags.hsfCollect + SetHookFlags.hsfOverride,
      hookOnArray: ['OfferCreate', 'Invoke'],
    })

    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    const clearHook = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('portfolio'),
    } as iHook
    await clearHookStateV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [{ Hook: clearHook }],
    } as SetHookParams)

    await clearAllHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
    } as SetHookParams)
    teardownClient(testContext)
  })

  it('portfolio - success', async () => {
    // 100 @ 10%
    const quantity = 100
    const marketPrice = 100
    const lowerLimit = 0.1
    const upperLimit = 0.05
    const hookWallet = testContext.hook1
    const txParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('I')
    )
    const txParam2 = new iHookParamEntry(
      new iHookParamName('MP'),
      new iHookParamValue(floatToLEXfl(String(marketPrice)), true)
    )
    const txParam3 = new iHookParamEntry(
      new iHookParamName('QTY'),
      new iHookParamValue(floatToLEXfl(String(quantity)), true)
    )
    const txParam4 = new iHookParamEntry(
      new iHookParamName('ISS'),
      new iHookParamValue(
        currencyToHex('XYZ') + xrpAddressToHex(testContext.gw.classicAddress),
        true
      )
    )
    const txParam5 = new iHookParamEntry(
      new iHookParamName('LP'),
      new iHookParamValue(floatToLEXfl(String(lowerLimit)), true)
    )
    const txParam6 = new iHookParamEntry(
      new iHookParamName('UP'),
      new iHookParamValue(floatToLEXfl(String(upperLimit)), true)
    )

    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: hookWallet.classicAddress,
      HookParameters: [
        txParam1.toXrpl(),
        txParam2.toXrpl(),
        txParam3.toXrpl(),
        txParam4.toXrpl(),
        txParam5.toXrpl(),
        txParam6.toXrpl(),
      ],
    }

    const result = await Xrpld.submit(testContext.client, {
      wallet: hookWallet,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookExecutions.executions[0].HookReturnString).toMatch(
      'trailing_stop.da: Initialized.'
    )

    await close(testContext.client)
  })
})
