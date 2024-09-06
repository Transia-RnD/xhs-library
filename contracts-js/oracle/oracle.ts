import { arrayEqual, sfAccount, sfBlob } from 'jshooks-api'

const ASSERT = (x: any, code: number) => {
  if (!x) {
    trace('error', 0, false)
    rollback(x.toString(), code)
  }
}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const Hook = (arg: number) => {
  trace('oracle.ts: Called.', 0, false)

  const hookAccid = hook_account() as number[]

  const otxnAccid = otxn_field(sfAccount) as number[]

  if (!arrayEqual(hookAccid, otxnAccid)) {
    return rollback('oracle.ts: Invalid OTXN `Account`', 18)
  }

  const txnId = otxn_id(0) as number[]
  ASSERT(txnId.length === 32, 23)

  ASSERT(otxn_slot(1) === 1, 25)
  ASSERT(slot_subfield(1, sfBlob, 2) === 2, 26)

  const buffer = slot(2) as number[]
  ASSERT(buffer.length > 0, 39)

  let len = buffer[0]
  let ptr = 1
  if (len > 192) {
    len = 193 + (len - 193) * 256 + buffer[1]
    ptr++
  }

  const end = ptr + len
  while (ptr < end) {
    const hash = util_sha512h(buffer.slice(ptr, ptr + 40)) as number[]

    ASSERT(state_set(buffer.slice(ptr + 40, ptr + 48), hash) === 8, 42)
    ptr += 48
  }

  return accept('oracle.c: Updated.', 46)
}

// REQUIRED FOR ESBUILD
export { Hook }
