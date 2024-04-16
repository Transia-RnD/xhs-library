import {
  Client,
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
  StateUtility,
  padHexString,
  hexNamespace,
  flipHex,
  generateHash,
} from '@transia/hooks-toolkit'
import {
  currencyToHex,
  hexToUInt32,
  uint32ToHex,
  xflToHex,
  xrpAddressToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { sign, verify } from '@transia/ripple-keypairs'

export async function getNextNonce(
  client: Client,
  hookAccount: string,
  nonceAccount: string
): Promise<number> {
  try {
    const state = await StateUtility.getHookState(
      client,
      hookAccount,
      padHexString(xrpAddressToHex(nonceAccount)),
      hexNamespace('nonces')
    )
    return Number(hexToUInt32(flipHex(state.HookStateData)))
  } catch (error: any) {
    console.log(error.message)
    return 0
  }
}

describe('modular - Success Group', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)

    const masterHook = testContext.hook1
    const userHook = testContext.hook2

    const adminWallet = testContext.frank
    const settleInvoker = testContext.grace
    const withdrawInvoker = testContext.ivan

    const hookParam1 = new iHookParamEntry(
      new iHookParamName('ADM'),
      new iHookParamValue(xrpAddressToHex(adminWallet.classicAddress), true)
    )
    const hookParam2 = new iHookParamEntry(
      new iHookParamName('STL'),
      new iHookParamValue(xrpAddressToHex(settleInvoker.classicAddress), true)
    )
    const hookParam3 = new iHookParamEntry(
      new iHookParamName('WKEY'),
      new iHookParamValue(withdrawInvoker.publicKey, true)
    )
    const hookParam4 = new iHookParamEntry(
      new iHookParamName('DLY'),
      new iHookParamValue(flipHex(uint32ToHex(10)), true)
    )
    const mhook1 = createHookPayload({
      version: 0,
      createFile: 'master',
      namespace: 'funds',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
      hookParams: [
        hookParam1.toXrpl(),
        hookParam2.toXrpl(),
        hookParam3.toXrpl(),
        hookParam4.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: masterHook.seed,
      hooks: [{ Hook: mhook1 }],
    } as SetHookParams)

    const uhook1 = createHookPayload({
      version: 0,
      createFile: 'user',
      namespace: 'funds',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke', 'Payment'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: userHook.seed,
      hooks: [{ Hook: uhook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    teardownClient(testContext)
  })

  it('operation (master) - add asset', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.frank
    const settlementWallet = testContext.bob

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('A')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('C')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(
        xflToHex(100000) +
          currencyToHex(testContext.ic.currency) +
          xrpAddressToHex(testContext.ic.issuer),
        true
      )
    )
    const otxnParam4 = new iHookParamEntry(
      new iHookParamName('ACC'),
      new iHookParamValue(
        xrpAddressToHex(settlementWallet.classicAddress),
        true
      )
    )
    const hash = generateHash(
      Buffer.from(
        currencyToHex(testContext.ic.currency) +
          xrpAddressToHex(testContext.ic.issuer),
        'hex'
      )
    )
    const otxnParam5 = new iHookParamEntry(
      new iHookParamName('AHS'),
      new iHookParamValue(hash, true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: adminWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
        otxnParam4.toXrpl(),
        otxnParam5.toXrpl(),
      ],
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: adminWallet,
      tx: builtTx,
    })
    console.log(result)

    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    await close(testContext.client)
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Master: Created Asset Integration.'
    )
  })
  it('operation (user) - add currency', async () => {
    const hookWallet = testContext.hook2
    const aliceWallet = testContext.alice
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('A')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('C')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(
        xflToHex(100000) +
          currencyToHex(testContext.ic.currency) +
          xrpAddressToHex(testContext.ic.issuer),
        true
      )
    )
    const hash = generateHash(
      Buffer.from(
        currencyToHex(testContext.ic.currency) +
          xrpAddressToHex(testContext.ic.issuer),
        'hex'
      )
    )
    const otxnParam4 = new iHookParamEntry(
      new iHookParamName('AHS'),
      new iHookParamValue(hash, true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: aliceWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
        otxnParam4.toXrpl(),
      ],
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
      'User: Created Asset Integration.'
    )
  })

  // it('operation - pause/unpause', async () => {
  //   const hookWallet = testContext.hook1
  //   const adminWallet = testContext.frank
  //   const otxn1Param1 = new iHookParamEntry(
  //     new iHookParamName('OP'),
  //     new iHookParamValue('P')
  //   )
  //   const builtTx1: Invoke = {
  //     TransactionType: 'Invoke',
  //     Account: adminWallet.classicAddress,
  //     Destination: hookWallet.classicAddress,
  //     HookParameters: [otxn1Param1.toXrpl()],
  //   }
  //   const result1 = await Xrpld.submit(testContext.client, {
  //     wallet: adminWallet,
  //     tx: builtTx1,
  //   })
  //   const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
  //     testContext.client,
  //     result1.meta as TransactionMetadata
  //   )
  //   expect(hookExecutions1.executions[0].HookReturnString).toEqual(
  //     'Master: Paused/Unpaused.'
  //   )
  //   const otxn2Param1 = new iHookParamEntry(
  //     new iHookParamName('OP'),
  //     new iHookParamValue('U')
  //   )
  //   const builtTx2: Invoke = {
  //     TransactionType: 'Invoke',
  //     Account: adminWallet.classicAddress,
  //     Destination: hookWallet.classicAddress,
  //     HookParameters: [otxn2Param1.toXrpl()],
  //   }
  //   const result2 = await Xrpld.submit(testContext.client, {
  //     wallet: adminWallet,
  //     tx: builtTx2,
  //   })
  //   const hookExecutions2 = await ExecutionUtility.getHookExecutionsFromMeta(
  //     testContext.client,
  //     result2.meta as TransactionMetadata
  //   )
  //   expect(hookExecutions2.executions[0].HookReturnString).toEqual(
  //     'Master: Paused/Unpaused.'
  //   )
  // })
  it('operation - user deposit', async () => {
    const hookWallet = testContext.hook2
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
      'User: Deposited.'
    )
  })

  it('operation - debit', async () => {
    const hookWallet = testContext.hook2
    const settlerInvoker = testContext.grace

    const amount = 10
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('B')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(
        xflToHex(amount) +
          currencyToHex(testContext.ic.currency) +
          xrpAddressToHex(testContext.ic.issuer),
        true
      )
    )
    const nonce = await getNextNonce(
      testContext.client,
      hookWallet.classicAddress,
      settlerInvoker.classicAddress
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('SEQ'),
      new iHookParamValue(flipHex(uint32ToHex(nonce)), true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: settlerInvoker.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
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
      'User: Emitted debit.'
    )
  })

  it('operation - user withdrawal (approval)', async () => {
    const hookWallet = testContext.hook2
    const userWallet = testContext.alice
    const withdrawInvoker = testContext.ivan

    const amount = 5
    const expiration = unixTimeToRippleTime(Date.now() + 3600000)
    const nonce = await getNextNonce(
      testContext.client,
      hookWallet.classicAddress,
      userWallet.classicAddress
    )
    const hex =
      xrpAddressToHex(userWallet.classicAddress) +
      xflToHex(amount) +
      currencyToHex(testContext.ic.currency) +
      xrpAddressToHex(testContext.ic.issuer) +
      flipHex(uint32ToHex(expiration)) +
      flipHex(uint32ToHex(nonce))

    const signature = sign(hex, withdrawInvoker.privateKey)
    expect(verify(hex, signature, withdrawInvoker.publicKey)).toBe(true)

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('W')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('A')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('SIG'),
      new iHookParamValue(hex + signature, true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: userWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
      ],
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
      'User: Emitted approval withdrawal.'
    )
  })
  it('operation - user withdrawal (intent)', async () => {
    const hookWallet = testContext.hook2
    const userWallet = testContext.alice

    const amount = 5
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('W')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('I')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(
        xflToHex(amount) +
          currencyToHex(testContext.ic.currency) +
          xrpAddressToHex(testContext.ic.issuer),
        true
      )
    )
    const nonce = await getNextNonce(
      testContext.client,
      hookWallet.classicAddress,
      userWallet.classicAddress
    )
    const otxnParam4 = new iHookParamEntry(
      new iHookParamName('SEQ'),
      new iHookParamValue(flipHex(uint32ToHex(nonce)), true)
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: userWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
        otxnParam4.toXrpl(),
      ],
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
      'User: Created permissionless withdraw intent.'
    )
  })
  it('operation - user withdrawal (execute)', async () => {
    const hookWallet = testContext.hook2
    const userWallet = testContext.alice

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('W')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('E')
    )
    const nonce = await getNextNonce(
      testContext.client,
      hookWallet.classicAddress,
      userWallet.classicAddress
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('WI'),
      new iHookParamValue(
        xrpAddressToHex(userWallet.classicAddress) +
          uint32ToHex(nonce === 0 ? 0 : nonce - 1),
        true
      )
    )
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: userWallet.classicAddress,
      Destination: hookWallet.classicAddress,
      HookParameters: [
        otxnParam1.toXrpl(),
        otxnParam2.toXrpl(),
        otxnParam3.toXrpl(),
      ],
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
      'User: Emitted permissionless withdrawal.'
    )
  })
  it('operation - modify (admin)', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.frank
    const admin1Wallet = testContext.elsa

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('M')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('A')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('ACC'),
      new iHookParamValue(xrpAddressToHex(admin1Wallet.classicAddress), true)
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
      'Master: Admin Modified.'
    )
  })
  it('operation - modify (settler)', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.elsa
    const settleWallet = testContext.frank

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('M')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('S')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('ACC'),
      new iHookParamValue(xrpAddressToHex(settleWallet.classicAddress), true)
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
      'Master: Settlement Verifier Modified.'
    )
  })
  it('operation - modify (withdraw verifier)', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.elsa
    const withdrawInvoker = testContext.heidi

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('M')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('W')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('KEY'),
      new iHookParamValue(withdrawInvoker.publicKey, true)
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
      'Master: Withdraw Verifier Modified.'
    )
  })
  it('operation - modify (withdraw delay)', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.elsa

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('M')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('D')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('DLY'),
      new iHookParamValue(flipHex(uint32ToHex(10)), true)
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
      'Master: Withdraw Delay Modified.'
    )
  })
  it('operation (master) - update asset', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.frank
    const settlementWallet = testContext.carol

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('A')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('U')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('ACC'),
      new iHookParamValue(
        xrpAddressToHex(settlementWallet.classicAddress),
        true
      )
    )
    const hash = generateHash(
      Buffer.from(
        currencyToHex(testContext.ic.currency) +
          xrpAddressToHex(testContext.ic.issuer),
        'hex'
      )
    )
    const otxnParam4 = new iHookParamEntry(
      new iHookParamName('AHS'),
      new iHookParamValue(hash, true)
    )
    const builtTx: Invoke = {
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
    const result = await Xrpld.submit(testContext.client, {
      wallet: adminWallet,
      tx: builtTx,
    })
    console.log(result)

    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    await close(testContext.client)
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Master: Updated Asset Integration.'
    )
  })
  it('operation (master) - delete asset', async () => {
    const hookWallet = testContext.hook1
    const adminWallet = testContext.frank

    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('OP'),
      new iHookParamValue('A')
    )
    const otxnParam2 = new iHookParamEntry(
      new iHookParamName('SOP'),
      new iHookParamValue('D')
    )
    const otxnParam3 = new iHookParamEntry(
      new iHookParamName('AMT'),
      new iHookParamValue(
        xflToHex(0) +
          currencyToHex(testContext.ic.currency) +
          xrpAddressToHex(testContext.ic.issuer),
        true
      )
    )
    const hash = generateHash(
      Buffer.from(
        currencyToHex(testContext.ic.currency) +
          xrpAddressToHex(testContext.ic.issuer),
        'hex'
      )
    )
    const otxnParam4 = new iHookParamEntry(
      new iHookParamName('AHS'),
      new iHookParamValue(hash, true)
    )
    const builtTx: Invoke = {
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
    const result = await Xrpld.submit(testContext.client, {
      wallet: adminWallet,
      tx: builtTx,
    })
    console.log(result)

    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    await close(testContext.client)
    expect(hookExecutions.executions[0].HookReturnString).toEqual(
      'Master: Deleted Asset Integration.'
    )
  })
})
