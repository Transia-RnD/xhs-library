// xrpl
import {
  Client,
  Wallet,
  Invoke,
  Payment,
  SetHookFlags,
  TransactionMetadata,
  xrpToDrops,
} from '@transia/xrpl'
// xrpl-helpers
import {
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  serverUrl,
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
  hexNamespace,
  clearAllHooksV3,
  clearHookStateV3,
  iHook,
  padHexString,
  flipHex,
} from '@transia/hooks-toolkit'
import {
  decodeModel,
  uint64ToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { LotteryModel } from './models/LotteryModel'

// Router: ACCEPT: success

export async function createLottery(
  client: Client,
  wallet: Wallet,
  price: number,
  fee: number,
  feeAccount: string,
  maxAmount: number,
  duration: number
) {
  const lotteryModel = new LotteryModel(
    BigInt(1),
    price,
    fee,
    feeAccount,
    maxAmount,
    BigInt(duration) // 30 seconds from now
  )

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
    Account: wallet.classicAddress,
    HookParameters: [otxn1param1.toXrpl(), otxn1param2.toXrpl()],
  }

  console.log(JSON.stringify([otxn1param1.toXrpl(), otxn1param2.toXrpl()]))
  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'lottery_start.c: Lottery Created.'
  )
}

export async function submitTicket(
  client: Client,
  hash: string,
  hookAccount: string,
  account: Wallet
) {
  // Payment
  // const daveWallet = testContext.dave
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
    Account: account.classicAddress,
    Destination: hookAccount,
    Amount: xrpToDrops(10),
    HookParameters: [otxn3param1.toXrpl(), otxn3param2.toXrpl()],
  }
  const result3 = await Xrpld.submit(client, {
    wallet: account,
    tx: builtTx3,
  })
  const hookExecutions3 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result3.meta as TransactionMetadata
  )

  expect(hookExecutions3.executions[1].HookReturnString).toMatch(
    'lottery.c: Ticket Created.'
  )
}

export async function endLottery(
  client: Client,
  hash: string,
  hookAccount: string,
  wallet: Wallet
) {
  // Invoke
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
    Account: wallet.classicAddress,
    Destination: hookAccount,
    HookParameters: [otxn4param1.toXrpl(), otxn4param2.toXrpl()],
  }
  const result4 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx4,
  })
  await close(client)
  const hookExecutions4 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result4.meta as TransactionMetadata
  )

  expect(hookExecutions4.executions[1].HookReturnString).toMatch(
    'lottery_end.c: Lottery Finished.'
  )
}

describe('lottery', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)

    const hookWallet = testContext.hook1

    const acct1hook1 = createHookPayload({
      version: 0,
      createFile: 'router_base',
      namespace: 'lottery',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
    })
    const acct1hook2 = createHookPayload({
      version: 0,
      createFile: 'lottery_start',
      namespace: 'lottery_start',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
    })
    const acct1hook3 = createHookPayload({
      version: 0,
      createFile: 'lottery',
      namespace: 'lottery',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment'],
    })

    const acct1hook4 = createHookPayload({
      version: 0,
      createFile: 'lottery_end',
      namespace: 'lottery',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
    })
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
    teardownClient(testContext)
  })

  it('lottery - success', async () => {
    // Invoke - Create the lottery
    const hookWallet = testContext.hook1
    const feeWallet = testContext.hook2
    await createLottery(
      testContext.client,
      hookWallet,
      10,
      0.5,
      feeWallet.classicAddress,
      300,
      1200
    )

    const hash = padHexString(flipHex(uint64ToHex(BigInt(1))))

    const hookState = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      hash,
      hexNamespace('lottery')
    )
    const model = decodeModel(hookState.HookStateData, LotteryModel)
    console.log(model)

    await submitTicket(
      testContext.client,
      hash,
      hookWallet.classicAddress,
      testContext.alice
    )

    await submitTicket(
      testContext.client,
      hash,
      hookWallet.classicAddress,
      testContext.bob
    )

    await submitTicket(
      testContext.client,
      hash,
      hookWallet.classicAddress,
      testContext.carol
    )

    await submitTicket(
      testContext.client,
      hash,
      hookWallet.classicAddress,
      testContext.dave
    )
    await submitTicket(
      testContext.client,
      hash,
      hookWallet.classicAddress,
      testContext.dave
    )
    await submitTicket(
      testContext.client,
      hash,
      hookWallet.classicAddress,
      testContext.elsa
    )

    for (let index = 0; index < 20; index++) {
      await close(testContext.client)
    }

    await endLottery(
      testContext.client,
      hash,
      hookWallet.classicAddress,
      testContext.frank
    )
  })
})
