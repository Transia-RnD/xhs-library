import { AccountSetAsfFlags, Client, Wallet } from '@transia/xrpl'
import {
  fund,
  balance,
  ICXRP,
  accountSet,
  sell,
  limit,
  trust,
  pay,
  IC,
  Account,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'
import { GW_WALLET } from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers/constants'

async function main() {
  const wallet = Wallet.fromSeed('saB71peqY7qA567nVG8ktC4gQa9CW')

  const userAccounts = [
    'gw',
    'alice',
    'bob',
    'carol',
    'dave',
    'elsa',
    'frank',
    'grace',
    'heidi',
    'ivan',
    'judy',
  ]
  const userWallets = userAccounts.map((account) => new Account(account))

  const serverUrl = 'wss://xahau-test.net'
  const client = new Client(serverUrl)
  await client.connect()
  client.networkID = await client.getNetworkID()

  const USD = IC.gw('USD', GW_WALLET.classicAddress)
  // FUND GW
  const gwWallet = GW_WALLET
  if ((await balance(client, gwWallet.classicAddress)) == 0) {
    await fund(client, wallet, new ICXRP(100000000), gwWallet.classicAddress)
    await accountSet(client, gwWallet, AccountSetAsfFlags.asfDefaultRipple)
    await sell(client, USD.set(20000), gwWallet, 0.8)
  }

  const needsFunding: string[] = []
  const needsLines: Wallet[] = []
  const needsIC: string[] = []

  for (let i = 1; i < userWallets.length; i++) {
    const wallet = userWallets[i]

    if ((await balance(client, wallet.wallet.classicAddress)) < 100000000) {
      needsFunding.push(wallet.wallet.classicAddress)
    }

    if ((await limit(client, wallet.wallet.classicAddress, USD)) < 100000) {
      needsLines.push(wallet.wallet)
    }

    if ((await balance(client, wallet.wallet.classicAddress, USD)) < 10000) {
      needsIC.push(wallet.wallet.classicAddress)
    }
  }

  await fund(client, wallet, new ICXRP(100), ...needsFunding)
  await trust(client, USD.set(100000), ...needsLines)
  await pay(client, USD.set(50000), gwWallet, ...needsIC)
}
main()
