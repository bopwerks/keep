#include <time.h>

enum {
    MAXACCT = 128,
    MAXSUB = 64
};

enum obj_type {
    ACCT,
    VAR,
    CONST,
};
typedef enum obj_type obj_type;

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
    obj_type typ;
    account_type type;
    
    char *name;
    char *longname;

    struct account *accounts[MAXSUB];
    int naccounts;

    struct account *parent;
    
    struct transaction **tr;
    int ntr;
    int trcap;

    double dval;
    expr *exp;

    bucket **buckets;
    int nbuckets;
    int maxbuckets;

    long dr;
    long cr;

    double (*eval)(struct account *a, int year, int month);
};
typedef struct account account;

extern account *accounts[MAXACCT];
extern int naccounts;

account* account_find(char *name);
account* account_new(account_type type, char *name, char *longname);
int      account_connect(account *parent, account *child);
long     account_eval(account *a, time_t t);
void     account_bin(account *a, time_t date, long dr, long cr);
void     account_print(account *acct, time_t date, int level);
account* const_new(char *name, double val);
