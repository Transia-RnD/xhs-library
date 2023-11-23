

<!-- # Prerequisits

# Accounts in hooks builder.

Navigate to https://hooks-builder.xrpl.org/deploy

## Import Alice
- Click Import under "Accounts" in the bottom left
- Enter "Alice" for the name and "rG1QQv2nh2gr7RCZ1P8YYcBUKCCN633jCn"

## Import Bob
- Click Import under "Accounts" in the bottom left
- Enter "Bob" for the name and "rPMh7Pi9ct699iZUTWaytJUoHcJ7cgyziK" -->

# Prerequisits

# Introduction to Hooks Builder

- Explain Develop
- Explain Deploy
- Explain Test

# Accounts in hooks builder.

Navigate to https://hooks-builder.xrpl.org

> You should have 1 account already named "Alice"

- Click Create under "Accounts" in the bottom left
- Enter a "Bob" for the Account

# Lesson #1

## Step 1

1. Open the hooks builder to https://hooks-builder.xrpl.org/develop/b16d9dd9c1085278ee9e487dd6a96186
2. Click "Compile to wasm"
3. Click "Develop" top right
4. Click "Set Hook" under "Alice" account

- Select the Payment type under "Invoke on transactions"

5. Click "Suggest" under Fee
6. Click "SetHook"

> Review the sethook operation

## Step 1.1

1. Set the Hook on Bob

> Review the Fee, review the HookDefinition.

## Step 2

1. Click "Test" top right
2. Under Account select Alice
2. Under TransactionType select Payment
3. Under Destination select Bob
4. Under Fee click "Suggest"
5. Click "Run Test"

> Review the debug, and the rejected payment operation, and the hook execution
> Copy the HookExecutionString, paste into the [Hex Converter](https://www.rapidtables.com/convert/number/hex-to-ascii.html)

## Step 3

1. Click "Develop" top right
2. Under Account select Alice
2. Under Hook Parameters enter
 - Name: AMT
 - Value: 0080C6A47E8DC354
3. Show user how to get that value
 - Navigate to the [XFL Tool](https://richardah.github.io/xfl-tools/)
 - Under Decimal enter 10
 - Copy the value in XFL
 - Paste into the [Decimal Converter](https://www.rapidtables.com/convert/number/decimal-to-hex.html)
 - Scroll down to the Little endian value
 - Copy the value and remove any spaces. Result should be 0080C6A47E8DC354
4. Click "Suggest" under Fee
6. Click "SetHook"

> Review the sethook operation, including the HookParameters

## Step 4

1. Click "Test" top right
2. Under Account select Alice
2. Under TransactionType select Payment
3. Under Destination select Bob
4. Under Amount enter 10
4. Under Fee click "Suggest"
5. Click "Run Test"

> Review the debug, and the successful payment operation, and the hook execution
> Copy the HookExecutionString, paste into the [Hex Converter](https://www.rapidtables.com/convert/number/hex-to-ascii.html)

## Step 5

1. Clean up and uninstall all hooks (Alice & Bob)

# Lesson #2

> What is the goal. What does the hook do. 

## Introduction

## Where we left.

- Hook Parameter
- Otxn Parameter

## Step 1

1. Open the hooks builder to https://hooks-builder.xrpl.org/develop/7ef229a275e94f4be465427e401aab1d
2. Click "Compile to wasm"
3. Click "Develop" top right
4. Click "Set Hook" under "Alice" account

- Select the Payment type under HookOn

5. Click "Suggest" under Fee
6. Click "SetHook"

> Review the sethook operation
 
 ## Step 2

1. Click "Test" top right
2. Under Account select Alice
2. Under TransactionType select Payment
3. Under Destination select Bob
2. Under Txn Parameters enter
 - Name: ARRAY
 - Value: 030080C6A47E8D0355F51DFC2A09D62CBBA1DFBDD4691DAC96AD98B90F0080C6A47E8D0355B389FBCED0AF9DCDFF62900BFAEFA3EB872D8A960080E03779C3D154AA266540F7DACC27E264B75ED0A5ED7330BFB614
4. Under Fee click "Suggest"
5. Click "Run Test"

> Review the debug, review the hook emissions on the original tx. Review each of the emitted transactions



