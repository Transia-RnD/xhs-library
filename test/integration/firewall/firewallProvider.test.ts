import {
  Invoke,
  LedgerEntryRequest,
  Payment,
  SetHookFlags,
  TransactionMetadata,
  xrpToDrops,
} from '@transia/xrpl'
import {
  HookDefinition as LeHookDefinition,
  Hook as LeHook,
} from '@transia/xrpl/dist/npm/models/ledger'
import {
  XrplIntegrationTestContext,
  setupClient,
  teardownClient,
  serverUrl,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
import {
  Xrpld,
  ExecutionUtility,
  createHookPayload,
  setHooksV3,
  SetHookParams,
  formatAccountBlob,
  StateUtility,
  padHexString,
  // padHexString,
} from '@transia/hooks-toolkit'
import { xrpAddressToHex } from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

// Firewall.Provider: ACCEPT: TT != Invoke
// Firewall.Provider: ACCEPT: TX Destination != HOOK

// Firewall.Provider: ACCEPT: Add Account
// Firewall.Provider: ACCEPT: Remove Account
// Firewall.Provider: ACCEPT: Add Mutliple - TODO
// Firewall.Provider: ACCEPT: Remove Mutliple - TODO

// Firewall.Provider: ACCEPT: Test Max - TODO
// Firewall.Provider: ACCEPT: Test Fee for exponential growth - TODO

describe('firewall provider - Success Group', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    const hook = createHookPayload({
      version: 0,
      createFile: 'firewall_provider',
      namespace: 'firewall_provider',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.alice.seed,
      hooks: [{ Hook: hook }],
    } as SetHookParams)
  })
  afterAll(async () => teardownClient(testContext))

  it('firewall provider - tt != invoke', async () => {
    // PAYMENT IN
    const aliceWallet = testContext.alice
    const bobWallet = testContext.bob
    const builtTx: Payment = {
      TransactionType: 'Payment',
      Account: bobWallet.classicAddress,
      Destination: aliceWallet.classicAddress,
      Amount: xrpToDrops(100),
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: bobWallet,
      tx: builtTx,
    })

    const metadata = result.meta as TransactionMetadata
    expect(metadata.HookExecutions).toBe(undefined)
  })

  it('firewall provider - tx dest != hook', async () => {
    // INVOKE OUT
    const aliceWallet = testContext.alice
    const bobWallet = testContext.bob
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: bobWallet.classicAddress,
      Destination: aliceWallet.classicAddress,
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: bobWallet,
      tx: builtTx,
    })
    const hookEmitted = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookEmitted.executions[0].HookReturnString).toMatch('')
  })

  it('firewall provider - add account state', async () => {
    // INVOKE OUT
    const aliceWallet = testContext.alice
    const blob = formatAccountBlob([testContext.carol.classicAddress])
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: aliceWallet.classicAddress,
      Blob: blob,
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx,
    })
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    // DONEEMPTY
    expect(hookExecutions.executions[0].HookReturnString).toEqual('')

    const leHook = await StateUtility.getHook(
      testContext.client,
      testContext.alice.classicAddress
    )
    const hookDefRequest: LedgerEntryRequest = {
      command: 'ledger_entry',
      hook_definition: leHook.Hooks[0].Hook.HookHash,
    }
    const hookDefRes = await testContext.client.request(hookDefRequest)
    const leHookDef = hookDefRes.result.node as LeHookDefinition
    const hookState = await StateUtility.getHookState(
      testContext.client,
      testContext.alice.classicAddress,
      padHexString(xrpAddressToHex(testContext.carol.classicAddress)),
      leHookDef.HookNamespace as string
    )
    expect(hookState.HookStateData).toBeDefined()
  })

  it('firewall provider - remove account state', async () => {
    // INVOKE OUT
    const aliceWallet = testContext.alice
    const blob = formatAccountBlob([], [testContext.carol.classicAddress])
    const builtTx: Invoke = {
      TransactionType: 'Invoke',
      Account: aliceWallet.classicAddress,
      Blob: blob,
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: aliceWallet,
      tx: builtTx,
    })

    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    // DONEEMPTY
    expect(hookExecutions.executions[0].HookReturnString).toEqual('')

    try {
      const hookReq: LedgerEntryRequest = {
        command: 'ledger_entry',
        hook: {
          account: testContext.alice.classicAddress,
        },
      }
      const hookRes = await testContext.client.request(hookReq)
      const leHook = hookRes.result.node as LeHook
      const hookDefRequest: LedgerEntryRequest = {
        command: 'ledger_entry',
        hook_definition: leHook.Hooks[0].Hook.HookHash,
      }
      const hookDefRes = await testContext.client.request(hookDefRequest)
      const leHookDef = hookDefRes.result.node as LeHookDefinition
      await StateUtility.getHookState(
        testContext.client,
        testContext.alice.classicAddress,
        padHexString(xrpAddressToHex(testContext.carol.classicAddress)),
        leHookDef.HookNamespace as string
      )
      throw Error('Expected Failure')
    } catch (error: unknown) {
      if (error instanceof Error) {
        expect(error.message).toEqual('entryNotFound')
      }
    }
  })
})
