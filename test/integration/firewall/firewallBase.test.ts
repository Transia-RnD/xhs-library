import {
  Payment,
  SetHook,
  SetHookFlags,
  TransactionMetadata,
  calculateHookOn,
  xrpToDrops,
} from '@transia/xrpl'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
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
  floatToLEXfl,
  setHooksV3,
  SetHookParams,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  // formatAccountBlob,
} from '@transia/hooks-toolkit'

// Firewall.Base: ACCEPT: Firewall: Ignoring SetHook txn
// Firewall.Base: ROLLBACK: Firewall: Blocklist match: TODO
// Firewall.Base: ROLLBACK: Firewall: blocking txn type (In): FIX
// Firewall.Base: ROLLBACK: Firewall: blocking txn type (Out): FIX
// Firewall.Base: ROLLBACK: Firewall: blocked incoming partial payment
// Firewall.Base: ROLLBACK: Firewall: Amount below threshold (XRP)
// Firewall.Base: ROLLBACK: Firewall: Amount below threshold (IC)
// Firewall.Base: ACCEPT: Passthrough

describe('firewall.base - Success Group', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
  })
  afterAll(async () => teardownClient(testContext))

  it('firewall base - passing sethook tt', async () => {
    const param1 = new iHookParamEntry(
      new iHookParamName('FI'),
      new iHookParamValue(calculateHookOn(['SetHook']), true)
    )
    const hook = createHookPayload({
      version: 0,
      createFile: 'firewall_base',
      namespace: 'firewall_base',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['SetHook'],
      hookParams: [param1.toXrpl()],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.alice.seed,
      hooks: [{ Hook: hook }],
    } as SetHookParams)

    const aliceWallet = testContext.alice
    const builtTx: SetHook = {
      TransactionType: 'SetHook',
      Account: aliceWallet.address,
      Hooks: [
        {
          Hook: createHookPayload({
            version: 0,
            createFile: 'base',
            namespace: 'base',
            flags: SetHookFlags.hsfOverride,
            hookOnArray: ['Payment'],
          }),
        },
      ],
    }

    const result = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx,
    })

    const hookEmitted = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookEmitted.executions[0].HookReturnString).toMatch(
      'Firewall: Ignoring SetHook txn'
    )
  })

  it('firewall base - block in tt', async () => {
    const param1 = new iHookParamEntry(
      new iHookParamName('FI'),
      new iHookParamValue(calculateHookOn(['SetHook']), true)
    )
    console.log(param1.toXrpl())

    const hook = createHookPayload({
      version: 0,
      createFile: 'firewall_base',
      namespace: 'firewall_base',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment'],
      hookParams: [param1.toXrpl()],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.alice.seed,
      hooks: [{ Hook: hook }],
    } as SetHookParams)

    try {
      // PAYMENT IN
      const aliceWallet = testContext.alice
      const bobWallet = testContext.bob
      const builtTx: Payment = {
        TransactionType: 'Payment',
        Account: bobWallet.classicAddress,
        Destination: aliceWallet.classicAddress,
        Amount: xrpToDrops(100),
      }
      await Xrpld.submit(testContext.client, {
        wallet: bobWallet,
        tx: builtTx,
      })
      throw Error('Expected Failure')
    } catch (error: unknown) {
      if (error instanceof Error) {
        expect(error.message).toMatch('Firewall: Txn type')
      }
    }
  })

  it('firewall base - block out tt', async () => {
    const param1 = new iHookParamEntry(
      new iHookParamName('FO'),
      new iHookParamValue(calculateHookOn(['SetHook']), true)
    )

    const hook = createHookPayload({
      version: 0,
      createFile: 'firewall_base',
      namespace: 'firewall_base',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment'],
      hookParams: [param1.toXrpl()],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.alice.seed,
      hooks: [{ Hook: hook }],
    } as SetHookParams)

    try {
      // PAYMENT OUT
      const aliceWallet = testContext.alice
      const carolWallet = testContext.carol
      const builtTx: Payment = {
        TransactionType: 'Payment',
        Account: aliceWallet.classicAddress,
        Destination: carolWallet.classicAddress,
        Amount: xrpToDrops(100),
      }
      await Xrpld.submit(testContext.client, {
        wallet: aliceWallet,
        tx: builtTx,
      })
      throw Error('Expected Failure')
    } catch (error: unknown) {
      if (error instanceof Error) {
        expect(error.message).toMatch('Firewall: Txn type')
      }
    }
  })

  it('firewall base - Firewall: Amount below threshold (XRP)', async () => {
    const param1 = new iHookParamEntry(
      new iHookParamName('FI'),
      new iHookParamValue(calculateHookOn(['SetHook', 'Payment']), true)
    )
    const param2 = new iHookParamEntry(
      new iHookParamName('FD'),
      new iHookParamValue(floatToLEXfl('100'), true)
    )

    const hook = createHookPayload({
      version: 0,
      createFile: 'firewall_base',
      namespace: 'firewall_base',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment'],
      hookParams: [param1.toXrpl(), param2.toXrpl()],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.alice.seed,
      hooks: [{ Hook: hook }],
    } as SetHookParams)

    try {
      // PAYMENT IN
      const aliceWallet = testContext.alice
      const carolWallet = testContext.carol
      const builtTx: Payment = {
        TransactionType: 'Payment',
        Account: carolWallet.classicAddress,
        Destination: aliceWallet.classicAddress,
        Amount: xrpToDrops(99),
      }
      await Xrpld.submit(testContext.client, {
        wallet: carolWallet,
        tx: builtTx,
      })
      throw Error('Expected Failure')
    } catch (error: unknown) {
      if (error instanceof Error) {
        console.log(error.message)
        expect(error.message).toMatch('Firewall: Amount below threshold')
      }
    }
  })

  it('firewall base - Firewall: Amount below threshold (IC)', async () => {
    const param1 = new iHookParamEntry(
      new iHookParamName('FI'),
      new iHookParamValue(calculateHookOn(['SetHook', 'Payment']), true)
    )
    const param2 = new iHookParamEntry(
      new iHookParamName('FT'),
      new iHookParamValue(floatToLEXfl('100'), true)
    )

    const hook = createHookPayload({
      version: 0,
      createFile: 'firewall_base',
      namespace: 'firewall_base',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment'],
      hookParams: [param1.toXrpl(), param2.toXrpl()],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.alice.seed,
      hooks: [{ Hook: hook }],
    } as SetHookParams)

    const amount: IssuedCurrencyAmount = {
      value: '99',
      currency: 'USD',
      issuer: testContext.gw.classicAddress,
    }

    try {
      // PAYMENT IN
      const aliceWallet = testContext.alice
      const carolWallet = testContext.carol
      const builtTx: Payment = {
        TransactionType: 'Payment',
        Account: carolWallet.classicAddress,
        Destination: aliceWallet.classicAddress,
        Amount: amount,
      }
      await Xrpld.submit(testContext.client, {
        wallet: carolWallet,
        tx: builtTx,
      })
      throw Error('Expected Failure')
    } catch (error: unknown) {
      if (error instanceof Error) {
        expect(error.message).toMatch('Firewall: Amount below threshold')
      }
    }
  })

  it('firewall - passthrough', async () => {
    const param1 = new iHookParamEntry(
      new iHookParamName('FI'),
      new iHookParamValue(calculateHookOn(['SetHook', 'Payment']), true)
    )
    const param2 = new iHookParamEntry(
      new iHookParamName('FO'),
      new iHookParamValue(calculateHookOn(['SetHook']), true)
    )
    const param3 = new iHookParamEntry(
      new iHookParamName('FD'),
      new iHookParamValue(floatToLEXfl('99'), true)
    )
    const param4 = new iHookParamEntry(
      new iHookParamName('FT'),
      new iHookParamValue(floatToLEXfl('99'), true)
    )

    const hook = createHookPayload({
      version: 0,
      createFile: 'firewall_base',
      namespace: 'firewall_base',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment'],
      hookParams: [
        param1.toXrpl(),
        param2.toXrpl(),
        param3.toXrpl(),
        param4.toXrpl(),
      ],
    })

    await setHooksV3({
      client: testContext.client,
      seed: testContext.alice.seed,
      hooks: [{ Hook: hook }],
    } as SetHookParams)

    // PAYMENT IN
    const aliceWallet = testContext.alice
    const carolWallet = testContext.carol
    const builtTx: Payment = {
      TransactionType: 'Payment',
      Account: carolWallet.classicAddress,
      Destination: aliceWallet.classicAddress,
      Amount: xrpToDrops(99),
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: carolWallet,
      tx: builtTx,
    })

    const hookEmitted = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookEmitted.executions[0].HookReturnString).toMatch(
      'Firewall: Txn within thresholds'
    )
  })
})
