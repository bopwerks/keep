#include <time.h>

enum {
    MAXACCT = 128,
    MAXSUB = 64
};

enum var_type {
    ACCT,
    VAR,
    NUM,
};
typedef enum var_type var_type;

enum account_type {
    ASSET,
    LIABILITY,
    INCOME,
    EXPENSE
};
typedef enum account_type account_type;

typedef struct expr expr;

struct bucket {
    int key;
    long dr;
    long cr;
};
typedef struct bucket bucket;

struct account {
    var_type typ;
    account_type type;
    
    char *name;
    char *longname;

    struct account *accounts[MAXSUB];
    int naccounts;

    struct account *parent;
    int nparents;
    
    struct transaction **tr;
    int ntr;
    int trcap;

    struct account *aval;
    double dval;
    expr *exp;

    time_t mindate;
    time_t maxdate;

    bucket **buckets;
    int nbuckets;
    int maxbuckets;

    long startdr;
    long startcr;

    double (*eval)(struct account *a, int year, int month, int *found);
};
typedef struct account account;

extern account *accounts[MAXACCT];
extern int naccounts;

account* account_find(char *name);
account* account_new(account_type type, char *name, char *longname);
long     account_balance(account *acct, time_t date);
int      account_connect(account *parent, account *child);
void     account_print(account *acct, time_t date, int level);
double   account_eval(account *acct, time_t date, int *ok);
void     account_bin(account *a, time_t date, long dr, long cr);
