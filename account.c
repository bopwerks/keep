#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "account.h"
#include "transaction.h"
#include "util.h"

account *accounts[MAXACCT];
int naccounts = 0;

static void bin_flow(account *a, transaction *t);
static void bin_balance(account *a, transaction *t);

account *
account_new(account_type type, char *name, char *longname)
{
    account *a;
    account *parent;

    /* fprintf(stderr, "Creating new account '%s'\n", name); */
    a = calloc(1, sizeof *a);
    if (a == NULL) {
        return NULL;
    }
    a->typ = ACCT;
    a->type = type;
    switch (a->type) {
    case INCOME:
    case EXPENSE:
        a->bin = bin_flow;
        break;
    case ASSET:
    case LIABILITY:
        a->bin = bin_balance;
        break;
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
    a->trcap = 1;
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

int
account_connect(account *parent, account *child)
{
    /* fprintf(stderr, "Connecting from %s to %s\n", child->name, parent->name); */
    if (child->parent != NULL) {
        fprintf(stderr, "Account %s already has parent %s\n", child->name, parent->name);
        return 0;
    }
    child->parent = parent;
    child->nparents += 1;
    /* if (child->nparents > 1) { */
    /*     fprintf(stderr, "More than one connection from %s found while connecting to %s\n", */
    /*             child->name, parent->name); */
    /* } */
    parent->accounts[parent->naccounts++] = child;
    return 1;
}

static long
account_debits(account *acct, int year, int month)
{
    long total;
    int i;
    transaction *tr;

    for (total = i = 0, tr = acct->tr[i]; i < acct->ntr; tr = acct->tr[++i]) {
        if (tr->year == year && tr->month == month) {
            total += tr->debit;
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
    /* period *yp; */
    /* period *mp; */
    long total;
    int i;

    total = (acct->ntr > 0) ? acct->tr[acct->ntr-1]->totalcredits : 0;
    /* for (yp = acct->year; yp != NULL; yp = yp->right) { */
    /*     for (mp = yp->left; mp != NULL; mp = mp->right) { */
    /*         if ((year == 0 || yp->n == year) && (month == 0 || mp->n == month)) { */
    /*             total += mp->debits; */
    /*         } */
    /*     } */
    /* } */
    for (i = 0; i < acct->naccounts; ++i) {
        total += account_credits(acct->accounts[i], year, month);
    }
    return total;
}

static int indent;

static void
account_flow_rec(account *acct, int year, int month, long *dr, long *cr)
{
    int i;
    transaction *tr;

    /* for (i = 0; i < indent; ++i) */
    /*     fputc('\t', stderr); */
    /* fprintf(stderr, "Factoring in transactions from account: %s\n", acct->name); */
    for (i = 0, tr = acct->tr[i]; i < acct->ntr; tr = acct->tr[++i]) {
        if ((year == 0 || tr->year == year) && (month == 0 || tr->month == month)) {
            /* fprintf(stderr, "%s dr %ld cr %ld\n", tr->description, tr->debit, tr->credit); */
            *dr += tr->debit;
            *cr += tr->credit;
        }
    }
    /* ++indent; */
    for (i = 0; i < acct->naccounts; ++i) {
        account_flow_rec(acct->accounts[i], year, month, dr, cr);
    }
    /* --indent; */
}

long
account_flow(account *acct, int year, int month, int *ok)
{
    long dr;
    long cr;
    long min;
    long max;

    dr = cr = 0;
    account_flow_rec(acct, year, month, &dr, &cr);
    if (dr == 0.0 && cr == 0.0 && ok != NULL) {
        *ok = 0;
    }
    /* dr = account_debits(acct, year, month); */
    /* cr = account_credits(acct, year, month); */
    min = (dr > cr) ? cr : dr;
    max = (dr > cr) ? dr : cr;

    return max - min;
}

static int
account_balance_rec(account *acct, int year, int month, long *dr, long *cr)
{
    int i;
    transaction *tr;
    int ntr;

    ntr = 0;
    for (i = 0; i < acct->ntr; tr = acct->tr[++i]) {
        tr = acct->tr[i];
        if ((year == 0 || tr->year <= year) && (month == 0 || tr->month <= month)) {
            /* fprintf(stderr, "%s dr %ld cr %ld\n", tr->description, tr->debit, tr->credit); */
            *dr += tr->debit;
            *cr += tr->credit;
            ++ntr;
        }
    }
    for (i = 0; i < acct->naccounts; ++i) {
        ntr += account_balance_rec(acct->accounts[i], year, month, dr, cr);
    }
    return ntr;
}

long
account_balance(account *acct, int year, int month, int *ok)
{
    long dr;
    long cr;
    long min;
    long max;

    dr = cr = 0;
    if (account_balance_rec(acct, year, month, &dr, &cr) == 0 && ok != NULL) {
        *ok = 0;
    }
    /* dr = account_debits(acct, year, month); */
    /* cr = account_credits(acct, year, month); */
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

    switch (acct->type) {
    case ASSET:
    case LIABILITY:
        bal = account_balance(acct, year, month, NULL);
        /* if (bal > 0) { */
            dollars = bal / 100;
            cents = bal % 100;
            for (i = 0; i < level; ++i) {
                putchar('\t');
            }
            printf("%s %d.%02d\n", acct->longname, dollars, cents);
            for (i = 0; i < acct->naccounts; ++i) {
                account_print(acct->accounts[i], year, month, level+1);
            }
        /* } */
        break;
    case EXPENSE:
    case INCOME:
        bal = account_flow(acct, year, month, NULL);
        /* if (bal > 0) { */
            dollars = bal / 100;
            cents = bal % 100;
            for (i = 0; i < level; ++i) {
                putchar('\t');
            }
            printf("%s %d.%02d\n", acct->longname, dollars, cents);
            for (i = 0; i < acct->naccounts; ++i) {
                account_print(acct->accounts[i], year, month, level+1);
            }
        /* } */
        break;
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

extern double eval(expr *e, int y, int m, int *ok);

double
account_eval(account *acct, int year, int month, int *ok)
{
    double rval;
    long bal;
    long dollars;
    long cents;
    switch (acct->typ) {
    case ACCT:
        switch (acct->type) {
        case ASSET:
        case LIABILITY:
            bal = account_balance(acct, year, month, ok);
            break;
        case INCOME:
        case EXPENSE:
            bal = account_flow(acct, year, month, ok);
            break;
        }
        dollars = bal / 100;
        cents = bal % 100;
        rval = dollars + (cents / 100.0);
        return rval;
    case VAR:
        return eval(acct->exp, year, month, ok);
    case NUM:
        return acct->dval;
    }
}

static void
bin_flow(account *a, transaction *t)
{
    /* TODO: Compute index into a->months from t->date and compute the
     * flow for that month */
}

static void
bin_balance(account *a, transaction *t)
{
    /* TODO: Compute index into a->months from t->date and compute the
     * resulting balance from the transaction for that month */
}
