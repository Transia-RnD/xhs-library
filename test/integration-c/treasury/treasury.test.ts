// xrpl
import {
  AccountSetAsfFlags,
  SetHookFlags,
  SetHook,
  Invoke,
  TransactionMetadata,
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
  iHook,
} from '@transia/hooks-toolkit'
import {
  uint32ToHex,
  // uint64ToHex,
  xflToHex,
  xrpAddressToHex,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'
import { getEmittedTxnID } from '../../utils/misc'

// accept: Treasury: Hook Set Successfully.
// accept: Treasury: ClaimReward Successful.

// What happens if the user does not blackhole their account?
// How are you going to check that the account is blackholed?
// You are checking last ledger before the claim. Therefore the user cannot claim when their funds are locked.

// Treasury
// rollback: Treasury: HookOn field is incorrectly set.
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
// accept: Treasury: Claimed successfully.
// accept: Treasury: Allowing SetHook Transaction.
// rollback: Treasury: HookOn field is incorrectly set.
// rollback: Treasury: Specify The Amount To Withdraw.
// rollback: Treasury: Outgoing transaction exceeds the amount limit set by you.
// rollback: Treasury: You must wait 000002 ledgers -- NOT TESTABLE
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
      hookOnArray: ['Invoke', 'SetHook'],
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
  hookResult: string
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
    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      testContext.client,
      result.meta as TransactionMetadata
    )
    expect(hookExecutions.executions[0].HookReturnString).toMatch(hookResult)
    await close(testContext.client)
  } catch (error: any) {
    expect(error.message).toMatch(hookResult)
  }
}

export const treasuryClaim = async (
  testContext: XrplIntegrationTestContext,
  hookResult: string
) => {
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
  const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
    testContext.client,
    result.meta as TransactionMetadata
  )
  expect(hookExecutions.executions[0].HookReturnString).toMatch(hookResult)
  await close(testContext.client)
  const mintHash = getEmittedTxnID(result.meta)
  const mintExecutions = await ExecutionUtility.getHookExecutionsFromTx(
    testContext.client,
    mintHash
  )
  console.log(mintExecutions.executions[0].HookReturnString)

  // expect(mintExecutions.executions[0].HookReturnString).toMatch(mintResult)
  await close(testContext.client)
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

  // it("rollback: Misconfigured. Amount 'A' not set as Hook parameter.", async () => {
  //   const hook1Param1 = new iHookParamEntry(
  //     new iHookParamName('A'),
  //     new iHookParamValue('00000000000000', true)
  //   )
  //   const hook1Param2 = new iHookParamEntry(
  //     new iHookParamName('L'),
  //     new iHookParamValue(flipHex(uint32ToHex(4)), true)
  //   )
  //   const hook1Param3 = new iHookParamEntry(
  //     new iHookParamName('D'),
  //     new iHookParamValue(
  //       xrpAddressToHex(testContext.alice.classicAddress),
  //       true
  //     )
  //   )
  //   const acct1hook1 = createHookPayload({
  //     version: 0,
  //     createFile: 'treasury',
  //     namespace: 'treasury',
  //     flags: SetHookFlags.hsfOverride,
  //     hookOnArray: ['Invoke', 'SetHook'],
  //     hookParams: [
  //       hook1Param1.toXrpl(),
  //       hook1Param2.toXrpl(),
  //       hook1Param3.toXrpl(),
  //     ],
  //   })
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.judy.seed,
  //     hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
  //   } as SetHookParams)

  //   // Amount Equal to Limit
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     "Treasury: Misconfigured. Amount 'A' not set as Hook parameter."
  //   )
  // })

  // it('rollback: Treasury: Invalid amount.', async () => {
  //   const hook1Param1 = new iHookParamEntry(
  //     new iHookParamName('A'),
  //     new iHookParamValue(xflToHex(0), true)
  //   )
  //   const hook1Param2 = new iHookParamEntry(
  //     new iHookParamName('L'),
  //     new iHookParamValue(flipHex(uint32ToHex(4)), true)
  //   )
  //   const hook1Param3 = new iHookParamEntry(
  //     new iHookParamName('D'),
  //     new iHookParamValue(
  //       xrpAddressToHex(testContext.alice.classicAddress),
  //       true
  //     )
  //   )
  //   const acct1hook1 = createHookPayload({
  //     version: 0,
  //     createFile: 'treasury',
  //     namespace: 'treasury',
  //     flags: SetHookFlags.hsfOverride,
  //     hookOnArray: ['Invoke', 'SetHook'],
  //     hookParams: [
  //       hook1Param1.toXrpl(),
  //       hook1Param2.toXrpl(),
  //       hook1Param3.toXrpl(),
  //     ],
  //   })
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.judy.seed,
  //     hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
  //   } as SetHookParams)

  //   // Amount Equal to Limit
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     'Treasury: Invalid amount.'
  //   )
  // })

  // it("rollback: Treasury: You don't want to set it to 10M plus XAH!", async () => {
  //   const hook1Param1 = new iHookParamEntry(
  //     new iHookParamName('A'),
  //     new iHookParamValue(xflToHex(10000000), true)
  //   )
  //   const hook1Param2 = new iHookParamEntry(
  //     new iHookParamName('L'),
  //     new iHookParamValue(flipHex(uint32ToHex(4)), true)
  //   )
  //   const hook1Param3 = new iHookParamEntry(
  //     new iHookParamName('D'),
  //     new iHookParamValue(
  //       xrpAddressToHex(testContext.alice.classicAddress),
  //       true
  //     )
  //   )
  //   const acct1hook1 = createHookPayload({
  //     version: 0,
  //     createFile: 'treasury',
  //     namespace: 'treasury',
  //     flags: SetHookFlags.hsfOverride,
  //     hookOnArray: ['Invoke', 'SetHook'],
  //     hookParams: [
  //       hook1Param1.toXrpl(),
  //       hook1Param2.toXrpl(),
  //       hook1Param3.toXrpl(),
  //     ],
  //   })
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.judy.seed,
  //     hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
  //   } as SetHookParams)

  //   // Amount Equal to Limit
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     "Treasury: You don't want to set it to 10M plus XAH!"
  //   )
  // })

  // it("rollback: Treasury: Misconfigured. Ledger limit 'L' not set as Hook parameter.", async () => {
  //   const hook1Param1 = new iHookParamEntry(
  //     new iHookParamName('A'),
  //     new iHookParamValue(xflToHex(10000000), true)
  //   )
  //   const hook1Param2 = new iHookParamEntry(
  //     new iHookParamName('L'),
  //     new iHookParamValue(flipHex(uint64ToHex(BigInt(4))), true)
  //   )
  //   const hook1Param3 = new iHookParamEntry(
  //     new iHookParamName('D'),
  //     new iHookParamValue(
  //       xrpAddressToHex(testContext.alice.classicAddress),
  //       true
  //     )
  //   )
  //   const acct1hook1 = createHookPayload({
  //     version: 0,
  //     createFile: 'treasury',
  //     namespace: 'treasury',
  //     flags: SetHookFlags.hsfOverride,
  //     hookOnArray: ['Invoke', 'SetHook'],
  //     hookParams: [
  //       hook1Param1.toXrpl(),
  //       hook1Param2.toXrpl(),
  //       hook1Param3.toXrpl(),
  //     ],
  //   })
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.judy.seed,
  //     hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
  //   } as SetHookParams)

  //   // Amount Equal to Limit
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     "Treasury: You don't want to set it to 10M plus XAH!"
  //   )
  // })

  // it('rollback: Treasury: Ledger limit must be greater than 324,000(15 days).', async () => {
  //   const hook1Param1 = new iHookParamEntry(
  //     new iHookParamName('A'),
  //     new iHookParamValue(xflToHex(10), true)
  //   )
  //   const hook1Param2 = new iHookParamEntry(
  //     new iHookParamName('L'),
  //     new iHookParamValue(flipHex(uint32ToHex(2)), true)
  //   )
  //   const hook1Param3 = new iHookParamEntry(
  //     new iHookParamName('D'),
  //     new iHookParamValue(
  //       xrpAddressToHex(testContext.alice.classicAddress),
  //       true
  //     )
  //   )
  //   const acct1hook1 = createHookPayload({
  //     version: 0,
  //     createFile: 'treasury',
  //     namespace: 'treasury',
  //     flags: SetHookFlags.hsfOverride,
  //     hookOnArray: ['Invoke', 'SetHook'],
  //     hookParams: [
  //       hook1Param1.toXrpl(),
  //       hook1Param2.toXrpl(),
  //       hook1Param3.toXrpl(),
  //     ],
  //   })
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.judy.seed,
  //     hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
  //   } as SetHookParams)

  //   // Amount Equal to Limit
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     'Treasury: Ledger limit must be greater than 324,000(15 days).'
  //   )
  // })

  // it('rollback: Treasury: Ledger limit must be less than 7,884,000(365 days).', async () => {
  //   const hook1Param1 = new iHookParamEntry(
  //     new iHookParamName('A'),
  //     new iHookParamValue(xflToHex(10), true)
  //   )
  //   const hook1Param2 = new iHookParamEntry(
  //     new iHookParamName('L'),
  //     new iHookParamValue(flipHex(uint32ToHex(5)), true)
  //   )
  //   const hook1Param3 = new iHookParamEntry(
  //     new iHookParamName('D'),
  //     new iHookParamValue(
  //       xrpAddressToHex(testContext.alice.classicAddress),
  //       true
  //     )
  //   )
  //   const acct1hook1 = createHookPayload({
  //     version: 0,
  //     createFile: 'treasury',
  //     namespace: 'treasury',
  //     flags: SetHookFlags.hsfOverride,
  //     hookOnArray: ['Invoke', 'SetHook'],
  //     hookParams: [
  //       hook1Param1.toXrpl(),
  //       hook1Param2.toXrpl(),
  //       hook1Param3.toXrpl(),
  //     ],
  //   })
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.judy.seed,
  //     hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
  //   } as SetHookParams)

  //   // Amount Equal to Limit
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     'Treasury: Ledger limit must be less than 7,884,000(365 days).'
  //   )
  // })

  // it("rollback: Treasury: Misconfigured. Destination 'D' not set as Hook parameter.", async () => {
  //   const hook1Param1 = new iHookParamEntry(
  //     new iHookParamName('A'),
  //     new iHookParamValue(xflToHex(10), true)
  //   )
  //   const hook1Param2 = new iHookParamEntry(
  //     new iHookParamName('L'),
  //     new iHookParamValue(flipHex(uint32ToHex(4)), true)
  //   )
  //   const hook1Param3 = new iHookParamEntry(
  //     new iHookParamName('D'),
  //     new iHookParamValue('00000000000000000000000000000000000000', true)
  //   )
  //   const acct1hook1 = createHookPayload({
  //     version: 0,
  //     createFile: 'treasury',
  //     namespace: 'treasury',
  //     flags: SetHookFlags.hsfOverride,
  //     hookOnArray: ['Invoke', 'SetHook'],
  //     hookParams: [
  //       hook1Param1.toXrpl(),
  //       hook1Param2.toXrpl(),
  //       hook1Param3.toXrpl(),
  //     ],
  //   })
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.judy.seed,
  //     hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
  //   } as SetHookParams)

  //   // Amount Equal to Limit
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     "Treasury: Misconfigured. Destination 'D' not set as Hook parameter."
  //   )
  // })

  // it('rollback: Treasury: The Set Destination Account Does Not Exist.', async () => {
  //   const hook1Param1 = new iHookParamEntry(
  //     new iHookParamName('A'),
  //     new iHookParamValue(xflToHex(10), true)
  //   )
  //   const hook1Param2 = new iHookParamEntry(
  //     new iHookParamName('L'),
  //     new iHookParamValue(flipHex(uint32ToHex(4)), true)
  //   )
  //   const hook1Param3 = new iHookParamEntry(
  //     new iHookParamName('D'),
  //     new iHookParamValue('0000000000000000000000000000000000000000', true)
  //   )
  //   const acct1hook1 = createHookPayload({
  //     version: 0,
  //     createFile: 'treasury',
  //     namespace: 'treasury',
  //     flags: SetHookFlags.hsfOverride,
  //     hookOnArray: ['Invoke', 'SetHook'],
  //     hookParams: [
  //       hook1Param1.toXrpl(),
  //       hook1Param2.toXrpl(),
  //       hook1Param3.toXrpl(),
  //     ],
  //   })
  //   await setHooksV3({
  //     client: testContext.client,
  //     seed: testContext.judy.seed,
  //     hooks: [{ Hook: acct1hook1 }, { Hook: {} }],
  //   } as SetHookParams)

  //   // Amount Equal to Limit
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     'Treasury: The Set Destination Account Does Not Exist.'
  //   )
  // })

  // it('accept: Treasury: Specify The Amount To Withdraw.', async () => {
  //   await hookBeforeAll(testContext)

  //   try {
  //     const tx: Invoke = {
  //       TransactionType: 'Invoke',
  //       Account: testContext.judy.classicAddress,
  //       Destination: testContext.alice.classicAddress,
  //     }
  //     await Xrpld.submit(testContext.client, {
  //       tx: tx,
  //       wallet: testContext.judy,
  //     })
  //     throw Error('Test Failure')
  //   } catch (error: any) {
  //     expect(error.message).toMatch('Treasury: Specify The Amount To Withdraw.')
  //   }
  // })

  // it('accept: Treasury: Outgoing transaction exceeds the amount limit set by you.', async () => {
  //   await hookBeforeAll(testContext)

  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     100,
  //     'Outgoing transaction exceeds the amount limit set by you.'
  //   )
  // })

  // it('accept: Treasury: You must wait 0000001 ledgers.', async () => {
  //   await hookBeforeAll(testContext)

  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     'Treasury: Released successfully.'
  //   )

  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     'You must wait 0000001 ledgers.'
  //   )
  // })

  // it('accept: Treasury: Released successfully.', async () => {
  //   await hookBeforeAll(testContext)

  //   // Amount Less than Limit
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     'Treasury: Released successfully.'
  //   )
  // })

  // it('accept: Treasury: Claimed successfully.', async () => {
  //   await hookBeforeAll(testContext)

  //   // Successful Release
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     'Treasury: Released successfully.'
  //   )

  //   // Successful Claim
  //   await treasuryClaim(testContext, 'Treasury: Claimed successfully.')
  // })

  // it('accept: Treasury: SetHook Failure.', async () => {
  //   await hookBeforeAll(testContext)

  //   // Successful Release
  //   await treasuryPayment(
  //     testContext,
  //     testContext.alice.classicAddress,
  //     10,
  //     'Treasury: Released successfully.'
  //   )

  //   // Failed SetHook
  //   try {
  //     const hook = {
  //       CreateCode: '',
  //       Flags: SetHookFlags.hsfOverride | SetHookFlags.hsfNSDelete,
  //     } as iHook
  //     const tx: SetHook = {
  //       TransactionType: `SetHook`,
  //       Account: testContext.judy.address,
  //       Hooks: [{ Hook: hook }],
  //     }
  //     await Xrpld.submit(testContext.client, {
  //       tx: tx,
  //       wallet: testContext.judy,
  //     })
  //   } catch (error: any) {
  //     expect(error.message).toMatch(
  //       'Treasury: HookOn field is incorrectly set.'
  //     )
  //   }
  // })
})
