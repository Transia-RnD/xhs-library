

# Prerequisits

# Accounts in hooks builder.

Navigate to https://hooks-builder.xrpl.org/deploy

## Import Alice
- Click Import under "Accounts" in the bottom left
- Enter "Alice" for the name and "rG1QQv2nh2gr7RCZ1P8YYcBUKCCN633jCn"

## Import Bob
- Click Import under "Accounts" in the bottom left
- Enter "Bob" for the name and "rPMh7Pi9ct699iZUTWaytJUoHcJ7cgyziK"

# Lesson #1

## Step 1

1. Open the hooks builder to https://hooks-builder.xrpl.org/develop/b16d9dd9c1085278ee9e487dd6a96186
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
4. Under Fee click "Suggest"
5. Click "Run Test"

> Review the debug, and the rejected payment operation, and the hook execution
> Copy the HookExecutionString, paste into the [Hex Converter](https://www.rapidtables.com/convert/number/hex-to-ascii.html)

## Step 3

1. Click "Develop" top right
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

# Lesson #2

## Step 1

1. Open the hooks builder to https://hooks-builder.xrpl.org/develop/7ef229a275e94f4be465427e401aab1d
2. Click "Compile to wasm"
3. Click "Develop" top right
4. Click "Set Hook" under "Alice" account

- Select the Payment type under HookOn

5. Click "Suggest" under Fee
6. Click "SetHook"

> Review the sethook operation