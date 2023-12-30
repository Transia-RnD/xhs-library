// xrpl
import {
  Invoke,
  Payment,
  SetHookFlags,
  TransactionMetadata,
} from '@transia/xrpl'
import { AccountID } from '@transia/ripple-binary-codec/dist/types'
// xrpl-helpers
import {
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  serverUrl,
  trust,
  IC,
  close,
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
  clearAllHooksV3,
  hexNamespace,
  clearHookStateV3,
  iHook,
  generateHash,
  padHexString,
  // flipHex,
} from '@transia/hooks-toolkit'
import {
  decodeModel,
  // uint64ToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { LotteryModel } from './models/LotteryModel'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'

// Router: ACCEPT: success

describe('auction', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)

    await clearAllHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
    } as SetHookParams)

    const clearHook1 = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('lottery'),
    } as iHook
    const clearHook2 = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('lottery_start'),
    } as iHook
    await clearHookStateV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [
        { Hook: clearHook1 },
        { Hook: clearHook2 },
        { Hook: clearHook1 },
        { Hook: clearHook1 },
      ],
    } as SetHookParams)

    const hookWallet = testContext.hook1

    const gwWallet = testContext.gw
    const USD = IC.gw('USD', gwWallet.classicAddress)
    await trust(testContext.client, USD.set(100000), ...[hookWallet])

    const acct1hook1 = createHookPayload(
      0,
      'router_base',
      'lottery',
      SetHookFlags.hsfOverride,
      ['Invoke', 'Payment']
    )
    const acct1hook2 = createHookPayload(
      0,
      'lottery_start',
      'lottery_start',
      SetHookFlags.hsfOverride,
      ['Invoke']
    )
    const acct1hook3 = createHookPayload(
      0,
      'lottery',
      'lottery',
      SetHookFlags.hsfOverride,
      ['Payment']
    )

    const acct1hook4 = createHookPayload(
      0,
      'lottery_end',
      'lottery',
      SetHookFlags.hsfOverride,
      ['Invoke']
    )
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [
        { Hook: acct1hook1 },
        { Hook: acct1hook2 },
        { Hook: acct1hook3 },
        { Hook: acct1hook4 },
      ],
    } as SetHookParams)
    console.log(
      JSON.stringify([
        { Hook: acct1hook1 },
        { Hook: acct1hook2 },
        { Hook: acct1hook3 },
        { Hook: acct1hook4 },
      ])
    )
  })
  afterAll(async () => {
    // await clearAllHooksV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    // } as SetHookParams)

    // const clearHook = {
    //   Flags: SetHookFlags.hsfNSDelete,
    //   HookNamespace: hexNamespace('lottery'),
    // } as iHook
    // await clearHookStateV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    //   hooks: [{ Hook: clearHook }, { Hook: clearHook }],
    // } as SetHookParams)

    // await clearAllHooksV3({
    //   client: testContext.client,
    //   seed: testContext.carol.seed,
    // } as SetHookParams)
    teardownClient(testContext)
  })

  it('lottery - success', async () => {
    console.log('STARTING')

    // Invoke - Create the lottery
    const hookWallet = testContext.hook1
    const feeWallet = testContext.alice
    const lotteryModel = new LotteryModel(10, 0.5, feeWallet.classicAddress)

    const otxn1param1 = new iHookParamEntry(
      new iHookParamName('HPA'),
      new iHookParamValue('0A01010000000000000000', true)
    )
    const otxn1param2 = new iHookParamEntry(
      new iHookParamName('LM'),
      new iHookParamValue(lotteryModel.encode().toUpperCase(), true)
    )
    const builtTx1: Invoke = {
      TransactionType: 'Invoke',
      Account: hookWallet.classicAddress,
      HookParameters: [otxn1param1.toXrpl(), otxn1param2.toXrpl()],
    }

    const result1 = await Xrpld.submit(testContext.client, {
      wallet: hookWallet,
      tx: builtTx1,
    })
    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )

    expect(hookExecutions1.executions[1].HookReturnString).toMatch(
      'lottery_start.c: Lottery Created.'
    )

    const hookAcctID = AccountID.from(hookWallet.classicAddress)
    const hookState = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      padHexString(hookAcctID.toHex()),
      'lottery'
    )
    const model = decodeModel(hookState.HookStateData, LotteryModel)
    console.log(model)

    const lotteryEnd = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      padHexString(hookAcctID.toHex()),
      'lottery_start'
    )

    const hash = generateHash(Buffer.from(lotteryEnd.HookStateData))
    console.log(hash)

    const amount: IssuedCurrencyAmount = {
      issuer: testContext.ic.issuer as string,
      currency: testContext.ic.currency as string,
      value: '10',
    }
    // Payment
    const carolWallet = testContext.carol
    const otxn2param1 = new iHookParamEntry(
      new iHookParamName('HPA'),
      new iHookParamValue('0A01000100000000000000', true)
    )
    const otxn2param2 = new iHookParamEntry(
      new iHookParamName('LH'),
      new iHookParamValue(hash, true)
    )
    const builtTx2: Payment = {
      TransactionType: 'Payment',
      Account: carolWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: amount,
      HookParameters: [otxn2param1.toXrpl(), otxn2param2.toXrpl()],
    }
    const result2 = await Xrpld.submit(testContext.client, {
      wallet: carolWallet,
      tx: builtTx2,
    })
    const hookExecutions2 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result2.meta as TransactionMetadata
    )

    expect(hookExecutions2.executions[1].HookReturnString).toMatch(
      'lottery.c: Ticket Created.'
    )

    // Payment
    const daveWallet = testContext.dave
    const otxn3param1 = new iHookParamEntry(
      new iHookParamName('HPA'),
      new iHookParamValue('0A01000100000000000000', true)
    )
    const otxn3param2 = new iHookParamEntry(
      new iHookParamName('LH'),
      new iHookParamValue(hash, true)
    )
    const builtTx3: Payment = {
      TransactionType: 'Payment',
      Account: daveWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: amount,
      HookParameters: [otxn3param1.toXrpl(), otxn3param2.toXrpl()],
    }
    const result3 = await Xrpld.submit(testContext.client, {
      wallet: daveWallet,
      tx: builtTx3,
    })
    const hookExecutions3 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result3.meta as TransactionMetadata
    )
    console.log(hookExecutions3)

    expect(hookExecutions3.executions[1].HookReturnString).toMatch(
      'lottery.c: Ticket Created.'
    )

    // Invoke
    const elsaWallet = testContext.elsa
    const otxn4param1 = new iHookParamEntry(
      new iHookParamName('HPA'),
      new iHookParamValue('0A01000001000000000000', true)
    )
    const otxn4param2 = new iHookParamEntry(
      new iHookParamName('LH'),
      new iHookParamValue(hash, true)
    )
    const builtTx4: Invoke = {
      TransactionType: 'Invoke',
      Account: elsaWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [otxn4param1.toXrpl(), otxn4param2.toXrpl()],
    }
    const result4 = await Xrpld.submit(testContext.client, {
      wallet: elsaWallet,
      tx: builtTx4,
    })
    await close(testContext.client)
    const hookExecutions4 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result4.meta as TransactionMetadata
    )
    console.log(hookExecutions4)

    expect(hookExecutions4.executions[1].HookReturnString).toMatch(
      'lottery_end.c: Lottery Finished.'
    )
  })
})
