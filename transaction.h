struct transaction {
    int year;
    int month;
    int day;
    time_t date;
    char *description;
    long debit;
    long credit;
    long totaldebits;
    long totalcredits;
};
typedef struct transaction transaction;

typedef struct account account;

transaction *newtrans(struct tm *date, char *description, long debit, long credit);
void addtrans(account *acct, transaction *tr);
