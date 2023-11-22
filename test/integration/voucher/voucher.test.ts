// xrpl
import {
  Wallet,
  Payment,
  Invoke,
  SetHookFlags,
  TransactionMetadata,
  xrpToDrops,
} from '@transia/xrpl'
import { sign } from '@transia/ripple-keypairs'
// xrpl-helpers
import {
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  serverUrl,
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
  clearAllHooksV3,
  hexNamespace,
  clearHookStateV3,
  iHook,
  generateHash,
} from '@transia/hooks-toolkit'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { VoucherModel } from './models/VoucherModel'
import { uint32ToHex } from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

// Voucher: ROLLBACK: user already claimed
// Voucher: ROLLBACK: voucher limit
// Voucher: ACCEPT: success - xrp
// Voucher: ACCEPT: success - iou

describe('voucher', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hookWallet = testContext.hook1

    // setup trustline
    const USD = IC.gw('USD', testContext.gw.classicAddress)
    await trust(testContext.client, USD.set(100000), ...[hookWallet])

    const hook1 = createHookPayload(
      0,
      'voucher_create',
      'router',
      SetHookFlags.hsfOverride,
      ['Payment']
    )
    const hook2 = createHookPayload(
      0,
      'voucher_claim',
      'router',
      SetHookFlags.hsfOverride,
      ['Invoke']
    )
    await setHooksV3({
      client: testContext.client,
      seed: hookWallet.seed,
      hooks: [{ Hook: hook1 }, { Hook: hook2 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    await clearAllHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
    } as SetHookParams)

    const clearHook = {
      Flags: SetHookFlags.hsfNSDelete,
      HookNamespace: hexNamespace('voucher'),
    } as iHook
    await clearHookStateV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [{ Hook: clearHook }],
    } as SetHookParams)
    teardownClient(testContext)
  })

  it('voucher - xrp failure: user claimed', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice
    const bobWallet = testContext.bob

    const wallet = Wallet.fromSeed('ssd5BQz7JuB4SjSnf7DJkyJi7NbKZ')

    const CLOSE_TIME: number = (
      await testContext.client.request({
        command: 'ledger',
        ledger_index: 'validated',
      })
    ).result.ledger.close_time

    const voucher = new VoucherModel(
      10,
      CLOSE_TIME,
      CLOSE_TIME + 10,
      10,
      '',
      'rrrrrrrrrrrrrrrrrrrrrhoLvTp',
      wallet.publicKey
    )
    const hash = generateHash(Buffer.from(voucher.encode().toUpperCase()))

    console.log(voucher.encode().toUpperCase().length / 2)

    const otxn1param1 = new iHookParamEntry(
      new iHookParamName('M'),
      new iHookParamValue(voucher.encode().toUpperCase(), true)
    )
    const otxn1param2 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )
    const builtTx1: Payment = {
      TransactionType: 'Payment',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: xrpToDrops(100),
      HookParameters: [otxn1param1.toXrpl(), otxn1param2.toXrpl()],
    }

    const result1 = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx1,
    })

    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'voucher_create.c: Voucher Created.'
    )

    const otxn2param1 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )
    const sig = sign(hash, wallet.privateKey)
    const otxn2param2 = new iHookParamEntry(
      new iHookParamName('SIGL'),
      new iHookParamValue(uint32ToHex(sig.length / 2), true)
    )
    const otxn3param2 = new iHookParamEntry(
      new iHookParamName('SIG'),
      new iHookParamValue(sig, true)
    )
    const builtTx2: Invoke = {
      TransactionType: 'Invoke',
      Account: bobWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxn2param1.toXrpl(),
        otxn2param2.toXrpl(),
        otxn3param2.toXrpl(),
      ],
    }
    const result2 = await Xrpld.submit(testContext.client, {
      wallet: bobWallet,
      tx: builtTx2,
    })

    const hookExecutions2 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result2.meta as TransactionMetadata
    )
    expect(hookExecutions2.executions[0].HookReturnString).toMatch(
      'voucher_claim.c: Voucher Claimed.'
    )
    try {
      const builtTx3: Invoke = {
        TransactionType: 'Invoke',
        Account: bobWallet.classicAddress,
        Destination: hookWallet.classicAddress,
        HookParameters: [
          otxn2param1.toXrpl(),
          otxn2param2.toXrpl(),
          otxn3param2.toXrpl(),
        ],
      }
      await Xrpld.submit(testContext.client, {
        wallet: bobWallet,
        tx: builtTx3,
      })
      throw Error('Fail Test')
    } catch (error: any) {
      expect(error.message).toMatch('voucher_claim.c: User already claimed.')
    }
  })

  it('voucher - xrp failure: limit', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice
    const bobWallet = testContext.bob
    const carolWallet = testContext.carol

    const wallet = Wallet.fromSeed('ssd5BQz7JuB4SjSnf7DJkyJi7NbKZ')

    const CLOSE_TIME: number = (
      await testContext.client.request({
        command: 'ledger',
        ledger_index: 'validated',
      })
    ).result.ledger.close_time

    const voucher = new VoucherModel(
      1,
      CLOSE_TIME,
      CLOSE_TIME + 10,
      100,
      '',
      'rrrrrrrrrrrrrrrrrrrrrhoLvTp',
      wallet.publicKey
    )
    const hash = generateHash(Buffer.from(voucher.encode().toUpperCase()))

    console.log(voucher.encode().toUpperCase().length / 2)

    const otxn1param1 = new iHookParamEntry(
      new iHookParamName('M'),
      new iHookParamValue(voucher.encode().toUpperCase(), true)
    )
    const otxn1param2 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )
    const builtTx1: Payment = {
      TransactionType: 'Payment',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: xrpToDrops(100),
      HookParameters: [otxn1param1.toXrpl(), otxn1param2.toXrpl()],
    }

    const result1 = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx1,
    })

    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'voucher_create.c: Voucher Created.'
    )

    const otxn2param1 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )
    const sig = sign(hash, wallet.privateKey)
    const otxn2param2 = new iHookParamEntry(
      new iHookParamName('SIGL'),
      new iHookParamValue(uint32ToHex(sig.length / 2), true)
    )
    const otxn3param2 = new iHookParamEntry(
      new iHookParamName('SIG'),
      new iHookParamValue(sig, true)
    )
    const builtTx2: Invoke = {
      TransactionType: 'Invoke',
      Account: bobWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxn2param1.toXrpl(),
        otxn2param2.toXrpl(),
        otxn3param2.toXrpl(),
      ],
    }
    const result2 = await Xrpld.submit(testContext.client, {
      wallet: bobWallet,
      tx: builtTx2,
    })

    const hookExecutions2 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result2.meta as TransactionMetadata
    )
    expect(hookExecutions2.executions[0].HookReturnString).toMatch(
      'voucher_claim.c: Voucher Claimed.'
    )
    try {
      const builtTx3: Invoke = {
        TransactionType: 'Invoke',
        Account: carolWallet.classicAddress,
        Destination: hookWallet.classicAddress,
        HookParameters: [
          otxn2param1.toXrpl(),
          otxn2param2.toXrpl(),
          otxn3param2.toXrpl(),
        ],
      }
      await Xrpld.submit(testContext.client, {
        wallet: carolWallet,
        tx: builtTx3,
      })
      throw Error('Fail Test')
    } catch (error: any) {
      expect(error.message).toMatch('voucher_claim.c: Voucher limit reached.')
    }
  })

  it('voucher - xrp success', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice
    const bobWallet = testContext.bob

    const wallet = Wallet.fromSeed('ssd5BQz7JuB4SjSnf7DJkyJi7NbKZ')

    const CLOSE_TIME: number = (
      await testContext.client.request({
        command: 'ledger',
        ledger_index: 'validated',
      })
    ).result.ledger.close_time

    const voucher = new VoucherModel(
      10,
      CLOSE_TIME,
      CLOSE_TIME + 10,
      10,
      '',
      'rrrrrrrrrrrrrrrrrrrrrhoLvTp',
      wallet.publicKey
    )
    const hash = generateHash(Buffer.from(voucher.encode().toUpperCase()))

    console.log(voucher.encode().toUpperCase().length / 2)

    const otxn1param1 = new iHookParamEntry(
      new iHookParamName('M'),
      new iHookParamValue(voucher.encode().toUpperCase(), true)
    )
    const otxn1param2 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )
    const builtTx1: Payment = {
      TransactionType: 'Payment',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: xrpToDrops(100),
      HookParameters: [otxn1param1.toXrpl(), otxn1param2.toXrpl()],
    }

    const result1 = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx1,
    })

    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'voucher_create.c: Voucher Created.'
    )

    const otxn2param1 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )
    const sig = sign(hash, wallet.privateKey)
    const otxn2param2 = new iHookParamEntry(
      new iHookParamName('SIGL'),
      new iHookParamValue(uint32ToHex(sig.length / 2), true)
    )
    const otxn3param2 = new iHookParamEntry(
      new iHookParamName('SIG'),
      new iHookParamValue(sig, true)
    )
    const builtTx2: Invoke = {
      TransactionType: 'Invoke',
      Account: bobWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxn2param1.toXrpl(),
        otxn2param2.toXrpl(),
        otxn3param2.toXrpl(),
      ],
    }
    console.log(JSON.stringify(builtTx2))

    const result2 = await Xrpld.submit(testContext.client, {
      wallet: bobWallet,
      tx: builtTx2,
    })

    const hookExecutions2 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result2.meta as TransactionMetadata
    )
    expect(hookExecutions2.executions[0].HookReturnString).toMatch(
      'voucher_claim.c: Voucher Claimed.'
    )
  })
  it('voucher - iou success', async () => {
    const hookWallet = testContext.hook1
    const aliceWallet = testContext.alice
    const bobWallet = testContext.bob

    const wallet = Wallet.fromSeed('ssd5BQz7JuB4SjSnf7DJkyJi7NbKZ')

    const CLOSE_TIME: number = (
      await testContext.client.request({
        command: 'ledger',
        ledger_index: 'validated',
      })
    ).result.ledger.close_time

    const voucher = new VoucherModel(
      10,
      CLOSE_TIME,
      CLOSE_TIME + 10,
      10,
      testContext.ic.currency,
      testContext.ic.issuer,
      wallet.publicKey
    )
    const hash = generateHash(Buffer.from(voucher.encode().toUpperCase()))

    console.log(voucher.encode().toUpperCase().length / 2)

    const otxn1param1 = new iHookParamEntry(
      new iHookParamName('M'),
      new iHookParamValue(voucher.encode().toUpperCase(), true)
    )
    const otxn1param2 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )
    const amount: IssuedCurrencyAmount = {
      issuer: testContext.ic.issuer as string,
      currency: testContext.ic.currency as string,
      value: '100',
    }
    const builtTx1: Payment = {
      TransactionType: 'Payment',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      Amount: amount,
      HookParameters: [otxn1param1.toXrpl(), otxn1param2.toXrpl()],
    }
    const result1 = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx1,
    })

    const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    expect(hookExecutions1.executions[0].HookReturnString).toMatch(
      'voucher_create.c: Voucher Created.'
    )

    const otxn2param1 = new iHookParamEntry(
      new iHookParamName('H'),
      new iHookParamValue(hash, true)
    )

    const sig = sign(hash, wallet.privateKey)
    const otxn2param2 = new iHookParamEntry(
      new iHookParamName('SIGL'),
      new iHookParamValue(uint32ToHex(sig.length / 2), true)
    )
    const otxn3param2 = new iHookParamEntry(
      new iHookParamName('SIG'),
      new iHookParamValue(sig, true)
    )
    const builtTx2: Invoke = {
      TransactionType: 'Invoke',
      Account: bobWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxn2param1.toXrpl(),
        otxn2param2.toXrpl(),
        otxn3param2.toXrpl(),
      ],
    }
    const result2 = await Xrpld.submit(testContext.client, {
      wallet: bobWallet,
      tx: builtTx2,
    })

    const hookExecutions2 = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result2.meta as TransactionMetadata
    )
    expect(hookExecutions2.executions[0].HookReturnString).toMatch(
      'voucher_claim.c: Voucher Claimed.'
    )
  })
})
