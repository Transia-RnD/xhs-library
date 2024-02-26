# Router Hook for the Xahau Ledger

## Overview

The router hook is designed to manage the execution of hooks on the Xahau Ledger. It allows for the conditional skipping of hooks based on their position in the execution sequence.

## Features

- **Conditional Execution**: The router hook can determine whether to execute or skip a hook based on its position.
- **Dynamic Hook Management**: Hooks can be dynamically managed without altering the core logic of the router hook.

## Usage

To use the router hook, you need to set up the hook with the appropriate parameters and deploy it on an account on the Xahau Ledger.

### Setting Up the Hook

1. Create a hook payload with the desired flags and parameters.
2. Use the `setHooksV3` function to attach the hook to your account.
3. Define the hook positions using the `HookPosModel` and encode them into a `HookPosArray`.

### Transaction Example

Here is an example of how to invoke a transaction with the router hook:

```typescript
// Invoke
const otxn1param1 = new iHookParamEntry(
  new iHookParamName('HPA'),
  new iHookParamValue("0A01000101000000000000", true)
)
const builtTx1: Invoke = {
  TransactionType: 'Invoke',
  Account: aliceWallet.classicAddress,
  Destination: hookWallet.classicAddress,
  HookParameters: [otxn1param1.toXrpl()],
};

const result1 = await Xrpld.submit(testContext.client, {
  wallet: aliceWallet,
  tx: builtTx1,
});
```

### Hook Logic

The router hook logic is implemented in `router_base.c`. It reads the hook positions from the transaction parameters and decides whether to skip or execute each hook based on its position.

### Debugging

To debug the router hook, you can monitor the `xrpld` logs for `HookTrace` entries:

```shell
tail -f xahau/log/debug.log | grep HookTrace
```

## Contributing

Contributions to the router hook are welcome. Please ensure that you test your changes thoroughly before submitting a pull request.

## License

The router hook is released under GPL-3.0-or-later, which can be found in the LICENSE file.

## Disclaimer

This hook is provided "as is", without warranty of any kind. Use at your own risk.