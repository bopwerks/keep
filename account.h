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

typedef struct expr expr;

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

    double *months; /* Flow or balance of account at the end of each month */
    int nmonths;
    double *days; /* Flow or balance of account at the end of each day */
    int ndays;

    double (*eval)(struct account *a, int year, int month, int *ok);
    void (*bin)(struct account *a, struct transaction *t);
};
typedef struct account account;

extern account *accounts[MAXACCT];
extern int naccounts;

account* account_find(char *name);
account* account_new(account_type type, char *name, char *longname);
long     account_balance(account *acct, int year, int month, int *ok);
int      account_connect(account *parent, account *child);
void     account_print(account *acct, int year, int month, int level);
double   account_eval(account *acct, int year, int month, int *ok);
