// xrpl
import {
  AccountSetAsfFlags,
  SetHookFlags,
  TransactionMetadata,
  ClaimReward,
} from '@transia/xrpl'
// xrpl-helpers
import {
  accountSet,
  close,
  serverUrl,
  setupClient,
  teardownClient,
  XrplIntegrationTestContext,
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
  flipHex,
} from '@transia/hooks-toolkit'
import {
  uint32ToHex,
  xflToHex,
  xrpAddressToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

// Alice is Governance Member
// Alice moves money to "Treasury" (Judy) account
// Treasury account adds both hooks
// - Dest: Alice
// - Amount: 1000
// - Limit: 10

// Genesis Mint
// rollback: Genesis Mint: Destination Account not set as Hook parameter
// rollback: Genesis mint: Invalid Mint Destination.
// rollback: Genesis Mint: Failed To Emit.
// accept: Genesis Mint: Passing ClaimReward.

// Treasury
// accept: Treasury: Hook Set Successfully.
// accept: Treasury: ClaimReward Successful.
// rollback: Treasury: Only ClaimReward and Payment txns are allowed.
// rollback: Treasury: Misconfigured. Amount 'A' not set as Hook parameter.
// rollback: Treasury: Invalid amount.
// rollback: Treasury: Misconfigured. Ledger limit 'L' not set as Hook parameter.
// rollback: Treasury: Ledger limit must be between 324,000(15 days) and 7,884,000(365 days).
// rollback: Treasury: Misconfigured. Destination 'D' not set as Hook parameter.
// rollback: Treasury: Fetching Keylet Failed.
// rollback: Treasury: The Set Destination Account Does Not Exist.
// accept: Treasury: Incoming Transaction.
// rollback: Treasury: Destination does not match.
// rollback: Treasury: You need to wait longer to withdraw.
// rollback: Treasury: Non XAH currency payments are forbidden.
// rollback: Treasury: Outgoing transaction exceeds the limit set by you.
// rollback: Treasury: Could not update state entry.
// accept: Treasury: Released successfully.

describe('treasury', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)

    accountSet(
      testContext.client,
      testContext.judy,
      AccountSetAsfFlags.asfTshCollect
    )
    const hook1Param1 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue(xflToHex(10), true)
    )
    const hook1Param2 = new iHookParamEntry(
      new iHookParamName('L'),
      new iHookParamValue(flipHex(uint32ToHex(4)), true)
    )
    const hook1Param3 = new iHookParamEntry(
      new iHookParamName('D'),
      new iHookParamValue(
        xrpAddressToHex(testContext.alice.classicAddress),
        true
      )
    )
    const acct1hook1 = createHookPayload({
      version: 0,
      createFile: 'treasury',
      namespace: 'treasury',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Payment', 'SetHook', 'ClaimReward'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    const hook2Param1 = new iHookParamEntry(
      new iHookParamName('D'),
      new iHookParamValue(
        xrpAddressToHex(testContext.alice.classicAddress),
        true
      )
    )
    const acct1hook2 = createHookPayload({
      version: 0,
      createFile: 'genesis_mint',
      namespace: 'treasury',
      flags: SetHookFlags.hsfCollect + SetHookFlags.hsfOverride,
      hookOnArray: ['GenesisMint'],
      hookParams: [hook2Param1.toXrpl()],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: acct1hook2 }],
    } as SetHookParams)
  })

  afterAll(async () => {
    await teardownClient(testContext)
  })

  it('Genesis Mint - success', async () => {
    // Opt In ClaimReward
    const optInTx: ClaimReward = {
      TransactionType: 'ClaimReward',
      Account: testContext.judy.classicAddress,
      Issuer: testContext.master.classicAddress,
    }
    await Xrpld.submit(testContext.client, {
      wallet: testContext.judy,
      tx: optInTx,
    })

    // Wait 100 Seconds
    // await new Promise((resolve) => setTimeout(resolve, 20000)) // await

    // ClaimReward
    const claimTx: ClaimReward = {
      TransactionType: 'ClaimReward',
      Account: testContext.judy.classicAddress,
      Issuer: testContext.master.classicAddress,
    }
    const result = await Xrpld.submit(testContext.client, {
      wallet: testContext.judy,
      tx: claimTx,
    })

    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )

    expect(hookExecutions.executions[0].HookReturnString).toMatch(
      'Treasury: ClaimReward Successful.'
    )

    await close(testContext.client)
  })
})
