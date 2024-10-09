// xrpl
import {
  Client,
  Wallet,
  AccountSetAsfFlags,
  SetHookFlags,
  TransactionMetadata,
  ClaimReward,
} from '@transia/xrpl'
// xrpl-helpers
import { accountSet } from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
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
} from '@transia/hooks-toolkit'
import { xrpAddressToHex } from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

const serverUrl = 'wss://xahau-test.net'
const hookSeed = 'snnf9az3KyNUfVRtER43grtixDrRy'
const destSeed = 'snDnCTwy5k2UdFq7fYnWxiXeEbdfJ'

describe('treasury', () => {
  let client: Client

  beforeAll(async () => {
    client = new Client(serverUrl)
    await client.connect()
    client.networkID = await client.getNetworkID()
    const hookWallet = Wallet.fromSeed(hookSeed)
    const destWallet = Wallet.fromSeed(destSeed)

    accountSet(client, hookWallet, AccountSetAsfFlags.asfTshCollect)
    const hookParam1 = new iHookParamEntry(
      new iHookParamName('DST'),
      new iHookParamValue(xrpAddressToHex(destWallet.classicAddress), true)
    )
    const acct1hook1 = createHookPayload({
      version: 0,
      createFile: 'genesis_mint',
      namespace: 'treasury',
      flags: SetHookFlags.hsfCollect + SetHookFlags.hsfOverride,
      hookOnArray: ['GenesisMint'],
      hookParams: [hookParam1.toXrpl()],
    })
    await setHooksV3({
      client: client,
      seed: hookWallet.seed,
      hooks: [{ Hook: acct1hook1 }],
    } as SetHookParams)
  })

  afterAll(async () => {
    client.disconnect()
  })

  it('treasury - success', async () => {
    client.networkID = await client.getNetworkID()
    const hookWallet = Wallet.fromSeed(hookSeed)

    // Opt In ClaimReward
    const optInTx: ClaimReward = {
      TransactionType: 'ClaimReward',
      Account: hookWallet.classicAddress,
      Issuer: 'rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh',
    }
    await Xrpld.submit(client, {
      wallet: hookWallet,
      tx: optInTx,
      debugStream: true,
    })

    // Wait 100 Seconds
    await new Promise((resolve) => setTimeout(resolve, 100000)) // await

    // ClaimReward
    const claimTx: ClaimReward = {
      TransactionType: 'ClaimReward',
      Account: hookWallet.classicAddress,
      Issuer: 'rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh',
    }
    const result = await Xrpld.submit(client, {
      wallet: hookWallet,
      tx: claimTx,
      debugStream: true,
    })

    const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
      client,
      result.meta as TransactionMetadata
    )
    console.log(hookExecutions)

    expect(hookExecutions.executions[0].HookReturnString).toMatch(
      'auction_start.c: Tx emitted success'
    )
  })
})
