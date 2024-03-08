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
  trust,
  IC,
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
  // StateUtility,
  // hexNamespace,
  // clearAllHooksV3,
  // clearHookStateV3,
  // iHook,
  // padHexString,
  // flipHex,
} from '@transia/hooks-toolkit'
// import {
//   decodeModel,
//   hexToUInt64,
//   uint64ToHex,
// } from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { LotteryModel } from './models/LotteryModel'

// Router: ACCEPT: success

export function calculateOdds(
  results: string[]
): Record<string, { count: number; odds: number }> {
  const counts: Record<string, number> = {}

  // Count the occurrences of each type
  for (const result of results) {
    if (counts[result]) {
      counts[result]++
    } else {
      counts[result] = 1
    }
  }

  // Calculate the odds
  const totalFlips = results.length
  const odds: Record<string, { count: number; odds: number }> = {}

  for (const type in counts) {
    odds[type] = {
      count: counts[type],
      odds: counts[type] / totalFlips,
    }
  }

  return odds
}

export async function createLottery(
  client: Client,
  wallet: Wallet,
  id: number,
  price: number,
  fee: number,
  feeAccount: string,
  maxAmount: number,
  maxTickets: number,
  duration: number
) {
  const lotteryModel = new LotteryModel(
    BigInt(id),
    price,
    fee,
    feeAccount,
    maxAmount,
    BigInt(maxTickets),
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
  account: Wallet,
  ticketPrice: number
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
    Amount: xrpToDrops(ticketPrice),
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

  const hookEmissions = await ExecutionUtility.getHookEmittedTxsFromMeta(
    client,
    result4.meta as TransactionMetadata
  )
  return hookEmissions
}

describe('lottery', () => {
  let testContext: XrplIntegrationTestContext
  const rate = 1

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)

    const hookWallet = testContext.hook1
    const rateWallet = testContext.hook5

    const USD = IC.gw('USD', testContext.gw.classicAddress)
    await trust(testContext.client, USD.set(rate), ...[rateWallet])

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
    // await clearAllHooksV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    // } as SetHookParams)

    // const clearHook1 = {
    //   Flags: SetHookFlags.hsfNSDelete,
    //   HookNamespace: hexNamespace('lottery'),
    // } as iHook
    // const clearHook2 = {
    //   Flags: SetHookFlags.hsfNSDelete,
    //   HookNamespace: hexNamespace('lottery_start'),
    // } as iHook
    // await clearHookStateV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    //   hooks: [
    //     { Hook: clearHook1 },
    //     { Hook: clearHook2 },
    //     { Hook: clearHook1 },
    //     { Hook: clearHook1 },
    //   ],
    // } as SetHookParams)
    teardownClient(testContext)
  })

  it('lottery - TMP', async () => {
    // Invoke - Create the lottery
    const hookWallet = testContext.hook1
    const id = 1
    const _rate = 0.12
    const ticketPrice = 10
    const maxAmount = 300
    await createLottery(
      testContext.client,
      hookWallet,
      id,
      ticketPrice,
      0.5,
      'r223rsyz1cfqPbjmiX6oYu1hFgNwCkWZH',
      maxAmount,
      maxAmount / _rate / ticketPrice,
      1200
    )
  })

  // it('lottery - Restart No Tickets', async () => {
  //   // Invoke - Create the lottery
  //   const hookWallet = testContext.hook1
  //   const feeWallet = testContext.hook2
  //   const CLOSE_TIME: number = (
  //     await testContext.client.request({
  //       command: 'ledger',
  //       ledger_index: 'validated',
  //     })
  //   ).result.ledger.close_time
  //   const id = 1
  //   const ticketPrice = 10
  //   const maxAmount = 300
  //   await createLottery(
  //     testContext.client,
  //     hookWallet,
  //     id,
  //     ticketPrice,
  //     0.5,
  //     feeWallet.classicAddress,
  //     maxAmount,
  //     maxAmount / ticketPrice / rate,
  //     15
  //   )

  //   const hash = padHexString(flipHex(uint64ToHex(BigInt(id))))

  //   const lotteryState = await StateUtility.getHookState(
  //     testContext.client,
  //     hookWallet.classicAddress,
  //     hash,
  //     hexNamespace('lottery')
  //   )
  //   const model = decodeModel(lotteryState.HookStateData, LotteryModel)
  //   expect(Number(model.id)).toBe(id)
  //   expect(model.price).toBe(10)
  //   expect(model.fee).toBe(0.5)
  //   expect(model.feeAddress).toBe(feeWallet.classicAddress)
  //   expect(model.maxAmount).toBe(300)
  //   expect(model.duration).toBe(15n)

  //   const timeState = await StateUtility.getHookState(
  //     testContext.client,
  //     hookWallet.classicAddress,
  //     hash,
  //     hexNamespace('lottery_start')
  //   )

  //   expect(CLOSE_TIME + 1 + Number(model.duration)).toBe(
  //     Number(hexToUInt64(flipHex(timeState.HookStateData)))
  //   )

  //   for (let index = 0; index < 20; index++) {
  //     await close(testContext.client)
  //   }

  //   const CLOSE_TIME1: number = (
  //     await testContext.client.request({
  //       command: 'ledger',
  //       ledger_index: 'validated',
  //     })
  //   ).result.ledger.close_time

  //   await endLottery(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.frank
  //   )

  //   const timeState1 = await StateUtility.getHookState(
  //     testContext.client,
  //     hookWallet.classicAddress,
  //     hash,
  //     hexNamespace('lottery_start')
  //   )

  //   expect(CLOSE_TIME1 + 1 + Number(model.duration)).toBe(
  //     Number(hexToUInt64(flipHex(timeState1.HookStateData)))
  //   )

  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.alice,
  //     ticketPrice
  //   )
  // })

  // it('lottery - Restart 1 Ticket', async () => {
  //   // Invoke - Create the lottery
  //   const hookWallet = testContext.hook1
  //   const feeWallet = testContext.hook2
  //   const id = 2
  //   const ticketPrice = 10
  //   const maxAmount = 300
  //   await createLottery(
  //     testContext.client,
  //     hookWallet,
  //     id,
  //     ticketPrice,
  //     0.5,
  //     feeWallet.classicAddress,
  //     maxAmount,
  //     maxAmount / ticketPrice / rate,
  //     15
  //   )

  //   const hash = padHexString(flipHex(uint64ToHex(BigInt(id))))

  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.alice,
  //     ticketPrice
  //   )

  //   for (let index = 0; index < 20; index++) {
  //     await close(testContext.client)
  //   }

  //   await endLottery(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.frank
  //   )
  // })

  // it('lottery - failure - timer & tickets', async () => {
  //   // Invoke - Create the lottery
  //   const hookWallet = testContext.hook1
  //   const feeWallet = testContext.hook2
  //   const id = 4
  //   const ticketPrice = 10
  //   const maxAmount = 300
  //   await createLottery(
  //     testContext.client,
  //     hookWallet,
  //     id,
  //     ticketPrice,
  //     0.5,
  //     feeWallet.classicAddress,
  //     maxAmount,
  //     maxAmount / ticketPrice / rate,
  //     30
  //   )

  //   const hash = padHexString(flipHex(uint64ToHex(BigInt(id))))

  //   try {
  //     // Invoke
  //     const otxn4param1 = new iHookParamEntry(
  //       new iHookParamName('HPA'),
  //       new iHookParamValue('0A01000001000000000000', true)
  //     )
  //     const otxn4param2 = new iHookParamEntry(
  //       new iHookParamName('LH'),
  //       new iHookParamValue(hash, true)
  //     )
  //     const builtTx4: Invoke = {
  //       TransactionType: 'Invoke',
  //       Account: testContext.frank.classicAddress,
  //       Destination: hookWallet.classicAddress,
  //       HookParameters: [otxn4param1.toXrpl(), otxn4param2.toXrpl()],
  //     }
  //     await Xrpld.submit(testContext.client, {
  //       wallet: testContext.frank,
  //       tx: builtTx4,
  //     })
  //     await close(testContext.client)
  //   } catch (error: any) {
  //     expect(error.message).toEqual('lottery_end.c: Lottery not ended.')
  //   }
  // })
  // it('lottery - success - timer ended', async () => {
  //   // Invoke - Create the lottery
  //   const hookWallet = testContext.hook1
  //   const feeWallet = testContext.hook2
  //   const id = 5
  //   const ticketPrice = 10
  //   const maxAmount = 300
  //   await createLottery(
  //     testContext.client,
  //     hookWallet,
  //     id,
  //     ticketPrice,
  //     0.5,
  //     feeWallet.classicAddress,
  //     maxAmount,
  //     maxAmount / ticketPrice / rate,
  //     30
  //   )

  //   const hash = padHexString(flipHex(uint64ToHex(BigInt(id))))

  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.alice,
  //     ticketPrice
  //   )

  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.bob,
  //     ticketPrice
  //   )

  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.carol,
  //     ticketPrice
  //   )

  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.dave,
  //     ticketPrice
  //   )
  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.dave,
  //     ticketPrice
  //   )
  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.elsa,
  //     ticketPrice
  //   )

  //   for (let index = 0; index < 20; index++) {
  //     await close(testContext.client)
  //   }

  //   await endLottery(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.frank
  //   )
  // })

  // it('lottery - success - tickets max', async () => {
  //   // Invoke - Create the lottery
  //   const hookWallet = testContext.hook1
  //   const feeWallet = testContext.hook2
  //   const id = 6
  //   const ticketPrice = 10
  //   const maxAmount = 300
  //   await createLottery(
  //     testContext.client,
  //     hookWallet,
  //     id,
  //     ticketPrice,
  //     0.5,
  //     feeWallet.classicAddress,
  //     maxAmount,
  //     3,
  //     1200
  //   )

  //   const hash = padHexString(flipHex(uint64ToHex(BigInt(id))))

  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.alice,
  //     ticketPrice
  //   )
  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.bob,
  //     ticketPrice
  //   )
  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.carol,
  //     ticketPrice
  //   )

  //   await endLottery(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.frank
  //   )

  //   const txs: any = (
  //     await testContext.client.request({
  //       command: 'ledger',
  //       ledger_index: 'validated',
  //       transactions: true,
  //       expand: true,
  //     })
  //   ).result.ledger.transactions
  //   for (let index = 0; index < txs.length; index++) {
  //     const element = txs[index]
  //     if (element['TransactionType'] === 'Payment') {
  //       expect(element['Amount']).toEqual('30000000')
  //     }
  //   }
  // })

  // it('lottery - failure - tickets max', async () => {
  //   // Invoke - Create the lottery
  //   const hookWallet = testContext.hook1
  //   const feeWallet = testContext.hook2
  //   const id = 7
  //   const ticketPrice = 100
  //   const maxAmount = 300
  //   await createLottery(
  //     testContext.client,
  //     hookWallet,
  //     id,
  //     ticketPrice,
  //     0.5,
  //     feeWallet.classicAddress,
  //     maxAmount,
  //     maxAmount / ticketPrice / rate,
  //     30
  //   )

  //   const hash = padHexString(flipHex(uint64ToHex(BigInt(id))))

  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.alice,
  //     ticketPrice
  //   )
  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.bob,
  //     ticketPrice
  //   )
  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.carol,
  //     ticketPrice
  //   )

  //   try {
  //     // Payment
  //     const daveWallet = testContext.dave
  //     const otxn3param1 = new iHookParamEntry(
  //       new iHookParamName('HPA'),
  //       new iHookParamValue('0A01000100000000000000', true)
  //     )
  //     const otxn3param2 = new iHookParamEntry(
  //       new iHookParamName('LH'),
  //       new iHookParamValue(hash, true)
  //     )
  //     const builtTx3: Payment = {
  //       TransactionType: 'Payment',
  //       Account: daveWallet.classicAddress,
  //       Destination: hookWallet.classicAddress,
  //       Amount: xrpToDrops(ticketPrice),
  //       HookParameters: [otxn3param1.toXrpl(), otxn3param2.toXrpl()],
  //     }
  //     await Xrpld.submit(testContext.client, {
  //       wallet: daveWallet,
  //       tx: builtTx3,
  //     })
  //   } catch (error: any) {
  //     const executions = JSON.parse(error.message)
  //     expect(executions[1].HookReturnString).toEqual(
  //       'lottery_end.c: Lottery maximum reached.'
  //     )
  //   }
  // })

  // it('lottery - success - overflow', async () => {
  //   // Invoke - Create the lottery
  //   const hookWallet = testContext.hook1
  //   const feeWallet = testContext.hook2
  //   const id = 8
  //   const ticketPrice = 1000
  //   const maxAmount = 300
  //   await createLottery(
  //     testContext.client,
  //     hookWallet,
  //     id,
  //     ticketPrice,
  //     0.5,
  //     feeWallet.classicAddress,
  //     maxAmount,
  //     3,
  //     30
  //   )

  //   const hash = padHexString(flipHex(uint64ToHex(BigInt(id))))

  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.alice,
  //     ticketPrice
  //   )
  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.bob,
  //     ticketPrice
  //   )
  //   await submitTicket(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.carol,
  //     ticketPrice
  //   )

  //   for (let index = 0; index < 20; index++) {
  //     await close(testContext.client)
  //   }

  //   await endLottery(
  //     testContext.client,
  //     hash,
  //     hookWallet.classicAddress,
  //     testContext.frank
  //   )

  //   const txs: any = (
  //     await testContext.client.request({
  //       command: 'ledger',
  //       ledger_index: 'validated',
  //       transactions: true,
  //       expand: true,
  //     })
  //   ).result.ledger.transactions
  //   for (let index = 0; index < txs.length; index++) {
  //     const element = txs[index]
  //     if (element['TransactionType'] === 'Payment') {
  //       expect(element['Amount']).toEqual('300000000')
  //     }
  //   }
  // })

  // it('lottery - Odds 100', async () => {
  //   // Invoke - Create the lottery
  //   const hookWallet = testContext.hook1
  //   const feeWallet = testContext.hook2
  //   const winners: string[] = []
  //   for (let index = 0; index < 100; index++) {
  //     const id = index + 9
  //     const ticketPrice = 10
  //     const maxAmount = 300
  //     await createLottery(
  //       testContext.client,
  //       hookWallet,
  //       id,
  //       ticketPrice,
  //       0.5,
  //       feeWallet.classicAddress,
  //       maxAmount,
  //       maxAmount / ticketPrice / rate,
  //       15
  //     )

  //     const hash = padHexString(flipHex(uint64ToHex(BigInt(id))))

  //     await submitTicket(
  //       testContext.client,
  //       hash,
  //       hookWallet.classicAddress,
  //       testContext.bob,
  //       ticketPrice
  //     )
  //     await submitTicket(
  //       testContext.client,
  //       hash,
  //       hookWallet.classicAddress,
  //       testContext.carol,
  //       ticketPrice
  //     )

  //     for (let index = 0; index < 20; index++) {
  //       await close(testContext.client)
  //     }

  //     const emittedTxns = await endLottery(
  //       testContext.client,
  //       hash,
  //       hookWallet.classicAddress,
  //       testContext.frank
  //     )
  //     for (let index = 0; index < emittedTxns.txs.length; index++) {
  //       const emittedTxn = emittedTxns.txs[index]
  //       if (emittedTxn.TransactionType === 'Payment') {
  //         winners.push(emittedTxn.Destination)
  //       }
  //     }
  //   }
  //   console.log(calculateOdds(winners))
  // })
})
