// xrpl
import { Invoke, SetHookFlags, TransactionMetadata } from '@transia/xrpl'
// xrpl-helpers
import {
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  serverUrl,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
// src
import {
  Xrpld,
  SetHookParams,
  setHooksV3,
  createHookPayload,
  ExecutionUtility,
  clearHookStateV3,
  clearAllHooksV3,
  iHook,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  StateUtility,
  padHexString,
  hexNamespace,
} from '@transia/hooks-toolkit'
import {
  decodeModel,
  xrpAddressToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { sign, verify } from '@transia/ripple-keypairs'
import { DIDModel } from './models/DIDModel'

// Step 1:
// - DMV Manager adds/updates/deletes clerk
// Key: clerkAcct: Data: publicKey

// Step 2:
// - Document created/encrypted (Document ID is generated)
// - DMV Clerk signs DIDModel(owner, document id)
// - DMV Clerk submits invoke to hook
// Key: citizen: Data: message + sig

// Step 3:
// - Anyone Anywhere verifies user account

describe('oracle', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1
    const adminWallet = testContext.alice
    const hookParam1 = new iHookParamEntry(
      new iHookParamName('ADM'),
      new iHookParamValue(xrpAddressToHex(adminWallet.classicAddress), true)
    )
    const acct1hook1 = createHookPayload({
      version: 0,
      createFile: 'did',
      namespace: 'validators',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
      hookParams: [hookParam1.toXrpl()],
    })
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: acct1hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    const clearHook1 = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('validators'),
    } as iHook
    const clearHook2 = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('dids'),
    } as iHook
    await clearHookStateV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [{ Hook: clearHook1 }, { Hook: clearHook2 }],
    } as SetHookParams)

    await clearAllHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
    } as SetHookParams)
    teardownClient(testContext)
  })

  it('did - create validator', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.alice
    const clerk1Wallet = testContext.bob
    // const keypair = Wallet.generate()
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('V')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SO'),
      new iHookParamValue('C')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('VA'),
      new iHookParamValue(xrpAddressToHex(clerk1Wallet.classicAddress), true)
    )
    const otxnParam4 = new iHookParamEntry(
      new iHookParamName('VK'),
      new iHookParamValue(clerk1Wallet.publicKey, true)
    )
    const builtTx1: Invoke = {
      TransactionType: 'Invoke',
      Account: adminWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
        otxnParam4.toXrpl(),
      ],
    }

    const result1 = await Xrpld.submit(testContext.client, {
      wallet: adminWallet,
      tx: builtTx1,
    })
    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'did.c: Validator Created.'
    )
  })
  it('did - create did', async () => {
    const hookWallet = testContext.hook1
    const clerk1Wallet = testContext.bob
    const citizenWallet = testContext.carol
    // const keypair = Wallet.generate()
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('D')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SO'),
      new iHookParamValue('C')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('CA'),
      new iHookParamValue(xrpAddressToHex(citizenWallet.classicAddress), true)
    )
    const hex = new DIDModel(clerk1Wallet.classicAddress, '123456').encode()
    const signature = sign(hex, clerk1Wallet.privateKey)
    expect(verify(hex, signature, clerk1Wallet.publicKey)).toBe(true)

    const otxnParam4 = new iHookParamEntry(
      new iHookParamName('SIG'),
      new iHookParamValue(hex + signature, true)
    )
    const builtTx1: Invoke = {
      TransactionType: 'Invoke',
      Account: clerk1Wallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
        otxnParam4.toXrpl(),
      ],
    }

    const result1 = await Xrpld.submit(testContext.client, {
      wallet: clerk1Wallet,
      tx: builtTx1,
    })
    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'did.c: DID Create.'
    )

    const sigState = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      padHexString(xrpAddressToHex(citizenWallet.classicAddress)),
      hexNamespace('dids')
    )
    // Signature Total - 256 (in hook) * 2
    const sigTotal = 256 * 2
    const model = decodeModel(
      sigState.HookStateData.slice(0, hex.length),
      DIDModel
    )
    const clerkState = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      padHexString(xrpAddressToHex(model.validator)),
      hexNamespace('validators')
    )
    expect(
      verify(
        model.encode(),
        sigState.HookStateData.slice(hex.length, sigTotal).replace(/0+$/, ''),
        clerkState.HookStateData
      )
    ).toBe(true)
  })
})
