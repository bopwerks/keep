#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "account.h"

extern int naccounts;
extern int nlines;

static struct account *curacct = NULL;

struct transaction {
    int year;
    int month;
    int day;
    char *description;
    long debit;
    long credit;
    struct transaction *next;
};
typedef struct transaction transaction;

static int error;

extern void yyparse(void);

int
main(int argc, char *argv[])
{
    int i;
    int year;
    int month;

    year = 0;
    month = 0;
    if (argc == 3) {
        year = strtol(argv[1], NULL, 10);
        month = strtol(argv[2], NULL, 10);
    }
    yyparse();
    if (error) {
        return EXIT_FAILURE;
    }
    for (i = 0; i < naccounts; ++i) {
        if (accounts[i]->nparents == 0)
            account_print(accounts[i], year, month, 0);
        /* printf("%s \"%s\"\n", accounts[i]->name, accounts[i]->longname); */
    }
    return EXIT_SUCCESS;
}

int
yyerror(const char *s)
{
    fprintf(stderr, "%d: %s\n", nlines, s);
    error = 1;
    return 0;
}

transaction *
newtrans(struct tm *date, char *description, long debit, long credit)
{
    transaction *tr;

    assert(date != NULL);
    tr = malloc(sizeof *tr);
    if (tr == NULL) {
        return NULL;
    }
    tr->year = date->tm_year + 1900;
    tr->month = date->tm_mon + 1;
    tr->day = date->tm_mday;
    tr->description = malloc(strlen(description) + 1);
    if (tr->description == NULL) {
        free(tr);
        return NULL;
    }
    strcpy(tr->description, description);
    tr->debit = debit;
    tr->credit = credit;
    tr->next = NULL;
    return tr;
}

void
addtrans(account *acct, transaction *tr)
{
    transaction **tmp;
    period *year;
    period *month;
    int newcap;

    if (acct->ntr == acct->trcap) {
        newcap = acct->trcap * 2;
        tmp = realloc(acct->tr, (sizeof *tmp) * newcap);
        if (tmp == NULL) {
            return;
        }
        acct->tr = tmp;
        acct->trcap = newcap;
    }
    acct->tr[acct->ntr++] = tr;
    /* Find year and insert if not found */
    for (year = acct->year; year != NULL && year->n != tr->year; year = year->right)
        ;
    if (year == NULL) {
        /* fprintf(stderr, "Adding year %d to account %s\n", tr->year, acct->name); */
        year = period_new(tr->year);
        year->right = acct->year;
        acct->year = year;
    }
    /* Find month and insert if not found */
    for (month = year->left; month != NULL && month->n != tr->month; month = month->right)
        ;
    if (month == NULL) {
        /* fprintf(stderr, "Adding month %d to account %s\n", tr->month, acct->name); */
        month = period_new(tr->month);
        month->right = year->left;
        year->left = month;
    }
    if (tr->debit == 0.0) {
        month->credits += tr->credit;
        /* fprintf(stderr, "CR %.2f\t%s\t%s\n", tr->credit, acct->name, tr->description); */
    } else {
        month->debits += tr->debit;
        /* fprintf(stderr, "DR %.2f\t%s\t%s\n", tr->debit, acct->name, tr->description); */
    }
}
