import { Invoke, SetHookFlags, TransactionMetadata } from '@transia/xrpl'
// xrpl-helpers
import {
  serverUrl,
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  close,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
// src
import {
  Xrpld,
  SetHookParams,
  createHookPayload,
  setHooksV3,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  ExecutionUtility,
} from '@transia/hooks-toolkit'
import {
  currencyToHex,
  uint32ToHex,
  xflToHex,
  xrpAddressToHex,
} from '@transia/binary-models'

describe('pull - Success Group', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    console.log(serverUrl)

    testContext = await setupClient(serverUrl)
    const userWallet = testContext.alice
    const settleInvoker = testContext.grace

    const hookParam1 = new iHookParamEntry(
      new iHookParamName('STL'),
      new iHookParamValue(xrpAddressToHex(settleInvoker.classicAddress), true)
    )
    const hookParam2 = new iHookParamEntry(
      new iHookParamName('CUR'),
      new iHookParamValue(currencyToHex(testContext.ic.currency), true)
    )
    const hookParam3 = new iHookParamEntry(
      new iHookParamName('ISS'),
      new iHookParamValue(xrpAddressToHex(testContext.ic.issuer), true)
    )
    const hookParam4 = new iHookParamEntry(
      new iHookParamName('DTG'),
      new iHookParamValue(uint32ToHex(112, true), true)
    )
    const hook1 = createHookPayload({
      version: 0,
      createFile: 'monolithic',
      namespace: 'pull',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
      hookParams: [
        hookParam1.toXrpl(),
        hookParam2.toXrpl(),
        hookParam3.toXrpl(),
        hookParam4.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: userWallet.seed,
      hooks: [{ Hook: hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    teardownClient(testContext)
  })

  it('operation - debit', async () => {
    const userWallet = testContext.alice
    const settlerInvoker = testContext.grace

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('D')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(xflToHex(10, true), true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: settlerInvoker.classicAddress,
      Destination: userWallet.classicAddress,
      HookParameters: [otxnParam1.toXrpl(), otxnParam2.toXrpl()],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: settlerInvoker,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    await close(testContext.client)
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Monolithic: Debited.'
    )
  })
})
