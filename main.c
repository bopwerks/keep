#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "transaction.h"
#include "account.h"

extern int naccounts;
extern int nlines;

static struct account *curacct = NULL;

static int error;

extern void yyparse(void);
extern FILE *yyin;

static char *progname;
static char *filename;

static void print_statements(int year, int month);

static int
cmptr(const void *a, const void *b)
{
    transaction *x;
    transaction *y;

    x = *((transaction **) a);
    y = *((transaction **) b);

    if (x->year != y->year)
        return x->year - y->year;
    if (x->month != y->month)
        return x->month - y->month;
    return x->day - y->day;
}

int
main(int argc, char *argv[])
{
    transaction *tr;
    account *acct;
    int i;
    int j;
    int year;
    int month;
    time_t clock;
    struct tm *tm;

    time(&clock);
    tm = localtime(&clock);

    year = tm->tm_year + 1900;
    month = tm->tm_mon+1;
    progname = argv[0];
    filename = argv[1];
    if (argc < 2) {
        fprintf(stderr, "Usage: %s journal-file ...\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 4) {
        year = strtol(argv[2], NULL, 10);
        month = strtol(argv[3], NULL, 10);
    }
    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
    }
    yyparse();
    if (error) {
        return EXIT_FAILURE;
    }
    for (i = 0; i < naccounts; ++i) {
        acct = accounts[i];
        qsort(acct->tr, acct->ntr, sizeof *(acct->tr), cmptr);
        for (j = 0, tr = acct->tr[j]; j < acct->ntr; tr = acct->tr[++j]) {
            tr->totaldebits = tr->debit;
            tr->totalcredits = tr->credit;
            if (j > 0) {
                tr->totaldebits += acct->tr[j-1]->totaldebits;
                tr->totalcredits += acct->tr[j-1]->totalcredits;
            }
        }
        /* if (acct->nparents == 0) */
        /*     account_print(acct, year, month, 0); */
        /* printf("%s \"%s\"\n", accounts[i]->name, accounts[i]->longname); */
    }
    print_statements(year, month);
    return EXIT_SUCCESS;
}

static void
print_statements(int year, int month)
{
    int i;
    account *acct;
    puts("# Financial Report");
    puts("");
    puts("## Balance Sheet");
    puts("");
    puts("### Assets");
    puts("");
    for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) {
        if (acct->type != ASSET || acct->nparents != 0)
            continue;
        account_print(acct, year, month, 0);
    }
    puts("");
    puts("### Liabilities");
    puts("");
    for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) {
        if (acct->type != LIABILITY || acct->nparents != 0)
            continue;
        account_print(acct, year, month, 0);
    }
    /* TODO: Display net worth */
    puts("");
    puts("## Income Statement");
    puts("");
    puts("### Income");
    puts("");
    for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) {
        if (acct->type != INCOME || acct->nparents != 0)
            continue;
        account_print(acct, year, month, 0);
    }
    puts("");
    puts("### Expenses");
    puts("");
    for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) {
        if (acct->type != EXPENSE || acct->nparents != 0)
            continue;
        account_print(acct, year, month, 0);
    }
    /* TODO: Display cashflow */
}

int
yyerror(const char *s)
{
    fprintf(stderr, "%s:%d: %s\n", filename, nlines, s);
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
    tr->totaldebits = 0;
    tr->totalcredits = 0;
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
