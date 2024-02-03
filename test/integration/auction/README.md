# Auction Hook for the Xahau Ledger

## Overview

The Auction Hook is a specialized component designed for the Xahau Ledger to facilitate the creation and management of auctions within the ledger. This hook allows users to automate the auction process, ensuring that the auction rules are enforced programmatically and transparently on the blockchain.

## Features

- **Auction Creation**: Users can create auctions for assets with predefined starting conditions.
- **Bid Management**: The hook manages incoming bids, ensuring they meet the auction criteria such as minimum bid increments.
- **Auction Finalization**: Automatically finalizes the auction when the end conditions are met, such as time expiration or a buyout bid.
- **Configurable Conditions**: Auction parameters such as start time, end time, minimum bid, and buyout price can be configured.

## Usage

To utilize the Auction Hook, you must set up the hook with the necessary auction parameters and deploy it to an account on the Xahau Ledger.

### Setting Up the Hook

1. Create a hook payload with the desired auction parameters.
2. Use the `setHooksV3` function to attach the hook to your account.
3. Add the `hsfCollect` flag to the Hook to enable collection of fees if applicable.

### Transaction Example

Here is an example of how to set up a transaction with the Auction hook:

```typescript
const acct1hook1 = createHookPayload(
    0,
    'auction_start',
    'auction',
    SetHookFlags.hsfCollect + SetHookFlags.hsfOverride,
    ['URITokenCreateSellOffer']
)
const acct1hook2 = createHookPayload(
    0,
    'auction',
    'auction',
    SetHookFlags.hsfOverride,
    ['EscrowCreate', 'Invoke']
)
await setHooksV3({
    client: testContext.client,
    seed: hookWallet.seed,
    hooks: [{ Hook: acct1hook1 }, { Hook: acct1hook2 }],
} as SetHookParams)
```

## Hook Logic

The Auction hook logic is implemented in `auction.c`. It evaluates the auction parameters and bid conditions from the transaction parameters and manages the auction process accordingly.

## Debugging

To debug the Auction hook, you can monitor the xrpld logs for `HookTrace` entries:

```shell
tail -f xahau/log/debug.log | grep HookTrace
```

## Contributing

Contributions to the Auction hook are welcome. Please ensure that you test your changes thoroughly before submitting a pull request.

## License

The Auction hook is released under GPL-3.0-or-later, which can be found in the LICENSE file.

## Disclaimer

This hook is provided "as is", without warranty of any kind. Use at your own risk.