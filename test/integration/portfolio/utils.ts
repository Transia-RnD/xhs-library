import { LiquidityCheck, RatesInCurrency } from 'xrpl-orderbook-reader'
import { XrplClient } from 'xrpl-client'
import {
  IC,
  sell,
  buy,
  serverUrl,
  setupClient,
  close,
} from '@transia/hooks-toolkit/dist/npm/src/libs/xrpl-helpers'

export async function getSellLiquidity(
  client: XrplClient,
  currency: string,
  issuer: string,
  amount: number
) {
  const request = new LiquidityCheck({
    trade: {
      from: {
        currency: 'XAH',
      },
      amount: amount,
      to: {
        currency: currency,
        issuer: issuer,
      },
    },
    options: {
      rates: RatesInCurrency.from,
      maxSpreadPercentage: 0,
      maxSlippagePercentage: 5,
      maxSlippagePercentageReverse: 5,
    },
    client: client,
  })
  return await request.get()
}

export async function getBuyLiquidity(
  client: XrplClient,
  currency: string,
  issuer: string,
  amount: number
) {
  const request = new LiquidityCheck({
    trade: {
      from: {
        currency: currency,
        issuer: issuer,
      },
      amount: amount,
      to: {
        currency: 'XAH',
      },
    },
    options: {
      rates: RatesInCurrency.to,
      maxSpreadPercentage: 0,
      maxSlippagePercentage: 5,
      maxSlippagePercentageReverse: 5,
    },
    client: client,
  })
  return await request.get()
}

export async function sellAsset() {
  const testContext = await setupClient(serverUrl)
  const XYZ = new IC(testContext.gw.classicAddress, 'XYZ', 100)
  await sell(testContext.client, XYZ.set(1), testContext.gw, 90)
}

export async function buyAsset() {
  const testContext = await setupClient(serverUrl)
  const XYZ = new IC(testContext.gw.classicAddress, 'XYZ', 100)
  await buy(testContext.client, XYZ.set(1), testContext.gw, 105)
}

export async function main(currency: string, issuer: string) {
  const testContext = await setupClient(serverUrl)
  //   await sellAsset()
  await buyAsset()
  await close(testContext.client)
  const client = new XrplClient(serverUrl)
  const sellLiquidity = await getSellLiquidity(client, currency, issuer, 1)
  const buyLiquidity = await getBuyLiquidity(client, currency, issuer, 1)
  console.log(`SELL RATE: ${sellLiquidity.rate}`)
  console.log(`BUY RATE: ${buyLiquidity.rate}`)
}

const currency = 'XYZ'
const issuer = 'rExKpRKXNz25UAjbckCRtQsJFcSfjL9Er3'
main(currency, issuer)
