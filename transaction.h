struct transaction {
    int year;
    int month;
    int day;
    char *description;
    long debit;
    long credit;
    long totaldebits;
    long totalcredits;
};
typedef struct transaction transaction;
