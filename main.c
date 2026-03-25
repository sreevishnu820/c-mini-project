/*
Project Title: Mini Banking System with Transaction Log
Student Name: 
Register No: 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <emscripten.h>

typedef struct {
    int accNo;
    char name[50];
    double balance;
} Account;



int getSafeInt(const char* promptMsg) {
    printf("%s", promptMsg); 
    fflush(stdout);
    // Execute inline JavaScript to get an integer
    int val = EM_ASM_INT({
        var res = prompt(UTF8ToString($0));
        return res ? parseInt(res, 10) : -1;
    }, promptMsg);
    printf("%d\n", val); // Echo the input to the screen
    return val;
}

double getSafeDouble(const char* promptMsg) {
    printf("%s", promptMsg); 
    fflush(stdout);
    // Execute inline JavaScript to get a decimal
    double val = EM_ASM_DOUBLE({
        var res = prompt(UTF8ToString($0));
        return res ? parseFloat(res) : -1.0;
    }, promptMsg);
    printf("%.2f\n", val);
    return val;
}

void getSafeString(const char* promptMsg, char* buffer, int maxLen) {
    printf("%s", promptMsg); 
    fflush(stdout);
    // Execute inline JavaScript to get a string
    EM_ASM({
        var res = prompt(UTF8ToString($0));
        if (!res) res = "Unknown";
        stringToUTF8(res, $1, $2);
    }, promptMsg, buffer, maxLen);
    printf("%s\n", buffer);
}
// ----------------------------------------------

void createAccount();
void deposit();
void withdraw();
void viewSummary();
void searchAccount();
void displayLastTransactions();
void logTransaction(int accNo, const char* type, double amount);
int findAccount(int searchAccNo, Account *acc);
void updateAccountFile(Account updatedAcc);

int main() {
    int choice;
    while (1) {
        printf("\n=== Mini Banking System ===\n");
        printf("1. Create Account\n");
        printf("2. Deposit\n");
        printf("3. Withdraw\n");
        printf("4. View Account Summary\n");
        printf("5. Search Account\n");
        printf("6. View Last 5 Transactions\n");
        printf("7. Exit\n");
        
        choice = getSafeInt("Enter your choice: ");

        switch (choice) {
            case 1: createAccount(); break;
            case 2: deposit(); break;
            case 3: withdraw(); break;
            case 4: viewSummary(); break;
            case 5: searchAccount(); break;
            case 6: displayLastTransactions(); break;
            case 7: 
                printf("Exiting system. Thank you!\n");
                exit(0);
            default: 
                printf("Invalid choice! Please enter a number between 1 and 7.\n");
        }
    }
    return 0;
}

void logTransaction(int accNo, const char* type, double amount) {
    FILE *logFile = fopen("transactions.log", "a");
    if (logFile == NULL) return;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(logFile, "%d | %s | %.2f | %02d-%02d-%d %02d:%02d:%02d\n", 
            accNo, type, amount, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, 
            tm.tm_hour, tm.tm_min, tm.tm_sec);
    fclose(logFile);
}

void createAccount() {
    FILE *file = fopen("accounts.txt", "a");
    if (file == NULL) return;

    Account newAcc;
    newAcc.accNo = getSafeInt("Enter Account Number: ");
    if (newAcc.accNo <= 0) {
        printf("Error: Invalid Account Number.\n");
        fclose(file);
        return;
    }
    
    Account temp;
    if (findAccount(newAcc.accNo, &temp)) {
        printf("Error: Account %d already exists!\n", newAcc.accNo);
        fclose(file);
        return;
    }

    getSafeString("Enter Account Holder Name: ", newAcc.name, sizeof(newAcc.name));
    newAcc.balance = 0.0; 

    fprintf(file, "%d,%s,%.2f\n", newAcc.accNo, newAcc.name, newAcc.balance);
    fclose(file);
    
    logTransaction(newAcc.accNo, "Create", 0.0);
    printf("Account created successfully!\n");
}

int findAccount(int searchAccNo, Account *acc) {
    FILE *file = fopen("accounts.txt", "r");
    if (file == NULL) return 0;
    while (fscanf(file, "%d,%49[^,],%lf\n", &acc->accNo, acc->name, &acc->balance) != EOF) {
        if (acc->accNo == searchAccNo) {
            fclose(file);
            return 1; 
        }
    }
    fclose(file);
    return 0; 
}

void updateAccountFile(Account updatedAcc) {
    FILE *file = fopen("accounts.txt", "r");
    FILE *tempFile = fopen("temp.txt", "w");
    if (file == NULL || tempFile == NULL) return;
    Account acc;
    while (fscanf(file, "%d,%49[^,],%lf\n", &acc.accNo, acc.name, &acc.balance) != EOF) {
        if (acc.accNo == updatedAcc.accNo) {
            fprintf(tempFile, "%d,%s,%.2f\n", updatedAcc.accNo, updatedAcc.name, updatedAcc.balance);
        } else {
            fprintf(tempFile, "%d,%s,%.2f\n", acc.accNo, acc.name, acc.balance);
        }
    }
    fclose(file);
    fclose(tempFile);
    remove("accounts.txt");
    rename("temp.txt", "accounts.txt");
}

void deposit() {
    Account acc;
    int accNo = getSafeInt("Enter Account Number: ");

    if (findAccount(accNo, &acc)) {
        double amount = getSafeDouble("Enter amount to deposit: ");
        if (amount <= 0) {
            printf("Error: Amount must be positive.\n");
            return;
        }
        acc.balance += amount;
        updateAccountFile(acc);
        logTransaction(accNo, "Deposit", amount);
        printf("Deposited %.2f. New Balance: %.2f\n", amount, acc.balance);
    } else {
        printf("Error: Account not found.\n");
    }
}

void withdraw() {
    Account acc;
    int accNo = getSafeInt("Enter Account Number: ");

    if (findAccount(accNo, &acc)) {
        double amount = getSafeDouble("Enter amount to withdraw: ");
        if (amount <= 0) {
            printf("Error: Amount must be positive.\n");
            return;
        }
        if (acc.balance < amount) {
            printf("Error: Insufficient funds. Balance is %.2f\n", acc.balance);
            return;
        }
        acc.balance -= amount;
        updateAccountFile(acc);
        logTransaction(accNo, "Withdraw", amount);
        printf("Withdrew %.2f. New Balance: %.2f\n", amount, acc.balance);
    } else {
        printf("Error: Account not found.\n");
    }
}

void searchAccount() {
    Account acc;
    int accNo = getSafeInt("Enter Account Number to search: ");

    if (findAccount(accNo, &acc)) {
        printf("\n--- Account Details ---\n");
        printf("Acc No  : %d\n", acc.accNo);
        printf("Name    : %s\n", acc.name);
        printf("Balance : %.2f\n", acc.balance);
        printf("-----------------------\n");
    } else {
        printf("Error: Account not found.\n");
    }
}

void viewSummary() {
    FILE *file = fopen("accounts.txt", "r");
    if (file == NULL) {
        printf("No accounts found.\n");
        return;
    }
    Account acc;
    int count = 0;
    printf("\n--- All Accounts Summary ---\n");
    while (fscanf(file, "%d,%49[^,],%lf\n", &acc.accNo, acc.name, &acc.balance) != EOF) {
        printf("Acc: %d | Name: %s | Bal: %.2f\n", acc.accNo, acc.name, acc.balance);
        count++;
    }
    fclose(file);
    if (count == 0) printf("No accounts to display.\n");
}

void displayLastTransactions() {
    int searchAccNo = getSafeInt("Enter Account Number: ");

    FILE *file = fopen("transactions.log", "r");
    if (file == NULL) {
        printf("No transactions found.\n");
        return;
    }
    char history[5][150]; 
    int count = 0;
    char line[150];
    int fileAccNo;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%d", &fileAccNo) == 1 && fileAccNo == searchAccNo) {
            if (count == 5) {
                for (int i = 0; i < 4; i++) strcpy(history[i], history[i + 1]);
                strcpy(history[4], line);
            } else {
                strcpy(history[count], line);
                count++;
            }
        }
    }
    fclose(file);

    if (count == 0) {
        printf("No transactions found for Account %d.\n", searchAccNo);
    } else {
        printf("\n--- Last %d Transactions for Acc %d ---\n", count, searchAccNo);
        for (int i = 0; i < count; i++) printf("%s", history[i]);
    }
}
