// xrpl
import {
  Invoke,
  EscrowCreate,
  AccountSet,
  AccountSetAsfFlags,
  URITokenMint,
  URITokenCreateSellOffer,
  SetHookFlags,
  TransactionMetadata,
  convertStringToHex,
  xrpToDrops,
} from '@transia/xrpl'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { hashURIToken } from '@transia/xrpl/dist/npm/utils/hashes'
// xrpl-helpers
import {
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  serverUrl,
  trust,
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
} from '@transia/hooks-toolkit'
import { decodeModel } from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { AuctionModel } from './models/AuctionModel'

// Router: ACCEPT: success

describe('auction', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1
    const carolWallet = testContext.carol
    const acct1hook1 = createHookPayload({
      version: 0,
      createFile: 'auction_start',
      namespace: 'auction',
      flags: SetHookFlags.hsfCollect + SetHookFlags.hsfOverride,
      hookOnArray: ['URITokenCreateSellOffer'],
    })
    const acct1hook2 = createHookPayload({
      version: 0,
      createFile: 'auction',
      namespace: 'auction',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['EscrowCreate', 'Invoke'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: acct1hook2 }],
    } as SetHookParams)

    // AccountSet: Carol Acct
    const asTx: AccountSet = {
      TransactionType: 'AccountSet',
      Account: carolWallet.classicAddress,
      SetFlag: AccountSetAsfFlags.asfTshCollect,
    }
    await Xrpld.submit(testContext.client, {
      wallet: carolWallet,
      tx: asTx,
    })
    const acct2hook1 = createHookPayload({
      version: 0,
      createFile: 'autotransfer',
      namespace: 'autotransfer',
      flags: SetHookFlags.hsfCollect + SetHookFlags.hsfOverride,
      hookOnArray: ['URITokenCreateSellOffer'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: carolWallet.seed,
      hooks: [{ Hook: acct2hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    await clearAllHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
    } as SetHookParams)

    const clearHook = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('auction'),
    } as iHook
    await clearHookStateV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [{ Hook: clearHook }, { Hook: clearHook }],
    } as SetHookParams)

    await clearAllHooksV3({
      client: testContext.client,
      seed: testContext.carol.seed,
    } as SetHookParams)
    teardownClient(testContext)
  })

  it('auction - success', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice
    const bobWallet = testContext.bob
    const carolWallet = testContext.carol

    // AccountSet
    const asTx: AccountSet = {
      TransactionType: 'AccountSet',
      Account: hookWallet.classicAddress,
      SetFlag: AccountSetAsfFlags.asfTshCollect,
    }
    await Xrpld.submit(testContext.client, {
      wallet: hookWallet,
      tx: asTx,
    })

    // URITokenMint
    const random = Math.random()
    const mintTx: URITokenMint = {
      TransactionType: 'URITokenMint',
      Account: aliceWallet.classicAddress,
      URI: convertStringToHex(`ipfs://auctionState-${random}`),
    }
    await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: mintTx,
    })
    const uriTokenID = hashURIToken(
      aliceWallet.classicAddress,
      `ipfs://auctionState-${random}`
    )

    const CLOSE_TIME: number = (
      await testContext.client.request({
        command: 'ledger',
        ledger_index: 'validated',
      })
    ).result.ledger.close_time

    // URITokenCreateSellOffer
    const auctionModel = new AuctionModel(
      CLOSE_TIME,
      CLOSE_TIME + 15,
      10,
      BigInt(0),
      0,
      'rrrrrrrrrrrrrrrrrrrrrhoLvTp',
      '0000000000000000000000000000000000000000000000000000000000000000'
    )
    const otxn1param1 = new iHookParamEntry(
      new iHookParamName('AM'),
      new iHookParamValue(auctionModel.encode().toUpperCase(), true)
    )

    console.log(auctionModel.encode())
    const builtTx1: URITokenCreateSellOffer = {
      TransactionType: 'URITokenCreateSellOffer',
      Account: aliceWallet.classicAddress,
      Amount: xrpToDrops(0),
      Destination: hookWallet.classicAddress,
      URITokenID: uriTokenID,
      HookParameters: [otxn1param1.toXrpl()],
    }
    const result1 = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx1,
    })
    await close(testContext.client)

    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    console.log(hookExecutions1)

    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'auction_start.c: Tx emitted success'
    )

    // Setup Hook
    await trust(
      testContext.client,
      testContext.ic.set(10000),
      ...[testContext.hook1]
    )

    // EscrowCreate - Carol
    const otxn2param1 = new iHookParamEntry(
      new iHookParamName('TID'),
      new iHookParamValue(uriTokenID, true)
    )
    const txAmount1: IssuedCurrencyAmount = {
      value: '100',
      currency: testContext.ic.currency as string,
      issuer: testContext.ic.issuer as string,
    }
    const builtTx2: EscrowCreate = {
      TransactionType: 'EscrowCreate',
      Account: carolWallet.classicAddress,
      Amount: txAmount1,
      Destination: hookWallet.classicAddress,
      FinishAfter: CLOSE_TIME + 15,
      CancelAfter: CLOSE_TIME + 20,
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

    const hookState = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      uriTokenID,
      hexNamespace('auction')
    )
    const model = decodeModel(hookState.HookStateData, AuctionModel)
    console.log(model)

    expect(hookExecutions2.executions[0].HookReturnString).toMatch(
      'auction.c: Accept.'
    )

    // EscrowCreate - Bob
    const otxn3param1 = new iHookParamEntry(
      new iHookParamName('TID'),
      new iHookParamValue(uriTokenID, true)
    )
    const txAmount2: IssuedCurrencyAmount = {
      value: '99',
      currency: testContext.ic.currency as string,
      issuer: testContext.ic.issuer as string,
    }
    const builtTx3: EscrowCreate = {
      TransactionType: 'EscrowCreate',
      Account: bobWallet.classicAddress,
      Amount: txAmount2,
      Destination: hookWallet.classicAddress,
      FinishAfter: CLOSE_TIME + 15,
      CancelAfter: CLOSE_TIME + 20,
      HookParameters: [otxn3param1.toXrpl()],
    }

    try {
      await Xrpld.submit(testContext.client, {
        wallet: bobWallet,
        tx: builtTx3,
      })
    } catch (error: any) {
      console.log(error.message)
    }

    for (let index = 0; index < 6; index++) {
      await close(testContext.client)
    }

    // Invoke - By Hook Evernode
    const otxn4param1 = new iHookParamEntry(
      new iHookParamName('TID'),
      new iHookParamValue(uriTokenID, true)
    )
    const builtTx4: Invoke = {
      TransactionType: 'Invoke',
      Account: hookWallet.classicAddress,
      HookParameters: [otxn4param1.toXrpl()],
    }
    const result4 = await Xrpld.submit(testContext.client, {
      wallet: hookWallet,
      tx: builtTx4,
    })
    await close(testContext.client)
    await close(testContext.client)

    const hookExecutions4 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result4.meta as TransactionMetadata
    )
    console.log(hookExecutions4)

    expect(hookExecutions4.executions[0].HookReturnString).toMatch(
      'auction.c: Completed.'
    )
  })
})
