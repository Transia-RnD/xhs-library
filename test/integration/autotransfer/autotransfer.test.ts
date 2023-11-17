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
  ExecutionUtility,
  clearAllHooksV3,
  clearHookStateV3,
  hexNamespace,
  iHook,
} from '@transia/hooks-toolkit'

// AutoTransfer: ACCEPT: success

describe('autotransfer', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1
    const hook = createHookPayload(
      0,
      'autotransfer',
      'autotransfer',
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
    await clearAllHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
    } as SetHookParams)

    const clearHook = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('autotransfer'),
    } as iHook
    await clearHookStateV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [{ Hook: clearHook }],
    } as SetHookParams)
    teardownClient(testContext)
  })

  it('autotransfer - success', async () => {
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
      URI: convertStringToHex('ipfs://autotransfer'),
    }
    await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: mintTx,
    })
    const uriTokenID = hashURIToken(
      aliceWallet.classicAddress,
      'ipfs://autotransfer'
    )

    // URITokenCreateSellOffer
    const builtTx1: URITokenCreateSellOffer = {
      TransactionType: 'URITokenCreateSellOffer',
      Account: aliceWallet.classicAddress,
      Amount: xrpToDrops(0),
      Destination: hookWallet.classicAddress,
      URITokenID: uriTokenID,
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
      'autotransfer.c: Tx emitted success'
    )
  })
})
