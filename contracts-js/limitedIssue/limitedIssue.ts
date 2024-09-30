import { Payment, Transaction } from '@transia/xahau-models'
import {
  assert,
  DOESNT_EXIST,
  encodeString,
  sfAccount,
  encodeArray,
  hexToUInt64,
} from 'jshooks-api'

export function flipHex(hexString: string): string {
  let flippedHex = ''
  for (let i = hexString.length - 2; i >= 0; i -= 2) {
    flippedHex += hexString.slice(i, i + 2)
  }
  return flippedHex
}

export function hexToXfl(hex: string): any {
  if (hex === '0000000000000000') {
    return 0
  }
  const value = flipHex(hex)
  return hexToUInt64(value.slice(0, 16))
}

export function flipXfl(endian: bigint): string {
  const hexString = endian.toString(16).toUpperCase()
  let flippedHex = ''
  for (let i = hexString.length - 2; i >= 0; i -= 2) {
    flippedHex += hexString.slice(i, i + 2)
  }
  return flippedHex
}

export function xflToHex(value: any): string {
  if (value === 0) {
    return '0000000000000000'
  }
  return flipXfl(value)
}

export function getState(key: string): number[] {
  return state(encodeString(key))
}

export function setStateXFL(xfl: number, key: string): number {
  return state_set(xflToHex(xfl), encodeString(key))
}

export function setStateHex(hex: string, key: string): number {
  return state_set(hex, encodeString(key))
}

export function setStateArray(array: number[], key: string): number {
  return state_set(encodeArray(array), encodeString(key))
}

export function getOtxnParam(key: string): number[] | number {
  return otxn_param(encodeString(key))
}

export function float_minus(value1: number, value2: number): number {
  return float_sum(value1, float_negate(value2))
}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const Hook = (arg: number) => {
  // Get the transaction
  const txn = otxn_json() as Transaction

  // Get the max amount from state
  const maxAmount = getState('MA')

  switch (txn.TransactionType) {
    // If the transaction is a Payment
    case 'Payment':
      // Check if the max amount is set
      if (typeof maxAmount === 'number' && maxAmount === DOESNT_EXIST) {
        return rollback('limitedIssue.ts: Max Amount not set.', 10)
      }

      // Get the max amount in XFL
      const maxXfl = hexToXfl(encodeArray(maxAmount))

      // Get the outstanding amount in XFL
      const outstandingXfl = hexToXfl(encodeArray(getState('OA')))

      // Get the amount of the transaction in XFL
      const amount = float_set(0, Number(txn.Amount.value))

      // Check if the transaction is outgoing
      const isOutgoing = txn.Account === txn.Destination

      // Check if the new outstanding exceeds the limit
      const newXfl = isOutgoing
        ? float_sum(outstandingXfl, amount)
        : float_minus(outstandingXfl, amount)
      if (float_compare(newXfl, maxXfl, 4)) {
        return rollback('limitedIssue.ts: Exceeds the limit.', 20)
      }

      // Update the outstanding amount
      setStateXFL(newXfl, 'OA')
      return accept('limitedIssue.c: Updated.', 46)

    // If the transaction is a Invoke
    case 'Invoke':
      // Check if the max amount is set
      if (typeof maxAmount === 'number' && maxAmount === DOESNT_EXIST) {
        const paramLimit = assert(getOtxnParam('MA'))
        setStateHex('0000000000000000', 'OA')
        setStateArray(paramLimit, 'MA')
        return accept('limitedIssue.js: Max Amount Added.', 0)
      }

      // If the max amount is already set, reject the transaction
      return rollback('limitedIssue.ts: Cannot update the limit.', 10)

    // If the transaction is any other txn reject it
    default:
      return rollback('limitedIssue.ts: Invalid OTXN `Type`', 10)
  }
}

// REQUIRED FOR ESBUILD
export { Hook }
