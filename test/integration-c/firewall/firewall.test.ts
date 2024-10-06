// xrpl
import {
  Client,
  Wallet,
  Invoke,
  SetHookFlags,
  TransactionMetadata,
  xrpToDrops,
  Payment,
} from '@transia/xrpl'
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
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  ExecutionUtility,
  StateUtility,
  padHexString,
  xrpAddressToHex,
  hexNamespace,
  decodeModel,
  uint32ToHex,
  hexToUInt32,
  flipHex,
  calculateHookOff,
} from '@transia/hooks-toolkit/dist/npm/src'
import { FirewallModel } from './models/FirewallModel'
import { sign, verify } from '@transia/ripple-keypairs'

export async function getNextNonce(
  client: Client,
  hookAccount: string,
  nonceAccount: string,
  namespace: string
): Promise<number> {
  try {
    const state = await StateUtility.getHookState(
      client,
      hookAccount,
      padHexString(xrpAddressToHex(nonceAccount)),
      hexNamespace(namespace)
    )
    return Number(hexToUInt32(flipHex(state.HookStateData)))
  } catch (error: any) {
    console.log(error.message)
    return 0
  }
}

export async function getFirewall(testContext: XrplIntegrationTestContext) {
  try {
    const state = await StateUtility.getHookState(
      testContext.client,
      testContext.hook1.classicAddress,
      padHexString(xrpAddressToHex(testContext.hook1.classicAddress)),
      hexNamespace('firewall')
    )
    console.log(state)

    const decoded = decodeModel(state.HookStateData, FirewallModel)
    console.log(decoded)
    return true
  } catch (error) {
    console.log(error)

    return false
  }
}

export async function setFirewall(
  testContext: XrplIntegrationTestContext,
  firewallModel: FirewallModel
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('C')
  )

  console.log(firewallModel.encode().toUpperCase())
  console.log(firewallModel.encode().length / 2)

  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('FM'),
    new iHookParamValue(firewallModel.encode().toUpperCase(), true)
  )
  const builtTx: Invoke = {
    TransactionType: 'Invoke',
    Account: testContext.hook1.classicAddress,
    HookParameters: [otxn1param1.toXrpl(), otxn1param2.toXrpl()],
  }

  const result = await Xrpld.submit(testContext.client, {
    wallet: testContext.hook1,
    tx: builtTx,
  })
  const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
    testContext.client,
    result.meta as TransactionMetadata
  )

  expect(hookExecutions.executions[0].HookReturnString).toMatch(
    'Firewall.c: Set Firewall.'
  )
}

export async function updateWhiteList(
  testContext: XrplIntegrationTestContext,
  op: string,
  kpWallet: Wallet,
  account: string,
  hookReturn: string
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue(op)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('ACC'),
    new iHookParamValue(xrpAddressToHex(account), true)
  )
  const nonce = await getNextNonce(
    testContext.client,
    testContext.hook1.classicAddress,
    testContext.hook1.classicAddress,
    'nonces'
  )
  const hex = uint32ToHex(nonce)
  const signature = sign(hex, kpWallet.privateKey)
  expect(verify(hex, signature, kpWallet.publicKey)).toBe(true)
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SIG'),
    new iHookParamValue(signature, true)
  )
  const builtTx: Invoke = {
    TransactionType: 'Invoke',
    Account: testContext.hook1.classicAddress,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
    ],
  }

  const result = await Xrpld.submit(testContext.client, {
    wallet: testContext.hook1,
    tx: builtTx,
  })
  const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
    testContext.client,
    result.meta as TransactionMetadata
  )

  expect(hookExecutions.executions[0].HookReturnString).toMatch(hookReturn)
}

export async function submitTxn(
  client: Client,
  account: Wallet,
  destination: string,
  amount: string,
  hookReturn: string
) {
  // Payment
  const builtTx: Payment = {
    TransactionType: 'Payment',
    Account: account.classicAddress,
    Destination: destination,
    Amount: amount,
  }
  try {
    const result = await Xrpld.submit(client, {
      wallet: account,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      client,
      result.meta as TransactionMetadata
    )

    expect(hookExecutions.executions[0].HookReturnString).toMatch(hookReturn)
  } catch (error: any) {
    expect(error.message).toMatch(hookReturn)
  }
}

describe('firewall', () => {
  let testContext: XrplIntegrationTestContext
  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hook1 = createHookPayload({
      version: 0,
      createFile: 'firewall',
      namespace: 'firewall',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: [],
    })
    hook1.HookOn = calculateHookOff([])
    await setHooksV3({
      client: testContext.client,
      seed: testContext.hook1.seed,
      hooks: [{ Hook: hook1 }],
    } as SetHookParams)
  })
  afterAll(async () => {
    teardownClient(testContext)
  })

  it('firewall - success', async () => {
    const kpWallet = Wallet.fromSeed('sEdSkmmthsWRyiEbr6qsvXbUwsHGjjZ')
    if (!(await getFirewall(testContext))) {
      const firewallModel = new FirewallModel(
        kpWallet.publicKey,
        testContext.hook2.classicAddress,
        86400,
        0,
        1000,
        0
      )
      await setFirewall(testContext, firewallModel)
      // Add whitelist
      await updateWhiteList(
        testContext,
        'A',
        kpWallet,
        testContext.alice.classicAddress,
        'Firewall.c: Add Whitelist Account.'
      )
    }

    // Pass Amount
    await submitTxn(
      testContext.client,
      testContext.hook1,
      testContext.bob.classicAddress,
      xrpToDrops(1000),
      'Firewall.c: Allowing Outgoing Txn. (Amount)'
    )

    // Fail Amount
    await submitTxn(
      testContext.client,
      testContext.hook1,
      testContext.bob.classicAddress,
      xrpToDrops(1001),
      'Firewall.c: Rejecting Outgoing Txn.'
    )

    // Pass Backup Account
    await submitTxn(
      testContext.client,
      testContext.hook1,
      testContext.hook2.classicAddress,
      xrpToDrops(1001),
      'Firewall.c: Allowing Outgoing Txn. (Backup)'
    )

    // Pass Whitelist Account
    await submitTxn(
      testContext.client,
      testContext.hook1,
      testContext.alice.classicAddress,
      xrpToDrops(1001),
      'Firewall.c: Allowing Outgoing Txn. (Whitelisted)'
    )

    await updateWhiteList(
      testContext,
      'R',
      kpWallet,
      testContext.alice.classicAddress,
      'Firewall.c: Remove Whitelist Account.'
    )

    await submitTxn(
      testContext.client,
      testContext.hook1,
      testContext.alice.classicAddress,
      xrpToDrops(1001),
      'Firewall.c: Rejecting Outgoing Txn.'
    )
  })
})
