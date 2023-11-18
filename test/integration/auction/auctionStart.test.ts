// xrpl
import {
  AccountSet,
  AccountSetAsfFlags,
  URITokenMint,
  URITokenCreateSellOffer,
  SetHookFlags,
  TransactionMetadata,
  convertStringToHex,
  xrpToDrops,
} from '@transia/xrpl'
import { hashURIToken } from '@transia/xrpl/dist/npm/utils/hashes'
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
} from '@transia/hooks-toolkit'
import { AuctionModel } from './models/AuctionModel'

// Router: ACCEPT: success

describe('auctionStart', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1
    const hook = createHookPayload(
      0,
      'auction_start',
      'auction',
      SetHookFlags.hsfCollect + SetHookFlags.hsfOverride,
      ['URITokenCreateSellOffer']
    )
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: hook }],
    } as SetHookParams)
  })
  afterAll(async () => {
    // await clearAllHooksV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    // } as SetHookParams)

    // const clearHook = {
    //   Flags: SetHookFlags.hsfNSDelete,
    //   HookNamespace: hexNamespace('3mm'),
    // } as iHook
    // await clearHookStateV3({
    //   client: testContext.client,
    //   seed: testContext.hook1.seed,
    //   hooks: [{ Hook: clearHook }],
    // } as SetHookParams)
    teardownClient(testContext)
  })

  it('auctionStart - success', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice

    // TSH Hook
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
    const mintTx: URITokenMint = {
      TransactionType: 'URITokenMint',
      Account: aliceWallet.classicAddress,
      URI: convertStringToHex('ipfs://auctionState'),
    }
    await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: mintTx,
    })
    const uriTokenID = hashURIToken(
      aliceWallet.classicAddress,
      'ipfs://auctionState'
    )
    const uriTokenID =
      '09749E0754C012395AB15B13F63BC6C5BDC37B7C56BA35764FE84401D17AF6E0'

    // URITokenCreateSellOffer
    const currentLedger = await testContext.client.getLedgerIndex()
    const auctionModel = new AuctionModel(
      currentLedger,
      currentLedger + 20,
      10,
      0,
      BigInt(0)
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
  })
})
