# 🏦 Bank Management Application (C++)

A full-featured, console-based **Bank Management System** built in **C++17** using **Object-Oriented Programming** and **File Handling**. Simulates real-world core banking operations.

---

## ✨ Features

### 👤 Customer Services
| # | Feature | Description |
|---|---------|-------------|
| 1 | **Open Account** | Create Savings / Current / Fixed Deposit accounts with auto-generated account number |
| 2 | **Deposit** | Add funds to any active account |
| 3 | **Withdrawal** | Withdraw with minimum balance enforcement & PIN verification |
| 4 | **Fund Transfer** | Transfer between two accounts with PIN auth |
| 5 | **Balance Enquiry** | Secure balance check with PIN |
| 6 | **Mini Statement** | Last 10 transactions with date, type & running balance |
| 7 | **Update Details** | Edit name, address, phone, email, PIN |
| 8 | **Close Account** | Deactivate account and disburse balance |

### 🔐 Admin Panel (`admin123`)
| Feature | Description |
|---------|-------------|
| View All Accounts | Tabular list of every account |
| Bank Statistics | Total deposits, account counts by type |
| Search Account | By account number or partial name |
| Apply Interest | Annual interest credit to all eligible accounts |

---

## 🗂️ File Structure

```
bank-management/
├── bank_management.cpp     # All source code (single file, ~500 lines)
├── accounts.dat            # Auto-created binary file (account records)
├── transactions.log        # Auto-created text log (all transactions)
└── README.md
```

---

## 🏗️ OOP Design

```
Structs / Data Models
├── Account         → Account record (accNo, name, type, balance, PIN, ...)
└── Transaction     → Transaction record (accNo, type, amount, date, note)

Enumerations
├── AccountType     → SAVINGS | CURRENT | FIXED_DEPOSIT
└── TxType          → DEPOSIT | WITHDRAWAL | TRANSFER_IN | TRANSFER_OUT | INTEREST

Core Functions (Modules)
├── File I/O        → loadAccounts(), saveAccounts(), logTransaction(), loadTransactions()
├── Customer Ops    → createAccount(), deposit(), withdrawal(), transfer()
│                     balanceEnquiry(), miniStatement(), updateAccount(), closeAccount()
└── Admin Ops       → adminViewAll(), adminStats(), adminSearch(), applyInterest()
```

---

## 💰 Account Rules

| Type | Min Balance | Interest Rate |
|------|------------|---------------|
| Savings | ₹500 | 4.0% per annum |
| Current | ₹1,000 | None |
| Fixed Deposit | ₹1,000 | 7.5% per annum |

---

## 🛠️ Build & Run

### Linux / macOS
```bash
g++ -std=c++17 -o bank bank_management.cpp
./bank
```

### Windows (MinGW)
```bash
g++ -std=c++17 -o bank.exe bank_management.cpp
bank.exe
```

### Windows (MSVC)
```bash
cl /EHsc /std:c++17 bank_management.cpp /Fe:bank.exe
bank.exe
```

> **Requires:** GCC 7+ / Clang 5+ / MSVC 2017+

---

## 🔐 Security Features
- **4-digit PIN** required for withdrawals, transfers, balance enquiry, and account closure
- **Admin password** (`admin123`) protects the admin panel
- Minimum balance enforcement prevents account overdraft

---

## 📸 Menu Preview

```
  ════════════════════════════════════════════════════════════════
                     WELCOME TO BHAVNA BANK
  ════════════════════════════════════════════════════════════════

  ── Customer Services ──────────────────

  [1]  Open New Account
  [2]  Deposit Cash
  [3]  Withdraw Cash
  [4]  Fund Transfer
  [5]  Balance Enquiry
  [6]  Mini Statement
  [7]  Update Account Details
  [8]  Close Account

  ── Administration ─────────────────────

  [9]  Admin Panel

  [0]  Exit
```

---

## 👨‍💻 Author

Feel free to ⭐ star, fork, and contribute!
