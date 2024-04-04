import {
  Invoke,
  Payment,
  SetHookFlags,
  TransactionMetadata,
  AccountSetAsfFlags,
  unixTimeToRippleTime,
} from '@transia/xrpl'
// xrpl-helpers
import {
  serverUrl,
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  accountSet,
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
  flipHex,
} from '@transia/hooks-toolkit'
import {
  currencyToHex,
  xflToHex,
  xrpAddressToHex,
  uint32ToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { sign, verify } from '@transia/ripple-keypairs'

describe('funds - Success Group', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1
    // const userWallet = testContext.alice
    const settlementWallet = testContext.bob

    const adminWallet = testContext.frank
    const settleInvoker = testContext.grace
    const refundInvoker = testContext.heidi
    const withdrawInvoker = testContext.ivan

    await accountSet(
      testContext.client,
      hookWallet,
      AccountSetAsfFlags.asfRequireDest
    )
    await close(testContext.client)

    const hookParam1 = new iHookParamEntry(
      new iHookParamName('ADM'),
      new iHookParamValue(xrpAddressToHex(adminWallet.classicAddress), true)
    )
    const hookParam2 = new iHookParamEntry(
      new iHookParamName('STL'),
      new iHookParamValue(xrpAddressToHex(settleInvoker.classicAddress), true)
    )
    const hookParam3 = new iHookParamEntry(
      new iHookParamName('RFD'),
      new iHookParamValue(xrpAddressToHex(refundInvoker.classicAddress), true)
    )
    const hookParam4 = new iHookParamEntry(
      new iHookParamName('WKEY'),
      new iHookParamValue(withdrawInvoker.publicKey, true)
    )
    const hookParam5 = new iHookParamEntry(
      new iHookParamName('CUR'),
      new iHookParamValue(currencyToHex(testContext.ic.currency), true)
    )
    const hookParam6 = new iHookParamEntry(
      new iHookParamName('ISS'),
      new iHookParamValue(xrpAddressToHex(testContext.ic.issuer), true)
    )
    const hookParam7 = new iHookParamEntry(
      new iHookParamName('ACC'),
      new iHookParamValue(
        xrpAddressToHex(settlementWallet.classicAddress),
        true
      )
    )
    const hook1 = createHookPayload({
      version: 0,
      createFile: 'monolithic',
      namespace: 'funds',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
      hookParams: [
        hookParam1.toXrpl(),
        hookParam2.toXrpl(),
        hookParam3.toXrpl(),
        hookParam4.toXrpl(),
        hookParam5.toXrpl(),
        hookParam6.toXrpl(),
        hookParam7.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    teardownClient(testContext)
  })

  it('operation - initialize', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.frank
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('I')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(xflToHex(100000), true)
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
      'Monolithic: Emitted TrustSet to initialize.'
    )
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
      'Monolithic: Paused/Unpaused.'
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
      'Monolithic: Paused/Unpaused.'
    )
  })
  it('operation - user deposit', async () => {
    const hookWallet = testContext.hook1
    const userWallet = testContext.alice

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
      Account: userWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      DestinationTag: 1,
      Amount: amount,
      HookParameters: [otxnParam1.toXrpl()],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: userWallet,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Monolithic: Deposited.'
    )
  })

  it('operation - debit', async () => {
    const hookWallet = testContext.hook1
    const settlerInvoker = testContext.grace

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('B')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('TAG'),
      new iHookParamValue(uint32ToHex(1), true)
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(xflToHex(10), true)
    )
    const otxnParam4 = new iHookParamEntry(
      new iHookParamName('SEQ'),
      new iHookParamValue(uint32ToHex(0), true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: settlerInvoker.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
        otxnParam4.toXrpl(),
      ],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: settlerInvoker,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    await close(testContext.client)
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Monolithic: Debited.'
    )
  })

  it('operation - settlement', async () => {
    const hookWallet = testContext.hook1
    const settlerInvoker = testContext.grace

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('S')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('TAG'),
      new iHookParamValue(uint32ToHex(1), true)
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(xflToHex(10), true)
    )
    const otxnParam4 = new iHookParamEntry(
      new iHookParamName('SEQ'),
      new iHookParamValue(uint32ToHex(0), true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: settlerInvoker.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
        otxnParam4.toXrpl(),
      ],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: settlerInvoker,
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
    const userWallet = testContext.alice
    const withdrawInvoker = testContext.ivan

    const amount = 5
    const dtag = 1
    const expiration = unixTimeToRippleTime(Date.now() + 3600000)
    const sequence = 0
    const hex =
      xrpAddressToHex(userWallet.classicAddress) +
      xflToHex(amount) +
      flipHex(uint32ToHex(dtag)) +
      flipHex(uint32ToHex(expiration)) +
      flipHex(uint32ToHex(sequence))

    const signature = sign(hex, withdrawInvoker.privateKey)
    expect(verify(hex, signature, withdrawInvoker.publicKey)).toBe(true)

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
      Account: userWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [otxnParam1.toXrpl(), otxnParam2.toXrpl()],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: userWallet,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    await close(testContext.client)
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Monolithic: Emitted withdrawal.'
    )
  })
})
