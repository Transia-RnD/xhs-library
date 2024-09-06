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
  IC,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
// src
import {
  SetHookParams,
  createHookPayload,
  setHooksV3,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  ExecutionUtility,
  Xrpld,
  clearAllHooksV3,
  hexNamespace,
  clearHookStateV3,
  iHook,
} from '@transia/hooks-toolkit/dist/npm/src'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { SellerModel } from './models/SellerModel'
import { SellerArray } from './models/SellerArray'

// Checkout: ACCEPT: success

describe('checkout', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const gwWallet = testContext.gw
    const hookWallet = testContext.hook1
    const USD = IC.gw('USD', gwWallet.classicAddress)
    await trust(testContext.client, USD.set(100000), ...[hookWallet])

    const hook1 = createHookPayload({
      version: 0,
      createFile: 'checkout',
      namespace: 'checkout',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    await clearAllHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
    } as SetHookParams)

    const clearHook = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('checkout'),
    } as iHook
    await clearHookStateV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [{ Hook: clearHook }],
    } as SetHookParams)
    teardownClient(testContext)
  })

  it('checkout - success', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice
    const seller1Wallet = testContext.bob
    const seller2Wallet = testContext.carol
    const seller3Wallet = testContext.dave

    const seller1 = new SellerModel(100, seller1Wallet.classicAddress)
    const seller2 = new SellerModel(100, seller2Wallet.classicAddress)
    const seller3 = new SellerModel(50, seller3Wallet.classicAddress)

    const sellerArray = new SellerArray([seller1, seller2, seller3])

    const param1 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue(sellerArray.encode(), true)
    )
    const amount: IssuedCurrencyAmount = {
      issuer: testContext.ic.issuer as string,
      currency: testContext.ic.currency as string,
      value: '250',
    }
    const builtTx: Payment = {
      TransactionType: 'Payment',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: amount,
      HookParameters: [param1.toXrpl()],
    }

    const result = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx,
    })
    await close(testContext.client)

    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookExecutions.executions[0].HookReturnString).toMatch(
      'checkout.c: Finished.'
    )
  })
})
