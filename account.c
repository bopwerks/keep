#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "account.h"
#include "transaction.h"
#include "util.h"
#include "track.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

account *accounts[MAXACCT];
int naccounts = 0;

static bucket ** find_bucket(account *a, time_t date);
static double eval_expense_income(account *a, int y, int m);
static double eval_asset_liability(account *a, int y, int m);

account *
account_new(account_type type, char *name, char *longname)
{
    account *a;

    a = calloc(1, sizeof *a);
    if (a == NULL) {
        return NULL;
    }
    a->typ = ACCT;
    a->type = type;
    switch (a->type) {
    case EXPENSE:
    case INCOME:
        a->eval = eval_expense_income;
        break;
    case ASSET:
    case LIABILITY:
        a->eval = eval_asset_liability;
        break;
    default:
        break;
    }
    /* a->mindate = 0; */
    /* a->maxdate = 0; */
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
    a->nbuckets = 0;
    a->maxbuckets = 1;
    a->buckets = malloc(sizeof *(a->buckets) * a->maxbuckets);
    if (a->buckets == NULL) {
        free(a->tr);
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
    int i;
    /* fprintf(stderr, "Connecting %s <- %s\n", parent->name, child->name); */
    if (child->parent != NULL) {
        fprintf(stderr, "Account %s already has parent %s\n", child->name, parent->name);
        return 0;
    }
    child->parent = parent;
    parent->accounts[parent->naccounts++] = child;
    for (i = 0; i < child->ntr; ++i) {
        addtrans(parent, child->tr[i]);
    }
    return 1;
}

void
account_print(account *acct, time_t date, int level)
{
    int i;
    double bal;

    assert(acct != NULL);
    bal = account_eval(acct, date);
    for (i = 0; i < level; ++i) {
        putchar('\t');
    }
    printf("%s %.2lf\n", acct->longname, bal);
    for (i = 0; i < acct->naccounts; ++i) {
        if (acct->accounts[i]->typ == ACCT) {
            account_print(acct->accounts[i], date, level+1);
        }
    }
}

account *
account_find(char *name)
{
    int i;

    assert(name != NULL);
    for (i = 0; i < naccounts; ++i) {
        assert(accounts[i] != NULL);
        assert(accounts[i]->name != NULL);
        if (strcmp(accounts[i]->name, name) == 0)
            return accounts[i];
    }
    return NULL;
}

double
account_eval(account *a, time_t t)
{
    struct tm tm;
    assert(a != NULL);

    localtime_r(&t, &tm);
    return a->eval(a, tm.tm_year + 1900, tm.tm_mon + 1);
}

static int
cmp(const void *x, const void *y)
{
    int key;
    bucket **b;

    key = *((int *) x);
    b = (bucket **) y;

    return key - (*b)->key;
}

static bucket **
find_bucket(account *a, time_t date)
{
    struct tm tm;
    int y;
    int m;
    int key;
    bucket **rval;

    localtime_r(&date, &tm);
    y = tm.tm_year + 1900;
    m = tm.tm_mon + 1;

    key = y * 100 + m;
    rval = bsearch((void *) &key, a->buckets, a->nbuckets, sizeof *(a->buckets), cmp);
    if (rval == NULL) {
        return NULL;
    } else {
        return rval;
    }
}

static bucket **
find_bucket_by_key(account *a, int y, int m)
{
    int key;
    bucket **rval;

    key = y * 100 + m;
    rval = bsearch((void *) &key, a->buckets, a->nbuckets, sizeof *(a->buckets), cmp);
    if (rval == NULL) {
        return NULL;
    } else {
        return rval;
    }
}

static int
cmpbucket(bucket *a, bucket *b)
{
    return a->key - b->key;
}

static bucket **
add_bucket(account *a, time_t date)
{
    struct tm tm;
    bucket *b;
    bucket *tmpb;
    bucket **rval;
    int i;
    assert(a != NULL);

    if (a->nbuckets == a->maxbuckets) {
        if (!array_grow((void **) &a->buckets, a->maxbuckets * 2, sizeof *(a->buckets))) {
            return NULL;
        }
        a->maxbuckets *= 2;
    }
    b = calloc(1, sizeof *b);
    if (b == NULL) {
        return NULL;
    }
    localtime_r(&date, &tm);
    b->key = (tm.tm_year + 1900) * 100 + (tm.tm_mon + 1);
    a->buckets[a->nbuckets] = b;
    rval = &a->buckets[a->nbuckets];
    for (i = a->nbuckets; i > 0 && cmpbucket(a->buckets[i-1], a->buckets[i]) > 0; --i) {
        tmpb = a->buckets[i-1];
        a->buckets[i-1] = a->buckets[i];
        rval = &a->buckets[i-1];
        a->buckets[i] = tmpb;
    }
    a->nbuckets += 1;
    return rval;
}

void
account_bin(account *a, time_t date, long dr, long cr)
{
    bucket **b;
    bucket *c;

    assert(a != NULL);
    b = find_bucket(a, date);
    if (b == NULL) {
        b = add_bucket(a, date);
        if (b == NULL) {
            /* TODO: error */
            return;
        }
        switch (a->type) {
        case EXPENSE:
        case INCOME:
            break;
        case ASSET:
        case LIABILITY:
            if (b == a->buckets) {
                /* Initialize bucket with account starting balance */
                (*b)->dr = a->dr;
                (*b)->cr = a->cr;
            } else {
                c = *(b - 1);
                /* Initialize bucket with last month's ending balance */
                (*b)->dr = c->dr;
                (*b)->cr = c->cr;
            }
        }
    }
    (*b)->dr += dr;
    (*b)->cr += cr;
}

static double
eval_expense_income(account *a, int y, int m)
{
    bucket **b;
    long bal;
    
    b = find_bucket_by_key(a, y, m);
    if (b == NULL) {
        return 0.0;
    }
    bal = max((*b)->dr, (*b)->cr) - min((*b)->dr, (*b)->cr);
    /* printf("%s dollars = %ld cents = %ld rval = %f\n", a->name, dollars, cents, rval); */
    return bal / 100.0;
}

static bucket **
fuzzy_find_bucket_by_key(account *a, int y, int m)
{
    int i;
    int key;
    bucket **pb;

    key = y * 100 + m;
    pb = NULL;
    for (i = 0; i < a->nbuckets && a->buckets[i]->key <= key; ++i) {
        if (a->buckets[i]->key == key) {
            return &a->buckets[i];
        } else {
            pb = &a->buckets[i];
        }
    }
    return pb;
}

static double
eval_asset_liability(account *a, int y, int m)
{
    bucket **b;

    b = fuzzy_find_bucket_by_key(a, y, m);
    if (b != NULL) {
        return (max((*b)->dr, (*b)->cr) - min((*b)->dr, (*b)->cr)) / 100.0;
    }
    return (max(a->dr, a->cr) - min(a->dr, a->cr)) / 100.0;
}
