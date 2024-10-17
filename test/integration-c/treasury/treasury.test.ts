// xrpl
import {
  AccountSetAsfFlags,
  SetHookFlags,
  xrpToDrops,
  Payment,
  TransactionMetadata,
} from '@transia/xrpl'
// xrpl-helpers
import {
  accountSet,
  serverUrl,
  setupClient,
  teardownClient,
  XrplIntegrationTestContext,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
// src
import {
  SetHookParams,
  setHooksV3,
  createHookPayload,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  flipHex,
  Xrpld,
  ExecutionUtility,
} from '@transia/hooks-toolkit'
import {
  uint32ToHex,
  xflToHex,
  xrpAddressToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

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

export const hookBeforeAll = async (
  testContext: XrplIntegrationTestContext
) => {
  await accountSet(
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
    new iHookParamValue(xrpAddressToHex(testContext.alice.classicAddress), true)
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
    new iHookParamValue(xrpAddressToHex(testContext.alice.classicAddress), true)
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
}

export const treasuryPayment = async (
  testContext: XrplIntegrationTestContext,
  destination: string,
  amount: number,
  hookResult: string
) => {
  const tx: Payment = {
    TransactionType: 'Payment',
    Account: testContext.judy.classicAddress,
    Destination: destination,
    Amount: xrpToDrops(amount),
  }
  const result = await Xrpld.submit(testContext.client, {
    tx: tx,
    wallet: testContext.judy,
  })
  const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
    testContext.client,
    result.meta as TransactionMetadata
  )
  expect(hookExecutions.executions[0].HookReturnString).toMatch(hookResult)
}

describe('Treasury', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl)
    await hookBeforeAll(testContext)
  })

  afterAll(async () => {
    await teardownClient(testContext)
  })

  it('accept: Treasury: Released successfully.', async () => {
    await hookBeforeAll(testContext)

    // Amount Equal to Limit
    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'Treasury: Released successfully.'
    )

    // Amount Less than Limit
    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'Treasury: Released successfully.'
    )
  })
})
