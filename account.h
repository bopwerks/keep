enum {
    MAXACCT = 128,
    MAXSUB = 64
};

enum account_type {
    ASSET,
    LIABILITY,
    INCOME,
    EXPENSE
};
typedef enum account_type account_type;

struct period {
    enum { MONTH, YEAR } type;
    int n;
    long debits;
    long credits;
    struct period *left; /* subperiod */
    struct period *right; /* next period */
};
typedef struct period period;

period* period_new(int yearmo);

struct account {
    account_type type;
    
    char *name;
    char *longname;

    period *year;

    struct account *accounts[MAXSUB];
    int naccounts;

    int nparents;
    struct transaction **tr;
    int ntr;
    int trcap;
};
typedef struct account account;

extern account *accounts[MAXACCT];
extern int naccounts;

account* account_find(char *name);
account* account_new(account_type type, char *name, char *longname);
long     account_balance(account *acct, int year, int month);
void     account_connect(account *parent, account *child);
void     account_print(account *acct, int year, int month, int level);
