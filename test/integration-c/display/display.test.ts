// xrpl
import {
  URITokenMint,
  Payment,
  Invoke,
  SetHookFlags,
  convertStringToHex,
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
  SetHookParams,
  createHookPayload,
  setHooksV3,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  ExecutionUtility,
  Xrpld,
  StateUtility,
  hexNamespace,
  padHexString,
  flipHex,
} from '@transia/hooks-toolkit/dist/npm/src'
import { hashURIToken } from '@transia/xrpl/dist/npm/utils/hashes'
import {
  decodeModel,
  hexToUInt64,
  xrpAddressToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { DisplayModel } from './models/DisplayModel'

// LevelThree: ACCEPT: success

describe('hunt', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1

    const hook1 = createHookPayload({
      version: 0,
      createFile: 'display',
      namespace: 'display',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment', 'Invoke'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => teardownClient(testContext))

  it('display - success', async () => {
    const aliceWallet = testContext.alice
    const hookWallet = testContext.hook1

    // Bob: Mint and Sell
    const random = Math.random()
    const builtTx1: URITokenMint = {
      TransactionType: 'URITokenMint',
      Account: aliceWallet.classicAddress,
      URI: convertStringToHex(`ipfs://${random}`),
    }
    await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx1,
    })
    await close(testContext.client)
    const uriTokenID = hashURIToken(
      aliceWallet.classicAddress,
      `ipfs://${random}`
    )

    console.log(uriTokenID)

    const param1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('B')
    )
    const param2 = new iHookParamEntry(
      new iHookParamName('ID'),
      new iHookParamValue(
        '35D1168C6D5B107A3B884B055D0657E36196882AAA16C68E345AC998717E86B3',
        true
      )
    )
    const builtTx: Payment = {
      TransactionType: 'Payment',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: xrpToDrops('6'),
      HookParameters: [param1.toXrpl(), param2.toXrpl()],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    console.log(hookExecutions.executions[0].HookReturnString)

    await close(testContext.client)

    const currentState = await StateUtility.getHookState(
      testContext.client,
      testContext.hook1.classicAddress,
      padHexString(xrpAddressToHex(testContext.hook1.classicAddress)),
      hexNamespace('display')
    )

    console.log(hexToUInt64(flipHex(currentState.HookStateData)))

    console.log(padHexString(currentState.HookStateData, 64))
    console.log(hexNamespace('bids'))

    const state = await StateUtility.getHookState(
      testContext.client,
      testContext.hook1.classicAddress,
      padHexString(currentState.HookStateData, 64),
      hexNamespace('bids')
    )

    console.log(decodeModel(state.HookStateData, DisplayModel))

    for (let index = 0; index < 30; index++) {
      await close(testContext.client)
    }

    const tx2Param1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('U')
    )
    const builtTx2: Invoke = {
      TransactionType: 'Invoke',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [tx2Param1.toXrpl()],
    }
    const result2 = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx2,
    })
    const hookExecutions2 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result2.meta as TransactionMetadata
    )
    console.log(hookExecutions2.executions[0].HookReturnString)

    await close(testContext.client)
  })
})
