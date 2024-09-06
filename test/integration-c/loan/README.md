# Loans Hook Documentation

## Overview

The Loans Hook is a smart contract designed to facilitate various loan functionalities within a decentralized finance (DeFi) ecosystem. It encompasses multiple components, including the Pool, Loan, and P2PLoan, each serving distinct purposes. This documentation provides an overview of the functions of each component, the admin aspect, and a detailed process of how loans are managed, including the use of URITokens.

## XLS Inspiration

[XLS-66: Loans](https://github.com/XRPLF/XRPL-Standards/discussions/190)
[XLS-65: Pools](https://github.com/XRPLF/XRPL-Standards/discussions/192)

## Table of Contents

- [Components](#components)
  - [Pool](#pool)
  - [Loan](#loan)
  - [P2PLoan](#p2ploan)
- [Admin Aspect](#admin-aspect)
- [Full Process](#full-process)
  - [Step-by-Step Transactions](#step-by-step)
  - [URIToken Usage](#uritoken-usage)

## Components

### Pool

The Pool component is responsible for managing liquidity and facilitating loan approvals. Its primary functions include:

- **Create Pool**: Establish a new pool for lending and borrowing activities.
- **Deposit Liquidity**: Allow lenders to deposit funds into the pool, increasing available liquidity.
- **Withdraw Liquidity**: Enable lenders to withdraw their funds from the pool.
- **Update Pool**: Modify pool parameters such as management fees, cover amounts, etc.
- **Delete Pool**: Remove a pool from the system when it is no longer needed.

### Loan

The Loan component manages individual loan inside the approved pool. Its primary functions include:

- **Accept Loan**: Called when the borrower makes thier final approval. Records the loan state in the pool.
- **End Loan**: Mark a loan as completed or settled. Pull collateral if loan is in default.
- **Payment Loan**: Process payments made towards the loan.
- **Drawdown Loan**: Allow borrowers to access the loan amount.

### P2PLoan

The P2PLoan component manages individual loan requests and their lifecycle. Its primary functions include:

- **Create P2PLoan**: Initiate a peer-to-peer loan request.
- **Update P2PLoan**: Modify the terms or status of an existing P2PLoan.
- **Delete P2PLoan**: Remove a P2PLoan request from the system.

## Admin Aspect

The admin aspect of the Loans Hook is crucial for managing the approval process of loans. There are two management structures:

1. **Pool Managed**: 
   - Each pool member has one vote to approve loans.
   - The pool collectively decides on loan approvals, ensuring democratic participation.

2. **Manager Managed**: 
   - Each manager has one vote to approve loans.
   - Managers are responsible for overseeing loan approvals, providing a more centralized approach.


## Full Process

The full process of managing loans involves several steps, each with specific transactions. Below is a detailed outline of the process:

### Step-by-Step

1. **Create Pool**: 
   - Description: A new pool is created with specified parameters (withdraw amount, management fee, cover amounts).

2. **Deposit Liquidity**: 
   - Description: Lenders deposit funds into the pool, increasing the available liquidity for loans.

3. **Create Loan**: 
   - Description: A borrower creates a loan specifying the amount and terms. Loans are recorded on the p2p loan hook specifying the amount and terms.

4. **Pool Approve Loan**: 
   - Description: Depending on the management structure, either pool members or managers vote to approve the loan.

5. **Borrower Approve Loan**: 
   - Description: The borrower also has to give final approval, as terms might change. The borrower also sends collateral with their final approval to the pool/loan they are approving.

6. **Drawdown Loan**: 
   - Description: Once approved, the borrower can access the loan amount.

7. **Payment Loan**: 
   - Description: The borrower makes payments towards the loan as per the agreed schedule. Interest is paid directly to the pool.

8. **End Loan**: 
   - Description: The loan is marked as completed once fully paid or collateral is collected if in default.

9. **Withdraw Liquidity**: 
   - Description: Lenders can withdraw their funds from the pool after loans are settled.

### URIToken Usage

The URIToken plays a vital role in the loan process by representing the loan agreement and its status. When a loan is created, a URIToken is generated, which contains the loan details. This token is transferred between parties during the loan lifecycle, ensuring that all transactions are recorded and traceable. The URIToken is also used to update the loan status on the P2P platform when it is transferred, providing real-time visibility into the loan's progress.