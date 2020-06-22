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
void
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
        if (b == a || a->parent != NULL) {
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

static void
tr_print(account *a, transaction *t)
{
    struct tm tm;
    long amount;
    long dollars;
    long cents;
    assert(t != NULL);

    localtime_r(&t->date, &tm);
    amount = (t->debit == 0.0) ? t->credit : t->debit;
    dollars = amount / 100;
    cents = amount % 100;
    fprintf(stderr, "%10lx %d %s %2d %-16s %s %5ld.%02ld \"%s\"\n",
            t->date,
            tm.tm_year + 1900,
            monthname(tm.tm_mon),
            tm.tm_mday,
            a->name,
            t->debit == 0.0 ? "cr" : "dr",
            dollars,
            cents,
            t->description);
}

int
main(int argc, char *argv[])
{
    char buf1[256];
    char buf2[256];
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
    time_t validtime;
    bucket *b;
    range r;

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
        r = expr_range(acct->exp);
        acct->mindate = r.start;
        acct->maxdate = r.end;
    }
    /*
    for (i = 0; i < naccounts; ++i) {
        struct tm tm;
        acct = accounts[i];
        puts(acct->name);
        localtime_r(&acct->mindate, &tm);
        strftime(buf1, sizeof buf1, "%Y %b %d", &tm);
        localtime_r(&acct->maxdate, &tm);
        strftime(buf2, sizeof buf2, "%Y %b %d", &tm);
        printf("range %s (%ld) to %s (%ld)\n", buf1, acct->mindate, buf2, acct->maxdate);
        puts("");
        if (acct->ntr == 0)
            continue;
        if (acct->typ != ACCT)
            continue;
        for (j = 0; j < acct->ntr; ++j) {
            tr_print(acct, acct->tr[j]);
        }
        if (acct->ntr > 0)
            puts("");
        for (j = 0; j < acct->nbuckets; ++j) {
            b = acct->buckets[j];
            fprintf(stderr, "%4d %s dr %ld cr %ld\n", b->key / 100, monthname(b->key % 100 - 1), b->dr, b->cr);
        }
        if (acct->nbuckets > 0)
            puts("");
    }
    */
    return 0;
    /* TODO: Do the same binning process for vars. Take the
     * intersection of the months available for each account named and
     * set start and end dates for the var. Then iterate over the
     * months in the range, evaluating the var with the year/month to
     * produce the bin value for that month. */
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
    /* for (i = 0; i < a->nbuckets; ++i) { */
    /*     fprintf(fp, "%d\t%f\n", i, a->bucket[i]); */
    /* } */
    fclose(fp);
    fprintf(stdout, "set title \"%s\"\n", a->longname);
    fprintf(stdout, "set xlabel \"Time\"\n");
    fprintf(stdout, "set term dumb\n");
    fprintf(stdout, "set grid\n");
    /* fprintf(stdout, "set xrange [0:%d]\n", a->monthcap); */
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
        /* account_print(acct, 0); */
    }
    puts("");
    puts("## Liabilities");
    puts("");
    for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) {
        if (acct->type != LIABILITY || acct->nparents != 0)
            continue;
        /* account_print(acct, year, month, 0); */
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
        /* account_print(acct, year, month, 0); */
    }
    puts("");
    puts("## Expenses");
    puts("");
    for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) {
        if (acct->type != EXPENSE || acct->nparents != 0)
            continue;
        /* account_print(acct, year, month, 0); */
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
newtrans(time_t date, char *description, long debit, long credit)
{
    transaction *tr;

    tr = calloc(1, sizeof *tr + strlen((description != NULL) ? description : "") + 1);
    if (tr == NULL) {
        return NULL;
    }
    tr->date = date;
    tr->description = ((char *) tr) + (sizeof *tr);
    if (description != NULL) {
        strcpy(tr->description, description);
    }
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
addtrans(account *a, transaction *tr)
{
    char buf1[256];
    char buf2[256];
    transaction **tmp;
    transaction **t;
    transaction *tmptr;
    bucket *b;
    int newcap;
    int i;

    fprintf(stderr, "Adding transaction to %s\n", a->name);

    /* Insert transaction into transaction list in ascending time order */
    if (a->ntr == a->trcap) {
        if (!array_grow((void **) &a->tr, a->trcap * 2, sizeof *a->tr)) {
            return;
        }
        a->trcap *= 2;
    }
    t = a->tr;
    t[a->ntr] = tr;
    for (i = a->ntr; i > 0 && cmptr(t[i-1], t[i]) > 0; --i) {
        tmptr = t[i-1];
        t[i-1] = t[i];
        t[i] = tmptr;
    }
    a->ntr += 1;

    /* Update date range */
    if (tr->date != 0) {
        if (a->mindate == 0)
            a->mindate = tr->date;
        else
            a->mindate = (tr->date < a->mindate) ? tr->date : a->mindate;
        if (a->maxdate == 0)
            a->maxdate = tr->date;
        else
            a->maxdate = (tr->date > a->maxdate) ? tr->date : a->maxdate;
        account_bin(a, tr->date, tr->debit, tr->credit);
    }

    /* Update parents */
    if (a->parent != NULL) {
        addtrans(a->parent, tr);
    }
}
