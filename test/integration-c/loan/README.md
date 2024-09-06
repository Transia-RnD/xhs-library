# Loans Hook Documentation

## Overview

The Loans Hook is a smart contract designed to facilitate various loan functionalities within a decentralized finance (DeFi) ecosystem. It encompasses multiple components, including the Pool, Loan, and P2PLoan, each serving distinct purposes. This documentation provides an overview of the functions of each component, the admin aspect, and a detailed process of how loans are managed, including the use of URITokens.

## XLS Inspiration

- [XLS-66: Loans](https://github.com/XRPLF/XRPL-Standards/discussions/190)
- [XLS-65: Pools](https://github.com/XRPLF/XRPL-Standards/discussions/192)

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

The P2PLoan component manages individual loan requests and their lifecycle, serving as a central location for all loans while they are pending. Its primary functions include:

- **Create P2PLoan**: Initiate a peer-to-peer loan request. Borrowers can specify the amount and terms of the loan.
  
- **Update P2PLoan**: Modify the terms or status of an existing P2PLoan. This allows borrowers to adjust their requests based on feedback or changing circumstances.

- **Delete P2PLoan**: Remove a P2PLoan request if it is no longer needed or if the borrower decides not to proceed.

This structure ensures that all pending loans are centralized, facilitating a streamlined approval process and enhancing transparency within the lending ecosystem.

## Admin Aspect

The admin aspect of the Loans Hook is essential for overseeing the loan approval process, ensuring that loans are evaluated fairly and transparently. This section outlines the two management structures available for loan approvals and emphasizes the importance of the collective decision-making process involved in approving loans.

### Management Structures

1. **Pool Managed**: 
   - In this structure, each member of the pool has one vote to approve loans. This democratic approach allows all members to participate in the decision-making process, fostering a sense of community and shared responsibility. The collective voting ensures that loan approvals are not solely based on the discretion of a few individuals but rather reflect the consensus of the entire pool. This can lead to more balanced and well-considered decisions regarding loan viability.

2. **Manager Managed**: 
   - In contrast, this structure centralizes the approval process by assigning the responsibility to designated managers. Each manager has one vote to approve loans, which streamlines the decision-making process. This approach can be beneficial in situations where quick decisions are necessary, as it reduces the time required to gather input from multiple pool members. However, it may limit the democratic participation seen in the pool-managed structure.

### Pool Approve Loan

Once a P2PLoan is created, the next critical step is the **Pool Approve Loan** phase. This step involves the following key aspects:

- **Review Process**: After a borrower submits a P2PLoan request, the pool members or managers (depending on the management structure) review the loan details, including the amount requested, the terms proposed, and any associated risks. This review process is vital for assessing the loan's viability and ensuring that it aligns with the pool's lending criteria.

- **Collective Decision-Making**: The approval process is designed to be collaborative. In a pool-managed structure, each member has the opportunity to express their opinion and vote on the loan request. This collective decision-making process helps to mitigate risks, as multiple perspectives are considered before reaching a conclusion. It also enhances transparency, as all members are involved in the evaluation of loan requests.

- **Initial Approval**: The outcome of the pool's review results in an initial approval or rejection of the loan request. If the loan is approved, it signifies that the pool believes the loan is a viable opportunity and is willing to proceed to the next step, where the borrower will provide their final approval. This initial approval is crucial as it sets the stage for the borrower to finalize their commitment to the loan terms.

- **Impact on Borrower Approval**: The pool's decision to approve the loan is a prerequisite for the borrower to give their final approval. This means that the borrower must wait for the pool's consensus before they can proceed with sending collateral and confirming their acceptance of the loan terms. This step ensures that both parties—the pool and the borrower—are aligned on the loan's conditions before any funds are disbursed.

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