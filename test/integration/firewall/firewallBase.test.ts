import {
  Invoke,
  // AccountSet,
  Payment,
  // SetHook,
  SetHookFlags,
  TransactionMetadata,
  calculateHookOn,
  // xrpToDrops,
} from '@transia/xrpl'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { AccountID, Currency } from '@transia/ripple-binary-codec/dist/types'
import {
  XrplIntegrationTestContext,
  serverUrl,
  setupClient,
  teardownClient,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
import {
  Xrpld,
  ExecutionUtility,
  createHookPayload,
  // floatToLEXfl,
  setHooksV3,
  SetHookParams,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  formatAccountCurrencyBlob,
  formatAccountBlob,
  floatToLEXfl,
} from '@transia/hooks-toolkit'

// Firewall.Base: ACCEPT: passing sethook tt
// Firewall.Base: ACCEPT: amount < 0
// Firewall.Base: ROLLBACK: block account - TODO
// Firewall.Base: ROLLBACK: block in tt
// Firewall.Base: ROLLBACK: block out tt
// Firewall.Base: ROLLBACK: block min xrp
// Firewall.Base: ROLLBACK: block max xrp
// Firewall.Base: ROLLBACK: block min usd
// Firewall.Base: ROLLBACK: block max xrp
// Firewall.Base: ACCEPT: Passthrough

describe('firewall.base - Success Group', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
  })
  afterAll(async () => teardownClient(testContext))

  it('firewall base - ic whitelist', async () => {
    const aliceWallet = testContext.alice
    const bobWallet = testContext.bob
    const elsaWallet = testContext.elsa
    const elsaHex = AccountID.from(elsaWallet.classicAddress).toHex()
    const currencyHex = Currency.from('USD').toHex()
    const hook1Param1 = new iHookParamEntry(
      new iHookParamName('FWP'),
      new iHookParamValue(elsaHex, true)
    )
    const hook1Param2 = new iHookParamEntry(
      new iHookParamName('FIC'),
      new iHookParamValue(currencyHex, true)
    )
    const hook1Param3 = new iHookParamEntry(
      new iHookParamName('FIP'),
      new iHookParamValue(elsaHex, true)
    )
    const hook1Param4 = new iHookParamEntry(
      new iHookParamName('FI'),
      new iHookParamValue(calculateHookOn(['Payment']), true)
    )
    const hook1Param5 = new iHookParamEntry(
      new iHookParamName('FLD'),
      new iHookParamValue(floatToLEXfl('1'), true)
    )
    const hook1Param6 = new iHookParamEntry(
      new iHookParamName('FUD'),
      new iHookParamValue(floatToLEXfl('100'), true)
    )
    const hook1 = createHookPayload({
      version: 0,
      createFile: 'firewall_base',
      namespace: 'firewall_base',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
        hook1Param4.toXrpl(),
        hook1Param5.toXrpl(),
        hook1Param6.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.alice.seed,
      hooks: [{ Hook: hook1 }],
    } as SetHookParams)
    const hook2 = createHookPayload({
      version: 0,
      createFile: 'firewall_provider',
      namespace: 'firewall_provider',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
    })
    hook2.HookNamespace =
      '0000000000000000000000000000000000000000000000000000000000000000'
    await setHooksV3({
      client: testContext.client,
      seed: testContext.elsa.seed,
      hooks: [{ Hook: hook2 }],
    } as SetHookParams)

    // INVOKE OUT
    const tx1Param1 = new iHookParamEntry(
      new iHookParamName('M'),
      new iHookParamValue(floatToLEXfl('1'), true)
    )
    const tx1blob = formatAccountBlob([bobWallet.classicAddress])
    const builtTx1: Invoke = {
      TransactionType: 'Invoke',
      Account: elsaWallet.classicAddress,
      Blob: tx1blob,
      HookParameters: [tx1Param1.toXrpl()],
    }
    const result1 = await Xrpld.submit(testContext.client, {
      wallet: elsaWallet,
      tx: builtTx1,
    })

    const hook1Emitted = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result1.meta as TransactionMetadata
    )
    expect(hook1Emitted.executions[0].HookReturnString).toMatch('')

    // INVOKE OUT
    const tx2Param1 = new iHookParamEntry(
      new iHookParamName('M'),
      new iHookParamValue(floatToLEXfl('2'), true)
    )
    const tx2blob = formatAccountCurrencyBlob('USD', [
      testContext.gw.classicAddress,
    ])
    const builtTx2: Invoke = {
      TransactionType: 'Invoke',
      Account: elsaWallet.classicAddress,
      Blob: tx2blob,
      HookParameters: [tx2Param1.toXrpl()],
    }

    const result2 = await Xrpld.submit(testContext.client, {
      wallet: elsaWallet,
      tx: builtTx2,
    })

    const hook2Emitted = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result2.meta as TransactionMetadata
    )
    expect(hook2Emitted.executions[0].HookReturnString).toMatch('')

    // PAYMENT IN
    const amount: IssuedCurrencyAmount = {
      value: '100',
      currency: 'USD',
      issuer: testContext.gw.classicAddress,
    }
    const builtTx: Payment = {
      TransactionType: 'Payment',
      Account: bobWallet.classicAddress,
      Destination: aliceWallet.classicAddress,
      Amount: amount,
    }

    const result = await Xrpld.submit(testContext.client, {
      wallet: bobWallet,
      tx: builtTx,
    })

    const hookEmitted = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookEmitted.executions[0].HookReturnString).toMatch(
      'Firewall: Passing SetHook txn'
    )
  })

  // it('firewall base - passing sethook tt', async () => {
  //   const param1 = new iHookParamEntry(
  //     new iHookParamName('FI'),
  //     new iHookParamValue(calculateHookOn(['SetHook']), true)
  //   )
  //   const hook = createHookPayload(
  //     0,
  //     'firewall_base',
  //     'firewall_base',
  //     SetHookFlags.hsfOverride,
  //     ['SetHook'],
  //     [param1.toXrpl()]
  //   )
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.alice.seed,
  //     hooks: [{ Hook: hook }],
  //   } as SetHookParams)

  //   const aliceWallet = testContext.alice
  //   const builtTx: SetHook = {
  //     TransactionType: 'SetHook',
  //     Account: aliceWallet.address,
  //     Hooks: [
  //       {
  //         Hook: createHookPayload(0, 'base', 'base', SetHookFlags.hsfOverride, [
  //           'Payment',
  //         ]),
  //       },
  //     ],
  //   }

  //   const result = await Xrpld.submit(testContext.client, {
  //     wallet: aliceWallet,
  //     tx: builtTx,
  //   })

  //   const hookEmitted = await ExecutionUtility.getHookExecutionsFromMeta(
  //     testContext.client,
  //     result.meta as TransactionMetadata
  //   )
  //   expect(hookEmitted.executions[0].HookReturnString).toMatch(
  //     'Firewall: Passing SetHook txn'
  //   )
  // })

  // it('firewall base - amount < 0', async () => {
  //   const param1 = new iHookParamEntry(
  //     new iHookParamName('FI'),
  //     new iHookParamValue(calculateHookOn(['SetHook', 'AccountSet']), true)
  //   )
  //   const hook = createHookPayload(
  //     0,
  //     'firewall_base',
  //     'firewall_base',
  //     SetHookFlags.hsfOverride,
  //     ['AccountSet'],
  //     [param1.toXrpl()]
  //   )
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.alice.seed,
  //     hooks: [{ Hook: hook }],
  //   } as SetHookParams)

  //   // ACCOUNT SET OUT
  //   const aliceWallet = testContext.alice
  //   const builtTx: AccountSet = {
  //     TransactionType: 'AccountSet',
  //     Account: aliceWallet.address,
  //   }
  //   const result = await Xrpld.submit(testContext.client, {
  //     wallet: aliceWallet,
  //     tx: builtTx,
  //   })

  //   const hookEmitted = await ExecutionUtility.getHookExecutionsFromMeta(
  //     testContext.client,
  //     result.meta as TransactionMetadata
  //   )
  //   expect(hookEmitted.executions[0].HookReturnString).toMatch(
  //     'Firewall: Ignoring negative amount'
  //   )
  // })

  // it('firewall base - block in tt', async () => {
  //   const param1 = new iHookParamEntry(
  //     new iHookParamName('FI'),
  //     new iHookParamValue(calculateHookOn(['SetHook']), true)
  //   )
  //   const hook = createHookPayload(
  //     0,
  //     'firewall_base',
  //     'firewall_base',
  //     SetHookFlags.hsfOverride,
  //     ['Payment'],
  //     [param1.toXrpl()]
  //   )
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.alice.seed,
  //     hooks: [{ Hook: hook }],
  //   } as SetHookParams)

  //   try {
  //     // PAYMENT IN
  //     const aliceWallet = testContext.alice
  //     const bobWallet = testContext.bob
  //     const builtTx: Payment = {
  //       TransactionType: 'Payment',
  //       Account: bobWallet.classicAddress,
  //       Destination: aliceWallet.classicAddress,
  //       Amount: xrpToDrops(100),
  //     }
  //     await Xrpld.submit(testContext.client, {
  //       wallet: bobWallet,
  //       tx: builtTx,
  //     })
  //   } catch (error: unknown) {
  //     if (error instanceof Error) {
  //       expect(error.message).toMatch('Firewall: blocking txn type')
  //     }
  //   }
  // })

  // it('firewall base - block out tt', async () => {
  //   const param1 = new iHookParamEntry(
  //     new iHookParamName('FO'),
  //     new iHookParamValue(calculateHookOn(['SetHook']), true)
  //   )

  //   const hook = createHookPayload(
  //     0,
  //     'firewall_base',
  //     'firewall_base',
  //     SetHookFlags.hsfOverride,
  //     ['Payment'],
  //     [param1.toXrpl()]
  //   )
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.alice.seed,
  //     hooks: [{ Hook: hook }],
  //   } as SetHookParams)

  //   try {
  //     // PAYMENT OUT
  //     const aliceWallet = testContext.alice
  //     const carolWallet = testContext.carol
  //     const builtTx: Payment = {
  //       TransactionType: 'Payment',
  //       Account: aliceWallet.classicAddress,
  //       Destination: carolWallet.classicAddress,
  //       Amount: xrpToDrops(100),
  //     }
  //     await Xrpld.submit(testContext.client, {
  //       wallet: aliceWallet,
  //       tx: builtTx,
  //     })
  //   } catch (error: unknown) {
  //     if (error instanceof Error) {
  //       expect(error.message).toMatch('Firewall: blocking txn type')
  //     }
  //   }
  // })

  // it('firewall base - block min xrp', async () => {
  //   const param1 = new iHookParamEntry(
  //     new iHookParamName('FI'),
  //     new iHookParamValue(calculateHookOn(['SetHook', 'Payment']), true)
  //   )
  //   const param2 = new iHookParamEntry(
  //     new iHookParamName('FLD'),
  //     new iHookParamValue(floatToLEXfl('100'), true)
  //   )

  //   const hook = createHookPayload(
  //     0,
  //     'firewall_base',
  //     'firewall_base',
  //     SetHookFlags.hsfOverride,
  //     ['Payment'],
  //     [param1.toXrpl(), param2.toXrpl()]
  //   )
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.alice.seed,
  //     hooks: [{ Hook: hook }],
  //   } as SetHookParams)

  //   try {
  //     // PAYMENT IN
  //     const aliceWallet = testContext.alice
  //     const carolWallet = testContext.carol
  //     const builtTx: Payment = {
  //       TransactionType: 'Payment',
  //       Account: carolWallet.classicAddress,
  //       Destination: aliceWallet.classicAddress,
  //       Amount: xrpToDrops(99),
  //     }
  //     await Xrpld.submit(testContext.client, {
  //       wallet: carolWallet,
  //       tx: builtTx,
  //     })
  //     throw Error('Expected Failure')
  //   } catch (error: unknown) {
  //     if (error instanceof Error) {
  //       console.log(error.message)
  //       expect(error.message).toMatch(
  //         'Firewall: blocking amount below threshold'
  //       )
  //     }
  //   }
  // })

  // it('firewall base - block max xrp', async () => {
  //   const param1 = new iHookParamEntry(
  //     new iHookParamName('FI'),
  //     new iHookParamValue(calculateHookOn(['SetHook', 'Payment']), true)
  //   )
  //   const param2 = new iHookParamEntry(
  //     new iHookParamName('FLD'),
  //     new iHookParamValue(floatToLEXfl('10'), true)
  //   )
  //   const param3 = new iHookParamEntry(
  //     new iHookParamName('FUD'),
  //     new iHookParamValue(floatToLEXfl('100'), true)
  //   )

  //   const hook = createHookPayload(
  //     0,
  //     'firewall_base',
  //     'firewall_base',
  //     SetHookFlags.hsfOverride,
  //     ['Payment'],
  //     [param1.toXrpl(), param2.toXrpl(), param3.toXrpl()]
  //   )
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.alice.seed,
  //     hooks: [{ Hook: hook }],
  //   } as SetHookParams)

  //   try {
  //     // PAYMENT IN
  //     const aliceWallet = testContext.alice
  //     const carolWallet = testContext.carol
  //     const builtTx: Payment = {
  //       TransactionType: 'Payment',
  //       Account: carolWallet.classicAddress,
  //       Destination: aliceWallet.classicAddress,
  //       Amount: xrpToDrops(101),
  //     }
  //     await Xrpld.submit(testContext.client, {
  //       wallet: carolWallet,
  //       tx: builtTx,
  //     })
  //     throw Error('Expected Failure')
  //   } catch (error: unknown) {
  //     if (error instanceof Error) {
  //       expect(error.message).toMatch(
  //         'Firewall: blocking amount above threshold'
  //       )
  //     }
  //   }
  // })

  // it('firewall base - block min amount', async () => {
  //   const param1 = new iHookParamEntry(
  //     new iHookParamName('FI'),
  //     new iHookParamValue(calculateHookOn(['SetHook', 'Payment']), true)
  //   )
  //   const param2 = new iHookParamEntry(
  //     new iHookParamName('FLT'),
  //     new iHookParamValue(floatToLEXfl('100'), true)
  //   )

  //   const hook = createHookPayload(
  //     0,
  //     'firewall_base',
  //     'firewall_base',
  //     SetHookFlags.hsfOverride,
  //     ['Payment'],
  //     [param1.toXrpl(), param2.toXrpl()]
  //   )
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.alice.seed,
  //     hooks: [{ Hook: hook }],
  //   } as SetHookParams)

  //   const amount: IssuedCurrencyAmount = {
  //     value: '99',
  //     currency: 'USD',
  //     issuer: testContext.gw.classicAddress,
  //   }

  //   try {
  //     // PAYMENT IN
  //     const aliceWallet = testContext.alice
  //     const carolWallet = testContext.carol
  //     const builtTx: Payment = {
  //       TransactionType: 'Payment',
  //       Account: carolWallet.classicAddress,
  //       Destination: aliceWallet.classicAddress,
  //       Amount: amount,
  //     }
  //     await Xrpld.submit(testContext.client, {
  //       wallet: carolWallet,
  //       tx: builtTx,
  //     })
  //   } catch (error: unknown) {
  //     if (error instanceof Error) {
  //       expect(error.message).toMatch(
  //         'Firewall: blocking amount below threshold'
  //       )
  //     }
  //   }
  // })
  // it('firewall base - block max amount', async () => {
  //   const param1 = new iHookParamEntry(
  //     new iHookParamName('FI'),
  //     new iHookParamValue(calculateHookOn(['SetHook', 'Payment']), true)
  //   )
  //   const param2 = new iHookParamEntry(
  //     new iHookParamName('FLT'),
  //     new iHookParamValue(floatToLEXfl('10'), true)
  //   )
  //   const param3 = new iHookParamEntry(
  //     new iHookParamName('FUT'),
  //     new iHookParamValue(floatToLEXfl('100'), true)
  //   )

  //   const hook = createHookPayload(
  //     0,
  //     'firewall_base',
  //     'firewall_base',
  //     SetHookFlags.hsfOverride,
  //     ['Payment'],
  //     [param1.toXrpl(), param2.toXrpl(), param3.toXrpl()]
  //   )
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.alice.seed,
  //     hooks: [{ Hook: hook }],
  //   } as SetHookParams)

  //   const amount: IssuedCurrencyAmount = {
  //     value: '101',
  //     currency: 'USD',
  //     issuer: testContext.gw.classicAddress,
  //   }
  //   try {
  //     // PAYMENT IN
  //     const aliceWallet = testContext.alice
  //     const carolWallet = testContext.carol
  //     const builtTx: Payment = {
  //       TransactionType: 'Payment',
  //       Account: carolWallet.classicAddress,
  //       Destination: aliceWallet.classicAddress,
  //       Amount: amount,
  //     }
  //     await Xrpld.submit(testContext.client, {
  //       wallet: carolWallet,
  //       tx: builtTx,
  //     })
  //   } catch (error: unknown) {
  //     if (error instanceof Error) {
  //       expect(error.message).toMatch(
  //         'Firewall: blocking amount above threshold'
  //       )
  //     }
  //   }
  // })

  // it('firewall - passthrough', async () => {
  //   const param1 = new iHookParamEntry(
  //     new iHookParamName('FI'),
  //     new iHookParamValue(calculateHookOn(['SetHook', 'Payment']), true)
  //   )
  //   const param2 = new iHookParamEntry(
  //     new iHookParamName('FO'),
  //     new iHookParamValue(calculateHookOn(['SetHook']), true)
  //   )
  //   const param3 = new iHookParamEntry(
  //     new iHookParamName('FLD'),
  //     new iHookParamValue(floatToLEXfl('1'), true)
  //   )
  //   const param4 = new iHookParamEntry(
  //     new iHookParamName('FUD'),
  //     new iHookParamValue(floatToLEXfl('100'), true)
  //   )

  //   const hook = createHookPayload(
  //     0,
  //     'firewall_base',
  //     'firewall_base',
  //     SetHookFlags.hsfOverride,
  //     ['Payment'],
  //     [param1.toXrpl(), param2.toXrpl(), param3.toXrpl(), param4.toXrpl()]
  //   )
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.alice.seed,
  //     hooks: [{ Hook: hook }],
  //   } as SetHookParams)

  //   // PAYMENT IN
  //   const aliceWallet = testContext.alice
  //   const carolWallet = testContext.carol
  //   const builtTx: Payment = {
  //     TransactionType: 'Payment',
  //     Account: carolWallet.classicAddress,
  //     Destination: aliceWallet.classicAddress,
  //     Amount: xrpToDrops(100),
  //   }
  //   const result = await Xrpld.submit(testContext.client, {
  //     wallet: carolWallet,
  //     tx: builtTx,
  //   })

  //   const hookEmitted = await ExecutionUtility.getHookExecutionsFromMeta(
  //     testContext.client,
  //     result.meta as TransactionMetadata
  //   )
  //   expect(hookEmitted.executions[0].HookReturnString).toMatch(
  //     'Firewall: Passing txn within thresholds'
  //   )
  // })
})
