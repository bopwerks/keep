#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "account.h"

account *accounts[MAXACCT];
int naccounts = 0;

struct period {
    enum { MONTH, YEAR } type;
    int n;
    double debits;
    double credits;
    struct period *left; /* subperiod */
    struct period *right; /* next period */
};
typedef struct period period;

period *
period_new(int yearmo)
{
    period *p;

    p = malloc(sizeof *p);
    if (p == NULL) {
        return NULL;
    }
    p->n = yearmo;
    p->debits = 0.0;
    p->credits = 0.0;
    p->left = NULL;
    p->right = NULL;
    return p;
}

account *
account_new(char *name, char *longname)
{
    account *a;

    /* fprintf(stderr, "Creating new account '%s'\n", name); */
    a = malloc(sizeof *a);
    if (a == NULL) {
        return NULL;
    }
    a->name = malloc(strlen(name) + 1);
    if (a->name == NULL) {
        free(a);
        return NULL;
    }
    strcpy(a->name, name);
    a->longname = malloc(strlen(longname) + 1);
    if (a->longname == NULL) {
        free(a->name);
        free(a);
        return NULL;
    }
    strcpy(a->longname, longname);
    a->ntr = 0;
    a->trcap = 1;
    a->naccounts = 0;
    a->nparents = 0;
    a->tr = malloc(sizeof *(a->tr) * a->trcap);
    if (a->tr == NULL) {
        free(a->longname);
        free(a->name);
        free(a);
        return NULL;
    }
    accounts[naccounts++] = a;
    return a;
}

void
account_connect(account *parent, account *child)
{
    child->nparents += 1;
    parent->accounts[parent->naccounts++] = child;
}

static double
account_debits(account *acct)
{
    double total;
    int i;

    total = acct->debit;
    for (i = 0; i < acct->naccounts; ++i) {
        total += account_debits(acct->accounts[i]);
    }
    return total;
}

static double
account_credits(account *acct)
{
    double total;
    int i;

    total = acct->credit;
    for (i = 0; i < acct->naccounts; ++i) {
        total += account_credits(acct->accounts[i]);
    }
    return total;
}

static double
account_balance(account *acct)
{
    double dr;
    double cr;
    double min;
    double max;
    
    dr = account_debits(acct);
    cr = account_credits(acct);
    min = (dr > cr) ? cr : dr;
    max = (dr > cr) ? dr : cr;

    return max - min;
}

void
account_print(account *acct, int level)
{
    int i;
    double dr;
    double cr;
    double min;
    double max;

    for (i = 0; i < level; ++i) {
        putchar('\t');
    }
    printf("%s %.2f\n", acct->longname, account_balance(acct));
    for (i = 0; i < acct->naccounts; ++i) {
        account_print(acct->accounts[i], level+1);
    }
}
