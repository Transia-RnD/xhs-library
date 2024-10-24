// xrpl
import {
  AccountSetAsfFlags,
  SetHookFlags,
  SetHook,
  Invoke,
  TransactionMetadata,
  ClaimReward,
  Client,
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
  createHookPayload,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  flipHex,
  Xrpld,
  ExecutionUtility,
  floatToLEXfl,
  setHooksV3,
  SetHookParams,
} from '@transia/hooks-toolkit'
import {
  uint32ToHex,
  xflToHex,
  xrpAddressToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { getEmittedTxnID } from '../../utils/misc'

// Treasury
// rollback: Treasury: HookOn field is incorrectly set. -- IGNORED
// rollback: Treasury: Misconfigured. Amount 'A' not set as Hook parameter.
// rollback: Treasury: Invalid amount.
// rollback: Treasury: You don't want to set it to 10M plus XAH!
// rollback: Treasury: Misconfigured. Ledger limit 'L' not set as Hook parameter.
// rollback: Treasury: Ledger limit must be greater than 324,000(15 days).
// rollback: Treasury: Ledger limit must be less than 7,884,000(365 days).
// rollback: Treasury: Misconfigured. Destination 'D' not set as Hook parameter.
// rollback: Treasury: Fetching Keylet Failed. -- NOT TESTABLE
// rollback: Treasury: The Set Destination Account Does Not Exist.
// rollback: Treasury: Failed To Emit. -- NOT TESTABLE
// rollback: Treasury: Fetching Keylet Failed. -- NOT TESTABLE
// rollback: Treasury: Reward Claim Setup Failed. -- NOT TESTABLE
// accept: Treasury: Reward Claim Setup Passed.
// rollback: Treasury: Rewards are disabled by governance. -- NOT TESTABLE
// rollback: Treasury: Rewards incorrectly configured by governance or unrecoverable error. -- NOT TESTABLE
// rollback: You must wait XXXXXX seconds.
// rollback: Treasury: Failed To Emit. -- NOT TESTABLE
// accept: Treasury: Claimed successfully.
// rollback: Treasury: Specify The Amount To Withdraw.
// rollback: Treasury: Outgoing transaction exceeds the amount limit set by you.
// rollback: Treasury: You must wait 000002 ledgers.
// rollback: Treasury: Failed To Emit. -- NOT TESTABLE
// rollback: Treasury: Could not update state entry, bailing -- NOT TESTABLE
// rollback: Treasury: Could not update state entry, bailing -- NOT TESTABLE
// accept: Treasury: Released successfully.

export const hookBeforeAll = async (
  testContext: XrplIntegrationTestContext
) => {
  try {
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
      hookOnArray: ['Invoke'],
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
    const tx: SetHook = {
      TransactionType: `SetHook`,
      Account: testContext.judy.address,
      Hooks: [{ Hook: acct1hook1 }, { Hook: acct1hook2 }],
    }
    await Xrpld.submit(testContext.client, {
      tx: tx,
      wallet: testContext.judy,
    })
    // throw Error('Test Error')
  } catch (error: any) {
    expect(error.message).toMatch('Treasury: HookOn field is incorrectly set.')
  }
}

export const treasuryPayment = async (
  testContext: XrplIntegrationTestContext,
  destination: string,
  amount: number,
  hookResult: string,
  // eslint-disable-next-line @typescript-eslint/no-inferrable-types
  isFailure: boolean = false
) => {
  try {
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('W'),
      new iHookParamValue(floatToLEXfl(String(amount)), true)
    )
    const tx: Invoke = {
      TransactionType: 'Invoke',
      Account: testContext.judy.classicAddress,
      Destination: destination,
      HookParameters: [otxnParam1.toXrpl()],
    }
    const result = await Xrpld.submit(testContext.client, {
      tx: tx,
      wallet: testContext.judy,
    })

    if (isFailure) throw Error('Test Error')

    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookExecutions.executions[0].HookReturnString).toMatch(hookResult)
    await close(testContext.client)
  } catch (error: any) {
    if (isFailure) expect(error.message).toMatch(hookResult)
    else throw error
  }
}

export const waitForClaim = async (client: Client) => {
  for (let i = 0; i < 10; i++) {
    await new Promise((resolve) => setTimeout(resolve, 1000))
    await close(client)
  }
}

// 'tesSUCCESS'
export const validateEmitted = async (
  client: Client,
  hash: string,
  txResult: string,
  executionIndex: number,
  executionResult: string
) => {
  const reponse = await client.request({
    command: 'tx',
    transaction: hash,
  })
  expect((reponse.result.meta as TransactionMetadata).TransactionResult).toBe(
    txResult
  )

  if (executionResult === null) return reponse

  const hookExecutions = await ExecutionUtility.getHookExecutionsFromTx(
    client,
    hash
  )
  expect(hookExecutions.executions[executionIndex].HookReturnString).toMatch(
    executionResult
  )
  return reponse
}

export const treasuryClaim = async (
  testContext: XrplIntegrationTestContext,
  hookResult: string,
  claimResult: string,
  mintResult: string,
  // eslint-disable-next-line @typescript-eslint/no-inferrable-types
  isFailure: boolean = false
) => {
  try {
    const otxnParam1 = new iHookParamEntry(
      new iHookParamName('C'),
      new iHookParamValue('C')
    )
    const tx: Invoke = {
      TransactionType: 'Invoke',
      Account: testContext.judy.classicAddress,
      HookParameters: [otxnParam1.toXrpl()],
    }
    const result = await Xrpld.submit(testContext.client, {
      tx: tx,
      wallet: testContext.judy,
    })

    if (isFailure) throw Error('Test Error')

    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookExecutions.executions[0].HookReturnString).toMatch(hookResult)

    await close(testContext.client)
    const claimHash = getEmittedTxnID(result.meta)
    const claimResponse = await validateEmitted(
      testContext.client,
      claimHash,
      'tesSUCCESS',
      1,
      claimResult
    )

    // Not Genesis Mint (Opt-In)
    if (mintResult === null) return

    await close(testContext.client)
    const mintHash = getEmittedTxnID(claimResponse.result.meta)
    const mintResponse = await validateEmitted(
      testContext.client,
      mintHash,
      'tesSUCCESS',
      0,
      mintResult
    )

    await close(testContext.client)
    const xferHash = getEmittedTxnID(mintResponse.result.meta)
    await validateEmitted(testContext.client, xferHash, 'tesSUCCESS', 0, null)
  } catch (error: any) {
    if (isFailure) expect(error.message).toMatch(hookResult)
    else throw error
  }
}

describe('Treasury', () => {
  let testContext: XrplIntegrationTestContext

  beforeAll(async () => {
    testContext = await setupClient(serverUrl, true)
    await hookBeforeAll(testContext)
  })

  afterAll(async () => {
    await teardownClient(testContext)
  })

  it("rollback: Misconfigured. Amount 'A' not set as Hook parameter.", async () => {
    const hook1Param1 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue('00000000000000', true)
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
      hookOnArray: ['Invoke'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
    } as SetHookParams)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      "Treasury: Misconfigured. Amount 'A' not set as Hook parameter.",
      true
    )
  })

  it('rollback: Treasury: Invalid amount.', async () => {
    const hook1Param1 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue(xflToHex(0), true)
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
      hookOnArray: ['Invoke'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
    } as SetHookParams)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'Treasury: Invalid amount.',
      true
    )
  })

  it("rollback: Treasury: You don't want to set it to 10M plus XAH! (1000000)", async () => {
    const hook1Param1 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue(xflToHex(10000000), true)
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
      hookOnArray: ['Invoke'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
    } as SetHookParams)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      "Treasury: You don't want to set it to 10M plus XAH!",
      true
    )
  })

  it("rollback: Treasury: You don't want to set it to 10M plus XAH! (1000001)", async () => {
    const hook1Param1 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue(xflToHex(10000001), true)
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
      hookOnArray: ['Invoke'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
    } as SetHookParams)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      "Treasury: You don't want to set it to 10M plus XAH!",
      true
    )
  })

  it("rollback: Treasury: You don't want to set it to 10M plus XAH! (999999)", async () => {
    const hook1Param1 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue(xflToHex(999999), true)
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
      hookOnArray: ['Invoke'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
    } as SetHookParams)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'Treasury: Released successfully.'
    )
  })

  it("rollback: Treasury: Misconfigured. Ledger limit 'L' not set as Hook parameter.", async () => {
    const hook1Param1 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue(xflToHex(10), true)
    )
    const hook1Param2 = new iHookParamEntry(
      new iHookParamName('L'),
      new iHookParamValue('000000', true)
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
      hookOnArray: ['Invoke'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
    } as SetHookParams)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      "Treasury: Misconfigured. Ledger limit 'L' not set as Hook parameter.",
      true
    )
  })

  it('rollback: Treasury: Ledger limit must be greater than 324,000(15 days).', async () => {
    const hook1Param1 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue(xflToHex(10), true)
    )
    const hook1Param2 = new iHookParamEntry(
      new iHookParamName('L'),
      new iHookParamValue(flipHex(uint32ToHex(2)), true)
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
      hookOnArray: ['Invoke'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
    } as SetHookParams)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'Treasury: Ledger limit must be greater than 324,000(15 days).',
      true
    )
  })

  it('rollback: Treasury: Ledger limit must be less than 7,884,000(365 days).', async () => {
    const hook1Param1 = new iHookParamEntry(
      new iHookParamName('A'),
      new iHookParamValue(xflToHex(10), true)
    )
    const hook1Param2 = new iHookParamEntry(
      new iHookParamName('L'),
      new iHookParamValue(flipHex(uint32ToHex(6)), true)
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
      hookOnArray: ['Invoke'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
    } as SetHookParams)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'Treasury: Ledger limit must be less than 7,884,000(365 days).',
      true
    )
  })

  it("rollback: Treasury: Misconfigured. Destination 'D' not set as Hook parameter.", async () => {
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
      new iHookParamValue('00000000000000000000000000000000000000', true)
    )
    const acct1hook1 = createHookPayload({
      version: 0,
      createFile: 'treasury',
      namespace: 'treasury',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
    } as SetHookParams)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      "Treasury: Misconfigured. Destination 'D' not set as Hook parameter.",
      true
    )
  })

  it('rollback: Treasury: The Set Destination Account Does Not Exist.', async () => {
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
      new iHookParamValue('0000000000000000000000000000000000000000', true)
    )
    const acct1hook1 = createHookPayload({
      version: 0,
      createFile: 'treasury',
      namespace: 'treasury',
      flags: SetHookFlags.hsfOverride,
      hookOnArray: ['Invoke'],
      hookParams: [
        hook1Param1.toXrpl(),
        hook1Param2.toXrpl(),
        hook1Param3.toXrpl(),
      ],
    })
    await setHooksV3({
      client: testContext.client,
      seed: testContext.judy.seed,
      hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
    } as SetHookParams)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'Treasury: The Set Destination Account Does Not Exist.',
      true
    )
  })

  it('rollback: Treasury: Reward Claim Setup Passed.', async () => {
    await hookBeforeAll(testContext)

    // Opt-Out
    const tx: ClaimReward = {
      TransactionType: 'ClaimReward',
      Account: testContext.judy.classicAddress,
      Flags: 1,
    }

    await Xrpld.submit(testContext.client, {
      tx: tx,
      wallet: testContext.judy,
    })

    await treasuryClaim(
      testContext,
      'Treasury: Reward Claim Setup Passed.',
      'Success',
      null,
      false
    )
  })

  it('rollback: You must wait XXXXXX seconds.', async () => {
    await hookBeforeAll(testContext)

    // Opt-Out
    const tx: ClaimReward = {
      TransactionType: 'ClaimReward',
      Account: testContext.judy.classicAddress,
      Flags: 1,
    }

    await Xrpld.submit(testContext.client, {
      tx: tx,
      wallet: testContext.judy,
    })

    await treasuryClaim(
      testContext,
      'Treasury: Reward Claim Setup Passed.',
      'Success',
      null,
      false
    )

    await treasuryClaim(
      testContext,
      'You must wait 0000007 seconds.',
      null,
      null,
      true
    )
  })

  it('accept: Treasury: Claimed successfully.', async () => {
    await hookBeforeAll(testContext)

    // Payment has no effect on Claim
    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'Treasury: Released successfully.'
    )

    // Opt-Out
    const tx: ClaimReward = {
      TransactionType: 'ClaimReward',
      Account: testContext.judy.classicAddress,
      Flags: 1,
    }

    await Xrpld.submit(testContext.client, {
      tx: tx,
      wallet: testContext.judy,
    })

    await treasuryClaim(
      testContext,
      'Treasury: Reward Claim Setup Passed.',
      'Success',
      null,
      false
    )

    await waitForClaim(testContext.client)

    await treasuryClaim(
      testContext,
      'Treasury: Claimed successfully.',
      'Success',
      'Genesis Mint: Passing ClaimReward.',
      false
    )
  })

  it('accept: Treasury: Specify The Amount To Withdraw.', async () => {
    await hookBeforeAll(testContext)

    try {
      const tx: Invoke = {
        TransactionType: 'Invoke',
        Account: testContext.judy.classicAddress,
        Destination: testContext.alice.classicAddress,
      }
      await Xrpld.submit(testContext.client, {
        tx: tx,
        wallet: testContext.judy,
      })
      throw Error('Test Failure')
    } catch (error: any) {
      expect(error.message).toMatch('Treasury: Specify The Amount To Withdraw.')
    }
  })

  it('accept: Treasury: Outgoing transaction exceeds the amount limit set by you.', async () => {
    await hookBeforeAll(testContext)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      100,
      'Outgoing transaction exceeds the amount limit set by you.',
      true
    )
  })

  it('accept: Treasury: You must wait 0000001 ledgers.', async () => {
    await hookBeforeAll(testContext)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'Treasury: Released successfully.'
    )

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'You must wait 0000001 ledgers.',
      true
    )
  })

  it('accept: Treasury: Released successfully.', async () => {
    await hookBeforeAll(testContext)

    await treasuryPayment(
      testContext,
      testContext.alice.classicAddress,
      10,
      'Treasury: Released successfully.'
    )
  })
})
