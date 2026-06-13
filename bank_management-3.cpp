/*
 * ================================================================
 *   Bank Management Application
 *   Language  : C++17
 *   Paradigm  : Object-Oriented Programming + File Handling
 *   Storage   : Binary file  (accounts.dat)
 *               Text  file   (transactions.log)
 *
 *   Features:
 *     • Account creation (Savings / Current / Fixed Deposit)
 *     • Deposit & Withdrawal with balance validation
 *     • Fund Transfer between accounts
 *     • Mini Statement (last 10 transactions)
 *     • Interest calculation
 *     • Account search & full profile
 *     • Admin panel (view all, statistics, close account)
 *     • Persistent storage via binary file I/O
 * ================================================================
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <ctime>
#include <sstream>
#include <cstring>

using namespace std;

// ────────────────────────────────────────────
//  Constants
// ────────────────────────────────────────────
const string ACCOUNTS_FILE    = "accounts.dat";
const string TRANSACTION_LOG  = "transactions.log";
const double SAVINGS_INTEREST  = 4.0;   // % per annum
const double CURRENT_INTEREST  = 0.0;
const double FD_INTEREST       = 7.5;
const double MIN_SAVINGS_BAL   = 500.0;
const double MIN_CURRENT_BAL   = 1000.0;
const int    MAX_TX_DISPLAY    = 10;

// ────────────────────────────────────────────
//  Enumerations
// ────────────────────────────────────────────
enum class AccountType { SAVINGS = 1, CURRENT, FIXED_DEPOSIT };
enum class TxType      { DEPOSIT, WITHDRAWAL, TRANSFER_IN, TRANSFER_OUT, INTEREST };

// ────────────────────────────────────────────
//  Transaction record (stored in text log)
// ────────────────────────────────────────────
struct Transaction {
    long long accNo;
    TxType    type;
    double    amount;
    double    balanceAfter;
    char      date[25];
    char      note[60];
};

// ────────────────────────────────────────────
//  Account record (stored in binary file)
// ────────────────────────────────────────────
struct Account {
    long long   accNo;
    char        holderName[60];
    char        address[80];
    char        phone[15];
    char        email[60];
    AccountType type;
    double      balance;
    char        pin[6];          // 4-digit PIN (stored as string)
    char        openDate[25];
    bool        active;
    double      interestEarned;
};

// ────────────────────────────────────────────
//  Utility helpers
// ────────────────────────────────────────────
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pause() {
    cout << "\n  Press Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

void clearIn() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

string currentDateTime() {
    time_t now = time(nullptr);
    char   buf[25];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buf);
}

void printLine(char ch = '-', int w = 64) {
    cout << "  " << string(w, ch) << "\n";
}

void printBanner(const string& title) {
    clearScreen();
    printLine('=');
    int pad = (64 - (int)title.size()) / 2;
    cout << "  " << string(pad,' ') << title << "\n";
    printLine('=');
    cout << "\n";
}

string accTypeName(AccountType t) {
    switch (t) {
        case AccountType::SAVINGS:       return "Savings";
        case AccountType::CURRENT:       return "Current";
        case AccountType::FIXED_DEPOSIT: return "Fixed Deposit";
    }
    return "Unknown";
}

string txTypeName(TxType t) {
    switch (t) {
        case TxType::DEPOSIT:      return "Deposit";
        case TxType::WITHDRAWAL:   return "Withdrawal";
        case TxType::TRANSFER_IN:  return "Transfer In";
        case TxType::TRANSFER_OUT: return "Transfer Out";
        case TxType::INTEREST:     return "Interest";
    }
    return "?";
}

long long generateAccNo() {
    // Simple: timestamp-based 10-digit number
    return 1000000000LL + (long long)(time(nullptr) % 1000000000LL);
}

// ────────────────────────────────────────────
//  File I/O – Accounts
// ────────────────────────────────────────────
vector<Account> loadAccounts() {
    vector<Account> list;
    ifstream fin(ACCOUNTS_FILE, ios::binary);
    if (!fin) return list;
    Account a;
    while (fin.read(reinterpret_cast<char*>(&a), sizeof(Account)))
        list.push_back(a);
    return list;
}

bool saveAccounts(const vector<Account>& list) {
    ofstream fout(ACCOUNTS_FILE, ios::binary | ios::trunc);
    if (!fout) return false;
    for (auto& a : list)
        fout.write(reinterpret_cast<const char*>(&a), sizeof(Account));
    return true;
}

// ────────────────────────────────────────────
//  File I/O – Transactions (append-text)
// ────────────────────────────────────────────
void logTransaction(const Transaction& tx) {
    ofstream fout(TRANSACTION_LOG, ios::app);
    if (!fout) return;
    fout << tx.accNo       << "|"
         << (int)tx.type   << "|"
         << fixed << setprecision(2) << tx.amount << "|"
         << tx.balanceAfter << "|"
         << tx.date        << "|"
         << tx.note        << "\n";
}

vector<Transaction> loadTransactions(long long accNo) {
    vector<Transaction> list;
    ifstream fin(TRANSACTION_LOG);
    if (!fin) return list;
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string tok;
        Transaction tx{};
        // accNo
        getline(ss, tok, '|'); tx.accNo = stoll(tok);
        if (tx.accNo != accNo) continue;
        // type
        getline(ss, tok, '|'); tx.type = (TxType)stoi(tok);
        // amount
        getline(ss, tok, '|'); tx.amount = stod(tok);
        // balanceAfter
        getline(ss, tok, '|'); tx.balanceAfter = stod(tok);
        // date
        getline(ss, tok, '|'); strncpy(tx.date, tok.c_str(), 24);
        // note
        getline(ss, tok, '|'); strncpy(tx.note, tok.c_str(), 59);
        list.push_back(tx);
    }
    return list;
}

// ────────────────────────────────────────────
//  Print account summary row
// ────────────────────────────────────────────
void printAccHeader() {
    cout << "  " << left
         << setw(13) << "Acc No"
         << setw(22) << "Name"
         << setw(14) << "Type"
         << setw(12) << "Balance"
         << "Status\n";
    printLine();
}

void printAccRow(const Account& a) {
    cout << "  " << left
         << setw(13) << a.accNo
         << setw(22) << a.holderName
         << setw(14) << accTypeName(a.type)
         << setw(12) << fixed << setprecision(2) << a.balance
         << (a.active ? "Active" : "Closed") << "\n";
}

// ────────────────────────────────────────────
//  PIN verification
// ────────────────────────────────────────────
bool verifyPIN(const Account& a) {
    cout << "  Enter 4-digit PIN: ";
    string pin; cin >> pin; clearIn();
    return (pin == string(a.pin));
}

// ────────────────────────────────────────────
//  Find account index by number
// ────────────────────────────────────────────
int findAcc(const vector<Account>& list, long long no) {
    for (int i = 0; i < (int)list.size(); i++)
        if (list[i].accNo == no) return i;
    return -1;
}

// ────────────────────────────────────────────
//  1. Create Account
// ────────────────────────────────────────────
void createAccount() {
    printBanner("OPEN NEW BANK ACCOUNT");
    vector<Account> list = loadAccounts();
    Account a{};

    a.accNo  = generateAccNo();
    a.active = true;
    a.interestEarned = 0.0;
    strncpy(a.openDate, currentDateTime().c_str(), 24);

    cout << "  Account Number (auto): " << a.accNo << "\n\n";

    clearIn();
    cout << "  Full Name    : "; cin.getline(a.holderName, 60);
    cout << "  Address      : "; cin.getline(a.address, 80);
    cout << "  Phone        : "; cin.getline(a.phone, 15);
    cout << "  Email        : "; cin.getline(a.email, 60);

    cout << "\n  Account Type:\n"
         << "  [1] Savings      (Min balance ₹500,  Interest " << SAVINGS_INTEREST << "%)\n"
         << "  [2] Current      (Min balance ₹1000, No interest)\n"
         << "  [3] Fixed Deposit(Interest " << FD_INTEREST << "%)\n"
         << "  Choice: ";
    int t; cin >> t; clearIn();
    if (t < 1 || t > 3) { cout << "\n  Invalid type.\n"; pause(); return; }
    a.type = (AccountType)t;

    double minBal = (a.type == AccountType::SAVINGS) ? MIN_SAVINGS_BAL : MIN_CURRENT_BAL;
    if (a.type == AccountType::FIXED_DEPOSIT) minBal = 1000.0;

    cout << "  Initial Deposit (min ₹" << minBal << "): ₹";
    double dep; cin >> dep; clearIn();
    if (dep < minBal) {
        cout << "\n  ✗  Deposit below minimum. Account not created.\n";
        pause(); return;
    }
    a.balance = dep;

    cout << "  Set 4-digit PIN: ";
    string pin; cin >> pin; clearIn();
    if (pin.size() != 4 || !all_of(pin.begin(), pin.end(), ::isdigit)) {
        cout << "\n  ✗  PIN must be exactly 4 digits.\n"; pause(); return;
    }
    strncpy(a.pin, pin.c_str(), 5);

    list.push_back(a);
    if (!saveAll(list)) { // alias below
        // use real save
    }
    saveAccounts(list);

    // Log opening deposit
    Transaction tx{};
    tx.accNo = a.accNo; tx.type = TxType::DEPOSIT;
    tx.amount = dep; tx.balanceAfter = dep;
    strncpy(tx.date, currentDateTime().c_str(), 24);
    strncpy(tx.note, "Account opening deposit", 59);
    logTransaction(tx);

    cout << "\n  ✓  Account created successfully!\n";
    cout << "  ──────────────────────────────────\n";
    cout << "  Account No : " << a.accNo << "\n";
    cout << "  Name       : " << a.holderName << "\n";
    cout << "  Type       : " << accTypeName(a.type) << "\n";
    cout << "  Balance    : ₹" << fixed << setprecision(2) << a.balance << "\n";
    cout << "  ──────────────────────────────────\n";
    cout << "  Please remember your Account Number and PIN!\n";
    pause();
}

// Alias so the code above compiles
bool saveAll(const vector<Account>& l) { return saveAccounts(l); }

// ────────────────────────────────────────────
//  2. Deposit
// ────────────────────────────────────────────
void deposit() {
    printBanner("DEPOSIT CASH");
    cout << "  Account Number: "; long long no; cin >> no; clearIn();

    vector<Account> list = loadAccounts();
    int idx = findAcc(list, no);
    if (idx < 0) { cout << "\n  ✗  Account not found.\n"; pause(); return; }
    if (!list[idx].active) { cout << "\n  ✗  Account is closed.\n"; pause(); return; }

    cout << "  Current Balance: ₹" << fixed << setprecision(2) << list[idx].balance << "\n";
    cout << "  Amount to deposit: ₹";
    double amt; cin >> amt; clearIn();
    if (amt <= 0) { cout << "\n  ✗  Invalid amount.\n"; pause(); return; }

    list[idx].balance += amt;
    saveAccounts(list);

    Transaction tx{};
    tx.accNo = no; tx.type = TxType::DEPOSIT;
    tx.amount = amt; tx.balanceAfter = list[idx].balance;
    strncpy(tx.date, currentDateTime().c_str(), 24);
    strncpy(tx.note, "Cash deposit", 59);
    logTransaction(tx);

    cout << "\n  ✓  ₹" << fixed << setprecision(2) << amt << " deposited.\n";
    cout << "  New Balance: ₹" << list[idx].balance << "\n";
    pause();
}

// ────────────────────────────────────────────
//  3. Withdrawal
// ────────────────────────────────────────────
void withdrawal() {
    printBanner("CASH WITHDRAWAL");
    cout << "  Account Number: "; long long no; cin >> no; clearIn();

    vector<Account> list = loadAccounts();
    int idx = findAcc(list, no);
    if (idx < 0) { cout << "\n  ✗  Account not found.\n"; pause(); return; }
    if (!list[idx].active) { cout << "\n  ✗  Account is closed.\n"; pause(); return; }
    if (!verifyPIN(list[idx])) { cout << "\n  ✗  Incorrect PIN.\n"; pause(); return; }

    double minBal = (list[idx].type == AccountType::SAVINGS)  ? MIN_SAVINGS_BAL  :
                    (list[idx].type == AccountType::CURRENT)   ? MIN_CURRENT_BAL  : 0.0;

    cout << "  Available Balance: ₹" << fixed << setprecision(2) << list[idx].balance << "\n";
    cout << "  Amount to withdraw: ₹";
    double amt; cin >> amt; clearIn();

    if (amt <= 0) { cout << "\n  ✗  Invalid amount.\n"; pause(); return; }
    if (list[idx].balance - amt < minBal) {
        cout << "\n  ✗  Insufficient balance (min ₹" << minBal << " must remain).\n";
        pause(); return;
    }

    list[idx].balance -= amt;
    saveAccounts(list);

    Transaction tx{};
    tx.accNo = no; tx.type = TxType::WITHDRAWAL;
    tx.amount = amt; tx.balanceAfter = list[idx].balance;
    strncpy(tx.date, currentDateTime().c_str(), 24);
    strncpy(tx.note, "Cash withdrawal", 59);
    logTransaction(tx);

    cout << "\n  ✓  ₹" << fixed << setprecision(2) << amt << " withdrawn.\n";
    cout << "  Remaining Balance: ₹" << list[idx].balance << "\n";
    pause();
}

// ────────────────────────────────────────────
//  4. Fund Transfer
// ────────────────────────────────────────────
void transfer() {
    printBanner("FUND TRANSFER");
    cout << "  From Account No: "; long long from; cin >> from; clearIn();

    vector<Account> list = loadAccounts();
    int fi = findAcc(list, from);
    if (fi < 0 || !list[fi].active) { cout << "\n  ✗  Source account invalid.\n"; pause(); return; }
    if (!verifyPIN(list[fi])) { cout << "\n  ✗  Incorrect PIN.\n"; pause(); return; }

    cout << "  To Account No  : "; long long to; cin >> to; clearIn();
    int ti = findAcc(list, to);
    if (ti < 0 || !list[ti].active) { cout << "\n  ✗  Destination account invalid.\n"; pause(); return; }
    if (from == to) { cout << "\n  ✗  Cannot transfer to same account.\n"; pause(); return; }

    cout << "  Your Balance: ₹" << fixed << setprecision(2) << list[fi].balance << "\n";
    cout << "  Transfer Amount: ₹"; double amt; cin >> amt; clearIn();
    if (amt <= 0) { cout << "\n  ✗  Invalid amount.\n"; pause(); return; }

    double minBal = (list[fi].type == AccountType::SAVINGS) ? MIN_SAVINGS_BAL : MIN_CURRENT_BAL;
    if (list[fi].balance - amt < minBal) {
        cout << "\n  ✗  Insufficient balance.\n"; pause(); return;
    }

    list[fi].balance -= amt;
    list[ti].balance += amt;
    saveAccounts(list);

    string dt = currentDateTime();
    Transaction tx{};
    tx.accNo = from; tx.type = TxType::TRANSFER_OUT;
    tx.amount = amt; tx.balanceAfter = list[fi].balance;
    strncpy(tx.date, dt.c_str(), 24);
    string note = "Transfer to " + to_string(to);
    strncpy(tx.note, note.c_str(), 59);
    logTransaction(tx);

    tx.accNo = to; tx.type = TxType::TRANSFER_IN;
    tx.balanceAfter = list[ti].balance;
    note = "Transfer from " + to_string(from);
    strncpy(tx.note, note.c_str(), 59);
    logTransaction(tx);

    cout << "\n  ✓  ₹" << fixed << setprecision(2) << amt
         << " transferred to Account " << to << "\n";
    cout << "  Your New Balance: ₹" << list[fi].balance << "\n";
    pause();
}

// ────────────────────────────────────────────
//  5. Balance Enquiry
// ────────────────────────────────────────────
void balanceEnquiry() {
    printBanner("BALANCE ENQUIRY");
    cout << "  Account Number: "; long long no; cin >> no; clearIn();

    vector<Account> list = loadAccounts();
    int idx = findAcc(list, no);
    if (idx < 0) { cout << "\n  ✗  Account not found.\n"; pause(); return; }
    if (!verifyPIN(list[idx])) { cout << "\n  ✗  Incorrect PIN.\n"; pause(); return; }

    auto& a = list[idx];
    printLine();
    cout << "  Account No   : " << a.accNo << "\n";
    cout << "  Holder Name  : " << a.holderName << "\n";
    cout << "  Account Type : " << accTypeName(a.type) << "\n";
    cout << "  Balance      : ₹" << fixed << setprecision(2) << a.balance << "\n";
    cout << "  Opened On    : " << a.openDate << "\n";
    cout << "  Status       : " << (a.active ? "Active" : "Closed") << "\n";
    printLine();
    pause();
}

// ────────────────────────────────────────────
//  6. Mini Statement
// ────────────────────────────────────────────
void miniStatement() {
    printBanner("MINI STATEMENT");
    cout << "  Account Number: "; long long no; cin >> no; clearIn();

    vector<Account> list = loadAccounts();
    int idx = findAcc(list, no);
    if (idx < 0) { cout << "\n  ✗  Account not found.\n"; pause(); return; }
    if (!verifyPIN(list[idx])) { cout << "\n  ✗  Incorrect PIN.\n"; pause(); return; }

    auto txList = loadTransactions(no);
    cout << "\n  Last " << MAX_TX_DISPLAY << " transactions for Acc: " << no << "\n";
    printLine();
    cout << "  " << left
         << setw(22) << "Date"
         << setw(14) << "Type"
         << setw(12) << "Amount"
         << "Balance\n";
    printLine();

    int start = max(0, (int)txList.size() - MAX_TX_DISPLAY);
    for (int i = start; i < (int)txList.size(); i++) {
        auto& tx = txList[i];
        cout << "  " << left
             << setw(22) << tx.date
             << setw(14) << txTypeName(tx.type)
             << "₹" << setw(11) << fixed << setprecision(2) << tx.amount
             << "₹" << tx.balanceAfter << "\n";
    }
    printLine();
    if (txList.empty()) cout << "  No transactions found.\n";
    pause();
}

// ────────────────────────────────────────────
//  7. Apply Interest
// ────────────────────────────────────────────
void applyInterest() {
    printBanner("APPLY INTEREST");
    cout << "  This will apply annual interest to all eligible accounts.\n";
    cout << "  Confirm? (y/n): "; char ch; cin >> ch; clearIn();
    if (ch != 'y' && ch != 'Y') { cout << "  Cancelled.\n"; pause(); return; }

    vector<Account> list = loadAccounts();
    int count = 0;
    string dt = currentDateTime();

    for (auto& a : list) {
        if (!a.active) continue;
        double rate = (a.type == AccountType::SAVINGS)       ? SAVINGS_INTEREST :
                      (a.type == AccountType::FIXED_DEPOSIT) ? FD_INTEREST      : 0.0;
        if (rate == 0.0) continue;

        double interest = a.balance * rate / 100.0;
        a.balance       += interest;
        a.interestEarned += interest;

        Transaction tx{};
        tx.accNo = a.accNo; tx.type = TxType::INTEREST;
        tx.amount = interest; tx.balanceAfter = a.balance;
        strncpy(tx.date, dt.c_str(), 24);
        string note = "Annual interest @ " + to_string(rate) + "%";
        strncpy(tx.note, note.c_str(), 59);
        logTransaction(tx);
        count++;
    }

    saveAccounts(list);
    cout << "\n  ✓  Interest applied to " << count << " account(s).\n";
    pause();
}

// ────────────────────────────────────────────
//  8. Update Account Details
// ────────────────────────────────────────────
void updateAccount() {
    printBanner("UPDATE ACCOUNT");
    cout << "  Account Number: "; long long no; cin >> no; clearIn();

    vector<Account> list = loadAccounts();
    int idx = findAcc(list, no);
    if (idx < 0) { cout << "\n  ✗  Account not found.\n"; pause(); return; }
    if (!verifyPIN(list[idx])) { cout << "\n  ✗  Incorrect PIN.\n"; pause(); return; }

    auto& a = list[idx];
    cout << "\n  (Leave blank to keep current value)\n\n";
    char buf[80];

    cout << "  Name    [" << a.holderName << "]: "; cin.getline(buf, 60);
    if (strlen(buf)) strncpy(a.holderName, buf, 60);

    cout << "  Address [" << a.address << "]: "; cin.getline(buf, 80);
    if (strlen(buf)) strncpy(a.address, buf, 80);

    cout << "  Phone   [" << a.phone << "]: "; cin.getline(buf, 15);
    if (strlen(buf)) strncpy(a.phone, buf, 15);

    cout << "  Email   [" << a.email << "]: "; cin.getline(buf, 60);
    if (strlen(buf)) strncpy(a.email, buf, 60);

    cout << "  Change PIN? (y/n): "; char ch; cin >> ch; clearIn();
    if (ch == 'y' || ch == 'Y') {
        cout << "  New 4-digit PIN: "; string pin; cin >> pin; clearIn();
        if (pin.size() == 4 && all_of(pin.begin(), pin.end(), ::isdigit))
            strncpy(a.pin, pin.c_str(), 5);
        else cout << "  Invalid PIN, not changed.\n";
    }

    saveAccounts(list);
    cout << "\n  ✓  Account updated successfully.\n";
    pause();
}

// ────────────────────────────────────────────
//  9. Close Account
// ────────────────────────────────────────────
void closeAccount() {
    printBanner("CLOSE ACCOUNT");
    cout << "  Account Number: "; long long no; cin >> no; clearIn();

    vector<Account> list = loadAccounts();
    int idx = findAcc(list, no);
    if (idx < 0) { cout << "\n  ✗  Account not found.\n"; pause(); return; }
    if (!list[idx].active) { cout << "\n  ✗  Already closed.\n"; pause(); return; }
    if (!verifyPIN(list[idx])) { cout << "\n  ✗  Incorrect PIN.\n"; pause(); return; }

    cout << "\n  Balance to be returned: ₹"
         << fixed << setprecision(2) << list[idx].balance << "\n";
    cout << "  Confirm close account? (y/n): "; char ch; cin >> ch; clearIn();
    if (ch != 'y' && ch != 'Y') { cout << "  Cancelled.\n"; pause(); return; }

    list[idx].active  = false;
    list[idx].balance = 0.0;
    saveAccounts(list);
    cout << "\n  ✓  Account " << no << " closed. Amount disbursed.\n";
    pause();
}

// ────────────────────────────────────────────
//  ADMIN – View All Accounts
// ────────────────────────────────────────────
void adminViewAll() {
    printBanner("ALL ACCOUNTS  [ADMIN]");
    vector<Account> list = loadAccounts();
    if (list.empty()) { cout << "  No accounts found.\n"; pause(); return; }

    printAccHeader();
    for (auto& a : list) printAccRow(a);
    printLine();
    cout << "  Total: " << list.size() << " account(s)\n";
    pause();
}

// ────────────────────────────────────────────
//  ADMIN – Bank Statistics
// ────────────────────────────────────────────
void adminStats() {
    printBanner("BANK STATISTICS  [ADMIN]");
    vector<Account> list = loadAccounts();
    if (list.empty()) { cout << "  No accounts.\n"; pause(); return; }

    double totalBal = 0, savBal = 0, curBal = 0, fdBal = 0;
    int active = 0, closed = 0, sav = 0, cur = 0, fd = 0;

    for (auto& a : list) {
        if (a.active) {
            active++; totalBal += a.balance;
            if (a.type == AccountType::SAVINGS)       { sav++; savBal += a.balance; }
            else if (a.type == AccountType::CURRENT)  { cur++; curBal += a.balance; }
            else                                       { fd++;  fdBal  += a.balance; }
        } else closed++;
    }

    printLine();
    cout << "  Total Accounts     : " << list.size() << "\n";
    cout << "  Active Accounts    : " << active << "\n";
    cout << "  Closed Accounts    : " << closed << "\n";
    cout << "  Total Deposits     : ₹" << fixed << setprecision(2) << totalBal << "\n";
    printLine();
    cout << "  Savings Accounts   : " << sav << "  (₹" << savBal << ")\n";
    cout << "  Current Accounts   : " << cur << "  (₹" << curBal << ")\n";
    cout << "  Fixed Deposits     : " << fd  << "  (₹" << fdBal  << ")\n";
    printLine();
    pause();
}

// ────────────────────────────────────────────
//  ADMIN – Search Account
// ────────────────────────────────────────────
void adminSearch() {
    printBanner("SEARCH ACCOUNT  [ADMIN]");
    cout << "  Search by [1] Account No  [2] Name: ";
    int ch; cin >> ch; clearIn();

    vector<Account> list = loadAccounts();
    vector<Account> res;

    if (ch == 1) {
        cout << "  Account No: "; long long no; cin >> no; clearIn();
        for (auto& a : list) if (a.accNo == no) res.push_back(a);
    } else {
        cout << "  Name (partial): "; string key; getline(cin, key);
        string lkey = key;
        transform(lkey.begin(), lkey.end(), lkey.begin(), ::tolower);
        for (auto& a : list) {
            string nm = a.holderName;
            transform(nm.begin(), nm.end(), nm.begin(), ::tolower);
            if (nm.find(lkey) != string::npos) res.push_back(a);
        }
    }

    if (res.empty()) { cout << "\n  No matching accounts.\n"; pause(); return; }
    cout << "\n";
    printAccHeader();
    for (auto& a : res) printAccRow(a);
    printLine();
    pause();
}

// ────────────────────────────────────────────
//  ADMIN Menu
// ────────────────────────────────────────────
void adminMenu() {
    printBanner("ADMIN LOGIN");
    cout << "  Admin Password: ";
    string pwd; cin >> pwd; clearIn();
    if (pwd != "admin123") {
        cout << "\n  ✗  Incorrect password.\n"; pause(); return;
    }

    int choice;
    do {
        printBanner("ADMIN PANEL");
        cout << "  [1]  View All Accounts\n";
        cout << "  [2]  Bank Statistics\n";
        cout << "  [3]  Search Account\n";
        cout << "  [4]  Apply Interest (Annual)\n";
        cout << "  [0]  Back to Main Menu\n";
        printLine();
        cout << "  Choice: ";
        if (!(cin >> choice)) { clearIn(); choice = -1; }
        switch (choice) {
            case 1: adminViewAll();  break;
            case 2: adminStats();    break;
            case 3: adminSearch();   break;
            case 4: applyInterest(); break;
            case 0: break;
            default: cout << "\n  Invalid.\n"; pause();
        }
    } while (choice != 0);
}

// ────────────────────────────────────────────
//  Main Menu
// ────────────────────────────────────────────
void showMainMenu() {
    printBanner("WELCOME TO BHAVNA BANK");
    cout << "  ── Customer Services ──────────────────\n\n";
    cout << "  [1]  Open New Account\n";
    cout << "  [2]  Deposit Cash\n";
    cout << "  [3]  Withdraw Cash\n";
    cout << "  [4]  Fund Transfer\n";
    cout << "  [5]  Balance Enquiry\n";
    cout << "  [6]  Mini Statement\n";
    cout << "  [7]  Update Account Details\n";
    cout << "  [8]  Close Account\n";
    cout << "\n  ── Administration ─────────────────────\n\n";
    cout << "  [9]  Admin Panel\n";
    cout << "\n  [0]  Exit\n";
    printLine();
    cout << "  Choice: ";
}

int main() {
    int choice;
    do {
        showMainMenu();
        if (!(cin >> choice)) { clearIn(); choice = -1; }
        switch (choice) {
            case 1: createAccount();  break;
            case 2: deposit();        break;
            case 3: withdrawal();     break;
            case 4: transfer();       break;
            case 5: balanceEnquiry(); break;
            case 6: miniStatement();  break;
            case 7: updateAccount();  break;
            case 8: closeAccount();   break;
            case 9: adminMenu();      break;
            case 0: break;
            default: cout << "\n  ✗  Invalid option.\n"; pause();
        }
    } while (choice != 0);

    clearScreen();
    cout << "\n  Thank you for banking with us! Goodbye.\n\n";
    return 0;
}
