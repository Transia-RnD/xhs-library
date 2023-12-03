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
  // clearAllHooksV3,
  // hexNamespace,
  // clearHookStateV3,
  // iHook,
  generateHash,
} from '@transia/hooks-toolkit'
import { decodeModel } from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { LotteryModel } from './models/LotteryModel'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'

// Router: ACCEPT: success

describe('auction', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1

    const gwWallet = testContext.gw
    const USD = IC.gw('USD', gwWallet.classicAddress)
    await trust(testContext.client, USD.set(100000), ...[hookWallet])

    const acct1hook1 = createHookPayload(
      0,
      'lottery_start',
      'lottery',
      SetHookFlags.hsfCollect + SetHookFlags.hsfOverride,
      ['Invoke']
    )
    const acct1hook2 = createHookPayload(
      0,
      'lottery',
      'lottery',
      SetHookFlags.hsfOverride,
      ['Payment']
    )
    const acct1hook3 = createHookPayload(
      0,
      'lottery_end',
      'lottery',
      SetHookFlags.hsfOverride,
      ['Invoke']
    )
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: acct1hook2 }, { Hook: acct1hook3 }],
    } as SetHookParams)
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
    const charityWallet = testContext.bob
    const CLOSE_TIME: number = (
      await testContext.client.request({
        command: 'ledger',
        ledger_index: 'validated',
      })
    ).result.ledger.close_time
    const lotteryModel = new LotteryModel(
      CLOSE_TIME,
      CLOSE_TIME + 15,
      10,
      0.5,
      feeWallet.classicAddress,
      0.5,
      charityWallet.classicAddress
    )

    const hookAcctHex = AccountID.from(hookWallet.classicAddress).toHex()
    const hash = generateHash(
      Buffer.from(hookAcctHex + lotteryModel.encode().toUpperCase())
    )

    console.log(lotteryModel.encode().toUpperCase().length / 2)

    const otxn1param1 = new iHookParamEntry(
      new iHookParamName('LH'),
      new iHookParamValue(hash, true)
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
    console.log(hookExecutions1)

    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'lottery_start.c: Lottery Created.'
    )

    const hookState = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      hash,
      'lottery'
    )
    const model = decodeModel(hookState.HookStateData, LotteryModel)
    console.log(model)

    const amount: IssuedCurrencyAmount = {
      issuer: testContext.ic.issuer as string,
      currency: testContext.ic.currency as string,
      value: '10',
    }
    // Payment
    const carolWallet = testContext.carol
    const otxn2param1 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )
    const builtTx2: Payment = {
      TransactionType: 'Payment',
      Account: carolWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: amount,
      HookParameters: [otxn2param1.toXrpl()],
    }
    const result2 = await Xrpld.submit(testContext.client, {
      wallet: carolWallet,
      tx: builtTx2,
    })
    const hookExecutions2 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result2.meta as TransactionMetadata
    )
    console.log(hookExecutions2)

    expect(hookExecutions2.executions[0].HookReturnString).toMatch(
      'lottery.c: Ticket Created.'
    )

    // Payment
    const daveWallet = testContext.dave
    const otxn3param1 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )
    const builtTx3: Payment = {
      TransactionType: 'Payment',
      Account: daveWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: amount,
      HookParameters: [otxn3param1.toXrpl()],
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

    expect(hookExecutions3.executions[0].HookReturnString).toMatch(
      'lottery.c: Ticket Created.'
    )

    // Invoke
    const elsaWallet = testContext.elsa
    const otxn4param1 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )
    const builtTx4: Invoke = {
      TransactionType: 'Invoke',
      Account: elsaWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [otxn4param1.toXrpl()],
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
