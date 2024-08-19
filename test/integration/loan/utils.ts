// xrpl
import {
  Client,
  Wallet,
  Invoke,
  Payment,
  TransactionMetadata,
  convertStringToHex,
  Remit,
} from '@transia/xrpl'
import { hashURIToken } from '@transia/xrpl/dist/npm/utils/hashes'
// xrpl-helpers
import {
  XrplIntegrationTestContext,
  IC,
  close,
  balance,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
// src
import {
  Xrpld,
  iHookParamEntry,
  iHookParamName,
  iHookParamValue,
  ExecutionUtility,
  StateUtility,
  xrpAddressToHex,
  padHexString,
  hexNamespace,
  decodeModel,
  uint64ToHex,
  generateHash,
} from '@transia/hooks-toolkit/dist/npm/src'
import { Fees, PoolDelegateCover, PoolModel, Withdraw } from './models/Pool'
import { IssuedCurrencyAmount } from '@transia/xrpl/dist/npm/models/common'
import { IssueModel } from './models/utils/IssueModel'
import { LoanModel } from './models/Loan'
import { AmountModel } from './models/utils/AmountModel'

export async function getPool(
  testContext: XrplIntegrationTestContext,
  hookWallet: Wallet
) {
  try {
    const state = await StateUtility.getHookState(
      testContext.client,
      hookWallet.classicAddress,
      padHexString(xrpAddressToHex(hookWallet.classicAddress)),
      hexNamespace('pool')
    )
    const decoded = decodeModel(state.HookStateData, PoolModel)
    console.log(decoded)
    return true
  } catch (error) {
    return false
  }
}

export async function getAdminMemberCount(
  testContext: XrplIntegrationTestContext,
  hookWallet: Wallet
) {
  try {
    const state = await StateUtility.getHookState(
      testContext.client,
      padHexString(convertStringToHex('MC')),
      padHexString(xrpAddressToHex(hookWallet.classicAddress)),
      hexNamespace('admin')
    )
    console.log(state.HookStateData)

    return state.HookStateData
  } catch (error) {
    return 0
  }
}

export async function createPool(
  client: Client,
  wallet: Wallet,
  issuer: Wallet,
  lenders: Wallet[],
  borrowers: Wallet[],
  withdrawAmount: number,
  managementFee: number,
  coverMin: number,
  coverMax: number
) {
  // const lendgerPermissions = new Permission(0, [])
  // const borrowPermissions = new Permission(0, [])
  const poolWithdraw = new Withdraw(0, withdrawAmount, new Fees(managementFee))
  const issue = new IssueModel(issuer.classicAddress, 'USD')
  const cover = new PoolDelegateCover(coverMin, coverMax)
  const poolModel = new PoolModel(0, 0, issue, poolWithdraw, cover)
  console.log(poolModel.encode().length / 2)

  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01010000000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('P')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('C')
  )
  const otxn1param4 = new iHookParamEntry(
    new iHookParamName('PM'),
    new iHookParamValue(poolModel.encode().toUpperCase(), true)
  )
  const builtTx1: Invoke = {
    TransactionType: 'Invoke',
    Account: wallet.classicAddress,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
      otxn1param4.toXrpl(),
    ],
  }

  console.log(JSON.stringify([otxn1param1.toXrpl(), otxn1param2.toXrpl()]))
  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'pool.c: Created Pool.'
  )
}

export async function depositLiquidity(
  client: Client,
  wallet: Wallet,
  hook: string,
  amount: IssuedCurrencyAmount
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01010000000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('L')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('D')
  )
  const builtTx1: Payment = {
    TransactionType: 'Payment',
    Account: wallet.classicAddress,
    Destination: hook,
    Amount: amount,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
    ],
  }

  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'pool.c: Transaction Complete (Deposit Liquidity).'
  )

  await close(client)
}

export async function withdrawLiquidity(
  client: Client,
  wallet: Wallet,
  hook: string,
  amount: IssuedCurrencyAmount
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01010000000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('L')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('W')
  )
  const builtTx1: Payment = {
    TransactionType: 'Payment',
    Account: wallet.classicAddress,
    Destination: hook,
    Amount: amount,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
    ],
  }

  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'pool.c: Transaction Complete (Withdraw Liquidity).'
  )

  await close(client)
}

export async function validateBalance(
  client: Client,
  wallet: Wallet,
  ic: IC,
  shouldBe: number
) {
  const navBalance = await balance(client, wallet.classicAddress, ic)
  expect(navBalance).toBe(shouldBe)
}

export async function createP2PLoan(
  client: Client,
  wallet: Wallet,
  hook: string,
  url: string,
  loan: LoanModel
) {
  const hexURL = convertStringToHex(url)
  console.log(hexURL)

  console.log(hexURL.length / 2)

  const param1 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('C')
  )
  const param2 = new iHookParamEntry(
    new iHookParamName('URI'),
    new iHookParamValue(hexURL, true)
  )
  const param3 = new iHookParamEntry(
    new iHookParamName('URIL'),
    new iHookParamValue(uint64ToHex(BigInt(hexURL.length / 2)), true)
  )
  const uriTokenID = hashURIToken(hook, url)
  const param4 = new iHookParamEntry(
    new iHookParamName('URIH'),
    new iHookParamValue(uriTokenID, true)
  )
  const param5 = new iHookParamEntry(
    new iHookParamName('LM'),
    new iHookParamValue(loan.encode(), true)
  )
  const param6 = new iHookParamEntry(
    new iHookParamName('LH'),
    new iHookParamValue(generateHash(Buffer.from(loan.encode(), 'hex')), true)
  )
  const builtTx: Invoke = {
    TransactionType: 'Invoke',
    Account: wallet.classicAddress,
    Destination: hook,
    HookParameters: [
      param1.toXrpl(),
      param2.toXrpl(),
      param3.toXrpl(),
      param4.toXrpl(),
      param5.toXrpl(),
      param6.toXrpl(),
    ],
  }
  const result = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx,
  })
  const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result.meta as TransactionMetadata
  )
  console.log(hookExecutions.executions[0].HookReturnString)
  expect(hookExecutions.executions[0].HookReturnString).toMatch(
    'p2p_loan.c: Created Loan.'
  )

  await close(client)

  return uriTokenID
}

export async function poolVote(
  client: Client,
  wallet: Wallet,
  hook: string,
  hash: string
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01000001000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('T'),
    new iHookParamValue('4C' + '00', true)
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('V'),
    new iHookParamValue(hash, true)
  )
  const builtTx: Invoke = {
    TransactionType: 'Invoke',
    Account: wallet.classicAddress,
    Destination: hook,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
    ],
  }
  const result = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx,
  })
  const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result.meta as TransactionMetadata
  )
  console.log(hookExecutions.executions[1].HookReturnString)
  expect(hookExecutions.executions[1].HookReturnString).toMatch(
    'admin.c: loan approved.'
  )

  await close(client)
}

export async function borrowerRemit(
  client: Client,
  wallet: Wallet,
  hook: string,
  amount: IssuedCurrencyAmount,
  uritokenId: string
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01000100000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('C')
  )
  const builtTx: Remit = {
    TransactionType: 'Remit',
    Account: wallet.classicAddress,
    Destination: hook,
    URITokenIDs: [uritokenId],
    Amounts: [
      {
        // @ts-expect-error -- ignore
        AmountEntry: {
          Amount: amount,
        },
      },
    ],
    HookParameters: [otxn1param1.toXrpl(), otxn1param2.toXrpl()],
  }
  const result = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx,
  })
  const hookExecutions = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result.meta as TransactionMetadata
  )
  console.log(hookExecutions.executions[0].HookReturnString)
  expect(hookExecutions.executions[0].HookReturnString).toMatch(
    'loan.c: Created Loan.'
  )

  await close(client)
}

export async function drawDown(
  client: Client,
  wallet: Wallet,
  hook: string,
  amount: IssuedCurrencyAmount
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01000100000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('L')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('R')
  )
  const otxn1param4 = new iHookParamEntry(
    new iHookParamName('AMT'),
    new iHookParamValue(
      new AmountModel(Number(amount.value), amount.currency, amount.issuer)
        .encode()
        .toUpperCase(),
      true
    )
  )
  const builtTx1: Invoke = {
    TransactionType: 'Invoke',
    Account: wallet.classicAddress,
    Destination: hook,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
      otxn1param4.toXrpl(),
    ],
  }

  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'loan.c: Transaction Success (Receive Loan).'
  )

  await close(client)
}

export async function makePayment(
  client: Client,
  wallet: Wallet,
  hook: string,
  amount: IssuedCurrencyAmount
) {
  const otxn1param1 = new iHookParamEntry(
    new iHookParamName('HPA'),
    new iHookParamValue('0A01000100000000000000', true)
  )
  const otxn1param2 = new iHookParamEntry(
    new iHookParamName('OP'),
    new iHookParamValue('L')
  )
  const otxn1param3 = new iHookParamEntry(
    new iHookParamName('SOP'),
    new iHookParamValue('P')
  )
  const builtTx1: Payment = {
    TransactionType: 'Payment',
    Account: wallet.classicAddress,
    Destination: hook,
    Amount: amount,
    HookParameters: [
      otxn1param1.toXrpl(),
      otxn1param2.toXrpl(),
      otxn1param3.toXrpl(),
    ],
  }

  const result1 = await Xrpld.submit(client, {
    wallet: wallet,
    tx: builtTx1,
  })
  const hookExecutions1 = await ExecutionUtility.getHookExecutionsFromMeta(
    client,
    result1.meta as TransactionMetadata
  )

  expect(hookExecutions1.executions[1].HookReturnString).toMatch(
    'loan.c: Payback Loan.'
  )

  await close(client)
}

export async function getLoan(
  testContext: XrplIntegrationTestContext,
  hookWallet: Wallet
) {
  const state = await StateUtility.getHookState(
    testContext.client,
    hookWallet.classicAddress,
    padHexString(xrpAddressToHex(hookWallet.classicAddress)),
    hexNamespace('loan')
  )
  const decoded = decodeModel(state.HookStateData, LoanModel)
  console.log(decoded)
}
