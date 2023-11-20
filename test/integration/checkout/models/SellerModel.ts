import {
  BaseModel,
  Metadata,
  XFL,
  XRPAddress,
} from '@transia/hooks-toolkit/dist/npm/src/libs/binary-models'

export class SellerModel extends BaseModel {
  price: XFL // Price as XFL
  seller: XRPAddress // Issuer

  constructor(price: XFL, seller: XRPAddress) {
    super()
    this.price = price
    this.seller = seller
  }

  getMetadata(): Metadata {
    return [
      { field: 'price', type: 'xfl' },
      { field: 'seller', type: 'xrpAddress' },
    ]
  }

  toJSON() {
    return {
      price: this.price,
      seller: this.seller,
    }
  }
}
