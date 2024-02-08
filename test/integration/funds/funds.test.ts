import {
  Invoke,
  Payment,
  SetHookFlags,
  TransactionMetadata,
  unixTimeToRippleTime,
} from '@transia/xrpl'
// xrpl-helpers
import {
  serverUrl,
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  close,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
// src
import {
  Xrpld,
  SetHookParams,
  createHookPayload,
  setHooksV3,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  ExecutionUtility,
  hexNamespace,
  iHook,
  clearAllHooksV3,
} from '@transia/hooks-toolkit'
import {
  currencyToHex,
  uint32ToHex,
  xflToHex,
  xrpAddressToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { sign, verify } from '@transia/ripple-keypairs'

// Success Group
// Failure Group

describe('funds - Success Group', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)

    // API: Auth
    // API: KYC
    // API: Create Account
    const hookWallet = testContext.hook1
    const adminWallet = testContext.frank
    const settleWallet = testContext.frank
    const swKey = testContext.grace.publicKey

    const hookParam1 = new iHookParamEntry(
      new iHookParamName('ADM'),
      new iHookParamValue(xrpAddressToHex(adminWallet.classicAddress), true)
    )
    const hookParam2 = new iHookParamEntry(
      new iHookParamName('CUR'),
      new iHookParamValue(currencyToHex(testContext.ic.currency), true)
    )
    const hookParam3 = new iHookParamEntry(
      new iHookParamName('ISS'),
      new iHookParamValue(xrpAddressToHex(testContext.ic.issuer), true)
    )
    const hookParam4 = new iHookParamEntry(
      new iHookParamName('STL'),
      new iHookParamValue(xrpAddressToHex(settleWallet.classicAddress), true)
    )
    const hookParam5 = new iHookParamEntry(
      new iHookParamName('KEY'),
      new iHookParamValue(swKey, true)
    )
    const hook1 = createHookPayload({
      version: 0,
      createFile: 'funds',
      namespace: 'funds',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
      hookParams: [
        hookParam1.toXrpl(),
        hookParam2.toXrpl(),
        hookParam3.toXrpl(),
        hookParam4.toXrpl(),
        hookParam5.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: hook1 }],
    } as SetHookParams)
    // API: Create Funding Source
  })
  afterAll(async () => {
    const hookWallet = testContext.hook1
    await clearAllHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
    } as SetHookParams)
    const clearHook = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('funds'),
    } as iHook
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: clearHook }],
    } as SetHookParams)
    teardownClient(testContext)
  })

  it('operation - initialize', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('I')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(xflToHex(10), true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [otxnParam1.toXrpl(), otxnParam2.toXrpl()],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    await close(testContext.client)
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Funds: Emitted TrustSet to initialize.'
    )
    // API: Create Card
  })

  it('operation - pause/unpause', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.frank
    const otxn1Param1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('P')
    )
    const builtTx1: Invoke = {
      TransactionType: 'Invoke',
      Account: adminWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [otxn1Param1.toXrpl()],
    }
    const result1 = await Xrpld.submit(testContext.client, {
      wallet: adminWallet,
      tx: builtTx1,
    })
    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    expect(hookExecutions1.executions[0].HookReturnString).toEqual(
      'Funds: Paused/Unpaused.'
    )
    const otxn2Param1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('U')
    )
    const builtTx2: Invoke = {
      TransactionType: 'Invoke',
      Account: adminWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [otxn2Param1.toXrpl()],
    }
    const result2 = await Xrpld.submit(testContext.client, {
      wallet: adminWallet,
      tx: builtTx2,
    })
    const hookExecutions2 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result2.meta as TransactionMetadata
    )
    expect(hookExecutions2.executions[0].HookReturnString).toEqual(
      'Funds: Paused/Unpaused.'
    )
  })
  it('operation - user deposit', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice

    const amount: IssuedCurrencyAmount = {
      issuer: testContext.ic.issuer,
      currency: testContext.ic.currency,
      value: '100',
    }
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('D')
    )
    const builtTx: Payment = {
      TransactionType: 'Payment',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: amount,
      HookParameters: [otxnParam1.toXrpl()],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Funds: Deposited.'
    )
  })

  it('operation - creditcard auth', async () => {
    // No Action - Funds are already locked
  })

  it('operation - creditcard cleared', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.frank
    const settleWallet = testContext.frank
    const privateKey = testContext.grace.privateKey

    const amount = 10
    const expiration = unixTimeToRippleTime(Date.now() + 3600)
    const sequence = 1234
    const hex =
      xrpAddressToHex(settleWallet.classicAddress) +
      xflToHex(amount) +
      uint32ToHex(expiration) +
      uint32ToHex(sequence)

    const signature = sign(hex, privateKey)
    expect(verify(hex, signature, testContext.grace.publicKey)).toBe(true)

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('S')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SIG'),
      new iHookParamValue(hex + signature, true)
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(xflToHex(amount), true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: adminWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
      ],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: adminWallet,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    await close(testContext.client)
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Funds: Emitted settlement.'
    )
  })

  it('operation - user withdrawal', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.frank
    const settleWallet = testContext.frank
    const privateKey = testContext.grace.privateKey

    const amount = 10
    const expiration = unixTimeToRippleTime(Date.now() + 3600)
    const sequence = 0
    const hex =
      xrpAddressToHex(settleWallet.classicAddress) +
      xflToHex(amount) +
      uint32ToHex(expiration) +
      uint32ToHex(sequence)

    const signature = sign(hex, privateKey)
    expect(verify(hex, signature, testContext.grace.publicKey)).toBe(true)

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('W')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SIG'),
      new iHookParamValue(hex + signature, true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: adminWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [otxnParam1.toXrpl(), otxnParam2.toXrpl()],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: adminWallet,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    await close(testContext.client)
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Funds: Emitted withdrawal.'
    )
  })
  it('operation - creditcard refund', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.frank

    const amount: IssuedCurrencyAmount = {
      issuer: testContext.ic.issuer,
      currency: testContext.ic.currency,
      value: '100',
    }
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('R')
    )
    const builtTx: Payment = {
      TransactionType: 'Payment',
      Account: adminWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: amount,
      HookParameters: [otxnParam1.toXrpl()],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: adminWallet,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    await close(testContext.client)
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Funds: Refunded.'
    )
  })
})
