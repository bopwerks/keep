enum {
    MAXACCT = 128,
    MAXSUB = 64
};

struct account {
    char *name;
    char *longname;

    double debit;
    double credit;
    
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

account* account_new(char *name, char *longname);
void     account_connect(account *parent, account *child);
void     account_print(account *acct, int level);
