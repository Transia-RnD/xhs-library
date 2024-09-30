var DOESNT_EXIST = -5;

function hexToUInt64(hex) {
  return BigInt(`0x${hex}`);
}
function encodeString(v) {
  let s = "";
  for (let i = 0; i < v.length; i++) {
    s += v.charCodeAt(i).toString(16).padStart(2, "0");
  }
  return s.toUpperCase();
}
var encodeArray = (a) => {
  return a.map((v) => v.toString(16).padStart(2, "0")).join("").toUpperCase();
};
var assert = (data) => {
  if (typeof data === "number" && data < 0) {
    rollback("assert.error", data);
  }
  return data;
};

function flipHex(hexString) {
  let flippedHex = "";
  for (let i = hexString.length - 2; i >= 0; i -= 2) {
    flippedHex += hexString.slice(i, i + 2);
  }
  return flippedHex;
}
function hexToXfl(hex) {
  if (hex === "0000000000000000") {
    return 0;
  }
  const value = flipHex(hex);
  return hexToUInt64(value.slice(0, 16));
}
function flipXfl(endian) {
  const hexString = endian.toString(16).toUpperCase();
  let flippedHex = "";
  for (let i = hexString.length - 2; i >= 0; i -= 2) {
    flippedHex += hexString.slice(i, i + 2);
  }
  return flippedHex;
}
function xflToHex(value) {
  if (value === 0) {
    return "0000000000000000";
  }
  return flipXfl(value);
}
var Hook = (arg) => {
  trace("limitedIssue.ts: Called.", 0, false);
  const txn = otxn_json();
  const maxAmount = state(encodeString("MA"));
  switch (txn.TransactionType) {
    case "Payment":
      if (typeof maxAmount === "number" && maxAmount === DOESNT_EXIST) {
        return rollback("limitedIssue.ts: Max Amount not set.", 10);
      }
      const maxXfl = hexToXfl(encodeArray(maxAmount));
      const outstandingXfl = hexToXfl(encodeArray(state(encodeString("OA"))));
      const amount = float_set(0, Number(txn.Amount.value));
      const newOutstanding = float_sum(outstandingXfl, amount);
      if (float_compare(newOutstanding, maxXfl, 4)) {
        return rollback("limitedIssue.ts: Exceeds the limit.", 20);
      }
      state_set(xflToHex(newOutstanding), encodeString("OA"));
      return accept("limitedIssue.c: Updated.", 46);
    case "Invoke":
      if (typeof maxAmount === "number" && maxAmount === DOESNT_EXIST) {
        const paramLimit = assert(otxn_param(encodeString("MA")));
        state_set("0000000000000000", encodeString("OA"));
        state_set(encodeArray(paramLimit), encodeString("MA"));
        return accept("limitedIssue.js: Max Amount Added.", 0);
      }
      return rollback("limitedIssue.ts: Cannot update the limit.", 10);
    default:
      return rollback("limitedIssue.ts: Invalid OTXN `Type`", 10);
  }
};