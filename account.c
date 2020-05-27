#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "account.h"

account *accounts[MAXACCT];
int naccounts = 0;

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
    a->year = NULL;
    return a;
}

void
account_connect(account *parent, account *child)
{
    child->nparents += 1;
    parent->accounts[parent->naccounts++] = child;
}

static long
account_debits(account *acct, int year, int month)
{
    period *yp;
    period *mp;
    long total;
    int i;

    total = 0.0;
    for (yp = acct->year; yp != NULL; yp = yp->right) {
        for (mp = yp->left; mp != NULL; mp = mp->right) {
            if ((year == 0 || yp->n == year) && (month == 0 || mp->n == month)) {
                total += mp->debits;
            }
        }
    }
    for (i = 0; i < acct->naccounts; ++i) {
        total += account_debits(acct->accounts[i], year, month);
    }
    return total;
}

static long
account_credits(account *acct, int year, int month)
{
    period *yp;
    period *mp;
    long total;
    int i;

    total = 0.0;
    for (yp = acct->year; yp != NULL; yp = yp->right) {
        for (mp = yp->left; mp != NULL; mp = mp->right) {
            if ((year == 0 || yp->n == year) && (month == 0 || mp->n == month)) {
                total += mp->credits;
            }
        }
    }
    for (i = 0; i < acct->naccounts; ++i) {
        total += account_credits(acct->accounts[i], year, month);
    }
    return total;
}

long
account_balance(account *acct, int year, int month)
{
    long dr;
    long cr;
    long min;
    long max;
    
    dr = account_debits(acct, year, month);
    cr = account_credits(acct, year, month);
    min = (dr > cr) ? cr : dr;
    max = (dr > cr) ? dr : cr;

    return max - min;
}

void
account_print(account *acct, int year, int month, int level)
{
    int i;
    long bal;
    int dollars;
    int cents;

    bal = account_balance(acct, year, month);
    if (bal > 0) {
        dollars = bal / 100;
        cents = bal % 100;
        for (i = 0; i < level; ++i) {
            putchar('\t');
        }
        printf("%s %d.%02d\n", acct->longname, dollars, cents);
        for (i = 0; i < acct->naccounts; ++i) {
            account_print(acct->accounts[i], year, month, level+1);
        }
    }
}

account *
account_find(char *name)
{
    int i;

    /* fprintf(stderr, "Looking for account '%s'\n", name); */
    for (i = 0; i < naccounts; ++i) {
        if (strcmp(accounts[i]->name, name) == 0)
            return accounts[i];
    }
    fprintf(stderr, "Can't find account: %s\n", name);
    return NULL;
}
