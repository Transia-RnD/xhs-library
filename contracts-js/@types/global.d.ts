/* eslint-disable @typescript-eslint/no-explicit-any */
/* eslint-disable @typescript-eslint/no-unused-vars */

import { Transaction } from '@transia/xahau-models'

declare global {
  /********************************************************************************************************************* */

  // Return number: int64
  //    An arbitrary return code you wish to return from your hook. This will be present in the metadata of the originating transaction.
  export type Hook = (reserved?: number) => bigint /** int64 */

  // Arg: uint32 'what'
  //    0 = the emittted transaction to which this callback relates was successfully accepted into a ledger.
  //    1 = the emitted transaction to which the callback relates was NOT successfully accepted into a ledger before it expired.
  // Return number: int64
  //    An arbitrary return code you wish to return from your hook. This will be present in the metadata of the originating transaction.
  export type Callback = (emittedTxError?: number) => bigint /** int64 */

  /********************************************************************************************************************* */

  type Trace = (message: string, data: any, hex?: boolean) => void

  const trace: Trace

  /********************************************************************************************************************* */

  // Rtrn
  type Accept = (error_msg: string, error_code: number) => bigint

  const accept: Accept

  /********************************************************************************************************************* */

  type Rollback = (error_msg: string, error_code: number) => bigint

  const rollback: Rollback

  /********************************************************************************************************************* */

  const util_raddr = (arg: any) => any
  const util_accid = (arg: any) => any
  const util_sha512h = (arg: any) => any
  const state = (arg: any) => any
  const state_set = (arg: any, arg: any) => any

  // Cleaned DA

  type JSResHookAccount = () => number[] | number
  const hook_account: JSResHookAccount

  declare function otxn_burden(): number
  declare function otxn_generation(): number
  declare function otxn_field(id: number): number[] | number
  declare function otxn_id(flags: number): number[] | number
  declare function otxn_type(): number
  declare function otxn_slot(index: number): number
  declare function otxn_param(key: number[] | string): number[] | number
  declare function otxn_json(): Transaction

  declare function hook_hash(index: number): number[] | number
  declare function hook_again(): number
  declare function fee_base(): number
  declare function ledger_seq(): number
  declare function ledger_last_hash(): number[]
  declare function ledger_last_time(): number
  declare function ledger_nonce(): number[] | number
  declare function ledger_keylet(
    low: number[] | string,
    high: number[] | string
  ): number[] | number

  declare function hook_param(key: number[] | string): nunber[] | number
  declare function hook_param_set(
    value: number[] | string,
    key: number[] | string,
    hash: number[] | string
  ): number
  declare function hook_pos(): number
  declare function hook_skip(hash: number[] | string, flags: number): number

  declare function state(key: number[] | string): number[]
  declare function state_foreign(
    key: number[] | string,
    namespace: number[] | string,
    account: number[] | string
  ): number[] | number
  declare function state_set(
    value: number[] | string,
    key: number[] | string
  ): number
  declare function state_foreign_set(
    value: number[] | string,
    key: number[] | string,
    namespace: number[] | string,
    account: number[] | string
  ): number

  declare function slot(index: number): number[] | number
  declare function slot_clear(index: number): number
  declare function slot_count(index: number): number
  declare function slot_float(index: number): number
  declare function slot_set(key: number[] | string, index: number): number
  declare function slot_size(index: number): number
  declare function slot_subarray(
    index: number,
    id: number,
    next: number
  ): number
  declare function slot_subfield(
    index: number,
    id: number,
    next: number
  ): number
  declare function slot_type(index: number, flags: number): number

  declare function float_set(exponent: number, mantissa: number): number
  declare function float_multiply(float1: number, float2: number): number
  declare function float_mulratio(
    float1: number,
    round_up: number,
    numerator: number,
    denominator: number
  ): number
  declare function float_negate(float1: number): number
  declare function float_compare(
    float1: number,
    float2: number,
    mode: number
  ): number
  declare function float_sum(float1: number, float2: number): number
  declare function float_sto(
    write_ptr: number,
    write_len: number,
    cread_ptr: number,
    cread_len: number,
    iread_ptr: number,
    iread_len: number,
    float1: number,
    field_code: number
  ): number
  declare function float_sto_set(read_ptr: number, read_len: number): number
  declare function float_invert(float1: number): number
  declare function float_divide(float1: number, float2: number): number
  declare function float_one(): number
  declare function float_mantissa(float1: number): number
  declare function float_sign(float1: number): number
  declare function float_int(
    float1: number,
    decimal_places: number,
    abs: number
  ): number
  declare function float_log(float1: number): number
  declare function float_root(float1: number, n: number): number

  declare function sto_emplace(
    raw_sto: number[] | string,
    raw_field: number[] | string,
    field_id: number
  ): number | number[]

  declare function sto_erase(
    raw_sto: number[] | string,
    field_id: number
  ): number | number[]

  declare function sto_subarray(
    raw_sto: number[] | string,
    index: number
  ): number | number[]

  declare function sto_subfield(
    raw_sto: number[] | string,
    field_id: number
  ): number

  declare function sto_validate(raw_sto: number[] | string): number | number[]

  // DA TODO: Does not exist in source, should it?
  // declare function float_exponent(float1: number): number

  declare function util_accid(raddr: any): number | string

  declare function util_keylet(
    keylet_type: number,
    arg1?: number[] | number,
    arg2?: number[] | number,
    arg3?: number[] | number,
    arg4?: number[] | number
  ): number

  declare function util_raddr(acc_id: any): number | string

  declare function util_sha512h(bytes: number[]): number[] | number

  declare function util_verify(
    msg: number[],
    sig: number[],
    pub_key: number[]
  ): number
}

export {}
