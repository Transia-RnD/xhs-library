# Pool
1. Create Pool
- Deposit
- Withdraw
2. Update Pool
3. Delete Pool
# Loan
1. Create Loan
2. Update Loan
3. Delete Loan
4. End Loan
4. Payment Loan
5. Drawdown Loan
5. Surity Loan (Collateral)

# System Creates an agnostic Loan Hook
1. Hook will take in parameters and emit a URIToken.
# Setup Pool
1. UserB Creates Pool (Deploy Hook)
2. UserB Deposits into Pool
# Create Loan
1. UserA Creates URIToken w/ Loan Details through the agnostic hook
# Approve Loan (Pool)
1. UserB Votes to Approve Loan
# Approve Loan (Borrower)
1. UserA Remits (Amount (collateral) + URIToken (loan details)) to Pool/Loan
- Only allow the approval if the agnostic hook owns the URIToken
- Creates Loan in Loan Hook
- Pool/Loan owns the Loan(URIToken)
- Pool owns collateral
- Create Loan & Collateral become 1 operation
# Receive Funds
1. UserB Pool Emits to UserA (Drawdown/Optional)
2. UserB Draws down the loan manually (invoke operation)
# Payback / End
1. As usual. Operations exist on the Pool/Loan
