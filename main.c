#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include "transaction.h"
#include "account.h"
#include "track.h"
#include "util.h"

extern int naccounts;
extern int nlines;

static struct account *curacct = NULL;

static int error;

extern void yyparse(void);
extern FILE *yyin;

static char *progname;
static char *filename;

static void print_balance(int year, int month);
static void print_income(int year, int month);

/* Connect built-in accounts (income, expenses, liabilities, assets)
 * to the root accounts of their type.
 */
static void
connect(void)
{
    static char *name[] = {
        "assets",
        "liabilities",
        "income",
        "expenses"
    };

    account *a;
    account *b;
    int i;
    int j;

    for (i = 0, a = accounts[i]; i < naccounts; a = accounts[++i]) {
        b = account_find(name[a->type]);
        if (b == a || a->nparents != 0) {
            continue;
        }
        assert(b != NULL);
        account_connect(b, a);
        for (j = 0; j < a->ntr; ++j) {
            addtrans(b, a->tr[j]);
        }
    }
}

static int plot(char *varname, int year, int month);

/* static void */
/* yearmonth(time_t date, int *year, int *month) */
/* { */
/*     struct tm *t; */
/*     assert(t != NULL); */
/*     assert(year != NULL); */
/*     assert(month != NULL); */

/*     t = localtime(&date); */
/*     *year = t->tm_year + 1900; */
/*     *month = t->tm_mon + 1; */
/* } */

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
    double dval;
    int ok;
    int minyear;
    int maxyear;
    int minmonth;
    int maxmonth;
    int nmonths;

    time(&clock);
    tm = localtime(&clock);

    year = tm->tm_year + 1900;
    month = tm->tm_mon+1;
    progname = argv[0];
    filename = argv[1];
    if (argc < 3) {
        fprintf(stderr, "Usage: %s journal-file {balance|income|plot} ...\n", argv[0]);
        return EXIT_FAILURE;
    }
    /* if (argc == 4) { */
    /*     year = strtol(argv[2], NULL, 10); */
    /*     month = strtol(argv[3], NULL, 10); */
    /* } */
    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
    }
    account_new(ASSET, "assets", "Total Assets");
    account_new(LIABILITY, "liabilities", "Total Liabilities");
    account_new(EXPENSE, "expenses", "Total Expenses");
    account_new(INCOME, "income", "Total Income");
    
    acct = account_new(EXPENSE, "e", "Euler's number");
    acct->typ = NUM;
    acct->dval = M_E;
    acct = account_new(EXPENSE, "pi", "Pi");
    acct->typ = NUM;
    acct->dval = M_PI;
    yyparse();
    if (error) {
        return EXIT_FAILURE;
    }
    connect();
    for (i = 0; i < naccounts; ++i) {
        acct = accounts[i];
        if (acct->typ != ACCT)
            continue;
        /* Adjust starting balances to have the same date as the first
         * real transaction, if there is one.
         */
        if (acct->ntr > 1 && acct->tr[0]->year == 1900) {
            acct->tr[0]->date = acct->tr[1]->date;
            acct->mindate = acct->tr[0]->date;
        }
        /* Compute total available months, allocate space for bins, and compute each bin */
        acct->nmonths = monthdist(acct->mindate, acct->maxdate) + 1;
        /* fprintf(stderr, "%d months of data for account %s\n", acct->nmonths, acct->longname); */
        acct->months = malloc((sizeof *(acct->months)) * acct->nmonths);
        for (j = 0; j < acct->ntr; ++j) {
            acct->bin(acct, acct->tr[j]);
        }
    }
    /* TODO: Do the same binning process for vars. Take the
     * intersection of the months available for each account named and
     * set start and end dates for the var. Then iterate over the
     * months in the range, evaluating the var with the year/month to
     * produce the bin value for that month. */
    for (i = 0; i < naccounts; ++i) {
        acct = accounts[i];
        if (acct->typ != VAR)
            continue;
        /* TODO: Traverse the var expr to compute the start and end
         * dates for the var. Constants have infinite
         * distance. Accounts have whatever distance was computed
         * during the binning process. Vars have whatever is computed
         * here. The start date will be the maximum of the start dates
         * of all vars/accounts named, and the end date will be the
         * minimum of the end dates of all vars/accounts named.
         */
    }
    if (!strcmp(argv[2], "balance")) {
        print_balance(year, month);
    } else if (!strcmp(argv[2], "income")) {
        print_income(year, month);
    } else if (!strcmp(argv[2], "plot")) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s plot tracker-name\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!plot(argv[3], year, month)) {
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "%s: No such command: %s\n", argv[0], argv[2]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int
plot(char *varname, int year, int month)
{
    FILE *fp;
    account *a;
    char *path;
    int y;
    int m;
    int i;
    int ok;
    double val;
    char tmp[] = "/tmp/keep.XXX";
    a = account_find(varname);
    if (a == NULL) {
        fprintf(stderr, "Can't find trackable object: %s\n", varname);
        return 0;
    }
    path = mktemp(tmp);
    if (path == NULL) {
        fprintf(stderr, "Can't make data file\n");
        return 0;
    }
    fp = fopen(path, "w");
    if (fp == NULL) {
        fprintf(stderr, "Can't open data file: %s\n", path);
        return 0;
    }
    /* fprintf(stderr, "Opened data file: %s\n", path); */
    for (i = 0; i < a->nmonths; ++i) {
        fprintf(fp, "%d\t%f\n", i, a->months[i]);
    }
    fclose(fp);
    fprintf(stdout, "set title \"%s\"\n", a->longname);
    fprintf(stdout, "set xlabel \"Time\"\n");
    fprintf(stdout, "set term dumb\n");
    fprintf(stdout, "set grid\n");
    fprintf(stdout, "set xrange [0:%d]\n", a->nmonths);
    /* fprintf(stdout, "set output \"%s.png\"\n", varname); */
    fprintf(stdout, "plot \"%s\" title \"\" with linespoints\n", path);
    return 1;
}

static void
print_balance(int year, int month)
{
    int i;
    account *acct;
    puts("# Balance Sheet");
    puts("");
    puts("## Assets");
    puts("");
    for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) {
        if (acct->type != ASSET || acct->nparents != 0)
            continue;
        account_print(acct, year, month, 0);
    }
    puts("");
    puts("## Liabilities");
    puts("");
    for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) {
        if (acct->type != LIABILITY || acct->nparents != 0)
            continue;
        account_print(acct, year, month, 0);
    }
    /* TODO: Display net worth */
}

static void
print_income(int year, int month)
{
    int i;
    account *acct;
    puts("");
    puts("# Income Statement");
    puts("");
    puts("## Income");
    puts("");
    for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) {
        if (acct->type != INCOME || acct->nparents != 0)
            continue;
        account_print(acct, year, month, 0);
    }
    puts("");
    puts("## Expenses");
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
    tr->date = mktime(date);
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

static int
cmptr(transaction *a, transaction *b)
{
    return a->date - b->date;
}

void
addtrans(account *acct, transaction *tr)
{
    transaction **tmp;
    transaction **t;
    transaction *tmptr;
    int newcap;
    int i;

    /* fprintf(stderr, "Adding transaction '%s' to account %s\n", tr->description, acct->name); */
    if (acct->ntr == acct->trcap) {
        if (!array_grow((void **) &acct->tr, acct->trcap * 2, sizeof *acct->tr)) {
            return;
        }
        /* newcap = acct->trcap * 2; */
        /* tmp = realloc(acct->tr, (sizeof *tmp) * newcap); */
        /* if (tmp == NULL) { */
        /*     return; */
        /* } */
        /* acct->tr = tmp; */
        acct->trcap *= 2;
    }
    t = acct->tr;
    t[acct->ntr] = tr;
    for (i = acct->ntr; i > 0 && cmptr(t[i-1], t[i]) > 0; --i) {
        tmptr = t[i-1];
        t[i-1] = t[i];
        t[i] = tmptr;
    }
    acct->ntr += 1;
    /* fprintf(stderr, "Transaction in Account %s\n", acct->name); */
    /* for (i = 0; i < acct->ntr; ++i) { */
    /*     fprintf(stderr, "%02d-%02d-%02d %s dr %ld cr %ld\n", */
    /*             t[i]->year, t[i]->month, t[i]->day, t[i]->description, t[i]->debit, t[i]->credit); */
    /* } */
    /* fputc('\n', stderr); */
    acct->mindate = (acct->mindate == 0 || tr->date < acct->mindate) ? tr->date : acct->mindate;
    acct->maxdate = (acct->maxdate == 0 || tr->date > acct->maxdate) ? tr->date : acct->maxdate;
    if (acct->parent != NULL) {
        /* fprintf(stderr, "Parent of %s is %s\n", acct->name, acct->parent->name); */
        addtrans(acct->parent, tr);
    }
}

long
cents(char *num)
{
    long val;
    int cents;
    int i;
    int len;

    len = strlen(num);
    val = 0;
    for (i = 0; i < len && num[i] != '.'; ++i) {
        val = val * 10 + (num[i] - '0');
    }
    val *= 100;
    cents = 0;
    for (++i; i < len; ++i) {
        cents = cents * 10 + (num[i] - '0');
    }
    val += cents;
    return val;
}
