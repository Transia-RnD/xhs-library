# The Xahau Hook Standard Library

Welcome to the Xahau Hook Standard Library, a collection of standard hooks designed to be used with the Xahau platform. This library is a collaborative effort with Transia DAO, and each hook within the library is associated with a unique Xahau Ledger Request (XLR) for easy reference and tracking.

## Getting Started

To get started with using the Xahau Hook Standard Library, clone this repository and install the dependencies:

```bash
git clone https://github.com/Transia-RnD/xhs-library
cd xhs-library
yarn install
```

## Available Hooks

Below is a list of available hooks in the Xahau Hook Standard Library. Each hook is accompanied by an XLR number for reference. Please follow the links for more information and documentation on how to use each hook.

### Router Hook

- **XLR Number**: XLR-1
- [Router Hook Documentation](test/integration/router/README.md)
- [Transia DAO Collaboration Request](https://dao.transia.co/dashboard/requests/TAmqb1V9UmS5VKU6LcLM/)

### Auction Hook

- **XLR Number**: XLR-3
- [Auction Hook Documentation](test/integration/auction/README.md)
- [Transia DAO Collaboration Request](https://dao.transia.co/dashboard/requests/06Xb0hGJKmqaqjaEVchq/)

### Autotransfer Hook

- **XLR Number**: XLR-5
- [Autotransfer Hook Documentation](test/integration/autotransfer/README.md)
- [Transia DAO Collaboration Request](https://dao.transia.co/dashboard/requests/afZJ95alSQRYCROGlnUY/)

### Checkout Hook

- **XLR Number**: XLR-7
- [Checkout Hook Documentation](test/integration/checkout/README.md)
- [Transia DAO Collaboration Request](https://dao.transia.co/dashboard/requests/wBdJM2k5J0e4z2o9SeKZ/)

_(Repeat the above section for each additional hook that is part of the library, including their respective XLR numbers and links to collaboration requests.)_

## Development

To build the hooks, run the following command:

```bash
yarn run build:hooks
```

To run integration tests:

```bash
yarn run test:integration
```

## Contributing

Contributions to the Xahau Hook Standard Library are welcome. If you would like to contribute or create a new request for collaboration with Transia DAO, please follow these steps:

1. Visit the Transia DAO Requests Portal at [https://dao.transia.co/dashboard/requests](https://dao.transia.co/dashboard/requests).
2. Click on the "New Request" button.
3. Fill out the form with the details of your proposal, including a clear description and any relevant information that can help the community understand your request.
4. Submit the form for review by the Transia DAO community.
5. Engage with the community members in the discussion of your request and provide any additional information if required.

Your request will be reviewed by the community, and you will receive feedback or approval to proceed with your contribution.

## License

This project is licensed under the GPL-3.0-or-later License - see the [LICENSE.md](LICENSE) file for details.

## Authors

- **Denis Angell** - _Initial work_ - [dangell7](https://github.com/dangell7)

xrpld-netgen up:standalone --network_id=21338

tail -f xahau/log/debug.log | grep HookTrace