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
static double eval_expense_income(account *a, int y, int m, int *found);
static double eval_asset_liability(account *a, int y, int m, int *found);

account *
account_new(account_type type, char *name, char *longname)
{
    account *a;

    /* fprintf(stderr, "Creating new account '%s'\n", name); */
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
    a->mindate = 0;
    a->maxdate = 0;
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
    /* fprintf(stderr, "Connecting %s <- %s\n", parent->name, child->name); */
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

static int
datekey(time_t t)
{
    struct tm tm;
    localtime_r(&t, &tm);
    return (tm.tm_year + 1900) * 100 + (tm.tm_mon + 1);
}

long
account_balance(account *a, time_t date)
{
    struct tm tm;
    bucket **b;
    bucket *c;
    int y;
    int m;
    int key;
    int i;
    /* TODO: If date is less than earliest bucket, return starting
     * balance or error. If it's greater than last bucket, return last
     * bucket
     */
    assert(a != NULL);
    localtime_r(&date, &tm);
    y = tm.tm_year + 1900;
    m = tm.tm_mon + 1;
    key = y * 100 + m;

    b = find_bucket(a, date);
    if (b != NULL) {
        return max((*b)->dr, (*b)->cr) - min((*b)->dr, (*b)->cr);
    }
    if (a->type == INCOME || a->type == EXPENSE) {
        return 0;
    }
    if (datekey(date) < datekey(a->mindate) || a->nbuckets == 0) {
        /* TODO: Error */
        return 0;
    }
    for (i = 0; i < a->nbuckets && a->buckets[i]->key < key; ++i)
        ;
    if (i > 0) {
        c = a->buckets[i-1];
        return max(c->dr, c->cr) - min(c->dr, c->cr);
    }
    return 0;
}

void
account_print(account *acct, time_t date, int level)
{
    int i;
    long bal;
    int dollars;
    int cents;

    assert(acct != NULL);
    bal = account_balance(acct, date);
    dollars = bal / 100;
    cents = bal % 100;
    for (i = 0; i < level; ++i) {
        putchar('\t');
    }
    printf("%s %d.%02d\n", acct->longname, dollars, cents);
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
    /* fprintf(stderr, "Looking for account '%s'\n", name); */
    for (i = 0; i < naccounts; ++i) {
        assert(accounts[i] != NULL);
        assert(accounts[i]->name != NULL);
        if (strcmp(accounts[i]->name, name) == 0)
            return accounts[i];
    }
    fprintf(stderr, "Can't find account: %s\n", name);
    return NULL;
}

static int
cmp(const void *x, const void *y)
{
    int key;
    bucket **b;

    key = *((int *) x);
    b = (bucket **) y;
    /* fprintf(stderr, "Search Key = %d Bucket Key = %d\n", key, (*b)->key); */

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
        /* fprintf(stderr, "Can't find key %d\n", key); */
        return NULL;
    } else {
        /* fprintf(stderr, "Found key %d\n", key); */
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
        /* fprintf(stderr, "Can't find key %d in account %s\n", key, a->name); */
        return NULL;
    } else {
        /* fprintf(stderr, "Found key %d in %s\n", key, a->name); */
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

    /* fprintf(stderr, "%s nbuckets = %d maxbuckets = %d\n", a->name, a->nbuckets, a->maxbuckets); */
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
    /* fprintf(stderr, "Added bucket %d\n", b->key); */
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
                /* fprintf(stderr, "Initializing bucket %d with account %s starting dr %ld cr %ld\n", */
                /*         (*b)->key, a->name, a->startdr, a->startcr); */
                (*b)->dr = a->startdr;
                (*b)->cr = a->startcr;
            } else {
                c = *(b - 1);
                /* Initialize bucket with last month's ending balance */
                /* fprintf(stderr, "Initializing account %s bucket %d with bucket %d dr %ld cr %ld\n", */
                /*         a->name, (*b)->key, c->key, c->dr, c->cr); */
                (*b)->dr = c->dr;
                (*b)->cr = c->cr;
            }
        }
    }
    /* fprintf(stderr, "Updated bucket %d for account %s dr %ld cr %ld\n", (*b)->key, a->name, dr, cr); */
    (*b)->dr += dr;
    (*b)->cr += cr;
    /* fprintf(stderr, "account %-10s bucket %6d dr %6ld cr %6ld\n", a->name, (*b)->key, (*b)->dr, (*b)->cr); */
}

static double
eval_expense_income(account *a, int y, int m, int *found)
{
    bucket **b;
    
    b = find_bucket_by_key(a, y, m);
    if (found != NULL) {
        *found = (b != NULL);
    }
    return (b == NULL) ? 0.0 : (max((*b)->dr, (*b)->cr) - min((*b)->dr, (*b)->cr)) / 100.0;
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
eval_asset_liability(account *a, int y, int m, int *found)
{
    bucket **b;

    b = fuzzy_find_bucket_by_key(a, y, m);
    if (found != NULL) {
        *found = (b != NULL);
    }
    if (b != NULL) {
        return (max((*b)->dr, (*b)->cr) - min((*b)->dr, (*b)->cr)) / 100.0;
    }
    return (max(a->startdr, a->startcr) - min(a->startdr, a->startcr)) / 100.0;
}
