# AutoTransfer Hook for the Xahau Ledger

## Overview

The AutoTransfer hook is designed to automate the transfer of assets on the Xahau Ledger. It enables the automatic execution of transfers based on predefined conditions and criteria.

## Features

- **Automated Transfers**: The AutoTransfer hook can automatically execute transfers when certain conditions are met.
- **Configurable Conditions**: Users can set up custom conditions under which the AutoTransfer will occur.

## Usage

To use the AutoTransfer hook, you need to configure the hook with the necessary conditions and deploy it on an account on the Xahau Ledger.

### Setting Up the Hook

1. Create a hook payload with the desired conditions and parameters.
2. Use the `setHooksV3` function to attach the hook to your account.
3. Add the `hsfCollect` flag to the Hook

### Transaction Example

Here is an example of how to set up a transaction with the AutoTransfer hook:

```typescript
// TSH Hook
const asTx: AccountSet = {
  TransactionType: 'AccountSet',
  Account: hookWallet.classicAddress,
  SetFlag: AccountSetAsfFlags.asfTshCollect,
}
await Xrpld.submit(testContext.client, {
  wallet: hookWallet,
  tx: asTx,
})
// URITokenCreateSellOffer
const builtTx1: URITokenCreateSellOffer = {
  TransactionType: 'URITokenCreateSellOffer',
  Account: aliceWallet.classicAddress,
  Amount: xrpToDrops(0),
  Destination: hookWallet.classicAddress,
  URITokenID: uriTokenID,
}

const autoTransferResult = await Xrpld.submit(testContext.client, {
  wallet: aliceWallet,
  tx: autoTransferTx,
});
```

### Hook Logic

The AutoTransfer hook logic is implemented in `autotransfer.c`. It evaluates the conditions from the transaction parameters and executes the transfer if the conditions are satisfied.

### Debugging

To debug the AutoTransfer hook, you can monitor the `xrpld` logs for `HookTrace` entries:

```shell
tail -f xrpld/log/debug.log | grep HookTrace
```

## Contributing

Contributions to the AutoTransfer hook are welcome. Please ensure that you test your changes thoroughly before submitting a pull request.

## License

The AutoTransfer hook is released under GPL-3.0-or-later, which can be found in the LICENSE file.

## Disclaimer

This hook is provided "as is", without warranty of any kind. Use at your own risk.