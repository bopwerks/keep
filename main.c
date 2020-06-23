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

static void print_balance(time_t);
static void print_income(time_t);

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

static int plot(char *argv[], int argc);

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
        fputs(acct->name, stderr);
        fputs("\n", stderr);
        localtime_r(&acct->mindate, &tm);
        strftime(buf1, sizeof buf1, "%Y %b %d", &tm);
        localtime_r(&acct->maxdate, &tm);
        strftime(buf2, sizeof buf2, "%Y %b %d", &tm);
        fprintf(stderr, "range %s (%ld) to %s (%ld)\n", buf1, acct->mindate, buf2, acct->maxdate);
        fputs("\n", stderr);
        if (acct->ntr == 0)
            continue;
        if (acct->typ != ACCT)
            continue;
        for (j = 0; j < acct->ntr; ++j) {
            tr_print(acct, acct->tr[j]);
        }
        if (acct->ntr > 0)
            fputs("\n", stderr);
        for (j = 0; j < acct->nbuckets; ++j) {
            b = acct->buckets[j];
            fprintf(stderr, "%4d %s dr %ld cr %ld\n", b->key / 100, monthname(b->key % 100 - 1), b->dr, b->cr);
        }
        if (acct->nbuckets > 0)
            fputs("\n", stderr);
    }
    */
    /* TODO: Do the same binning process for vars. Take the
     * intersection of the months available for each account named and
     * set start and end dates for the var. Then iterate over the
     * months in the range, evaluating the var with the year/month to
     * produce the bin value for that month. */
    if (!strcmp(argv[2], "balance")) {
        print_balance(time(NULL));
    } else if (!strcmp(argv[2], "income")) {
        print_income(time(NULL));
    } else if (!strcmp(argv[2], "plot")) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s plot tracker-name\n", argv[0]);
            return EXIT_FAILURE;
        }
        return plot(&argv[2], argc-2);
    } else {
        fprintf(stderr, "%s: No such command: %s\n", argv[0], argv[2]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int
getyear(time_t date)
{
    struct tm tm;

    localtime_r(&date, &tm);
    return tm.tm_year + 1900;
}

static int
getmonth(time_t date)
{
    struct tm tm;
    localtime_r(&date, &tm);
    return tm.tm_mon + 1;
}

static int
plot(char *argv[], int argc)
{
    account *accts[10];
    int na;
    FILE *fp;
    account *a;
    char *path;
    int y;
    int j;
    int m;
    int i;
    int found;
    int nfound;
    int nplotted = 0;
    double val;
    char tmp[] = "/tmp/keep.XXX";
    for (na = 0; na+1 < argc; ++na) {
        accts[na] = account_find(argv[na+1]);
        if (accts[na] == NULL) {
            fprintf(stderr, "Can't find trackable object: %s\n", argv[na+1]);
            return EXIT_FAILURE;
        }
    }
    /* fp = stderr; */
    fprintf(stdout, "set xlabel \"Time\"\n");
    fprintf(stdout, "set grid\n");
    for (i = 0; i < na; ++i) {
        a = accts[i];
        strcpy(tmp, "/tmp/keep.XXX");
        path = mktemp(tmp);
        if (path == NULL) {
            fprintf(stderr, "Can't make data file\n");
            return EXIT_FAILURE;
        }
        fp = fopen(path, "w");
        if (fp == NULL) {
            fprintf(stderr, "Can't open data file: %s\n", path);
            return EXIT_FAILURE;
        }
        if (a->maxdate >= a->mindate) {
            for (y = getyear(a->mindate); y <= getyear(a->maxdate); ++y) {
                for (m = getmonth(a->mindate); m <= getmonth(a->maxdate); ++m) {
                    found = 1;
                    val = a->eval(a, y, m, &found);
                    if (found) {
                        fprintf(fp, "%d\t%lf\n", y*100+m, val);
                    }
                }
            }
        }
        fclose(fp);
        if (a->maxdate >= a->mindate) {
            /* fprintf(stdout, "set title \"%s\"\n", a->longname); */
            /* fprintf(stdout, "set xlabel \"Time\"\n"); */
            /* fprintf(stdout, "set term dumb\n"); */
            /* fprintf(stdout, "set xrange [0:%d]\n", monthdist(a->mindate, a->maxdate) + 1); */
            /* fprintf(stdout, "set output \"%s.png\"\n", varname); */
            if (nplotted++ == 0) {
                fprintf(stdout, "plot \"%s\" title \"%s\" with linespoints", path, a->longname);
            } else {
                fprintf(stdout, ", \"%s\" title \"%s\" with linespoints", path, a->longname);
            }
        }
    }
    fputs("\n", stdout);
    return EXIT_SUCCESS;
}

static void
print_amount(FILE *fp, long amt)
{
    int dollars;
    int cents;
    assert(fp != NULL);
    dollars = amt / 100;
    cents = amt % 100;
    fprintf(fp, "%d.%02d", dollars, cents);
}

static void
print_balance(time_t date)
{
    account *assets;
    account *liabilities;
    int i;
    account *acct;
    long bal;
    
    puts("# Balance Sheet");
    puts("");
    puts("## Assets");
    puts("");
    assets = account_find("assets");
    account_print(assets, date, 0);
    /* for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) { */
    /*     if (acct->type != ASSET || acct->nparents != 0) */
    /*         continue; */
    /*     account_print(acct, date, 0); */
    /* } */
    puts("");
    puts("## Liabilities");
    puts("");
    liabilities = account_find("liabilities");
    account_print(liabilities, date, 0);
    /* for (i = 0, acct = accounts[i]; i < naccounts; acct = accounts[++i]) { */
    /*     if (acct->type != LIABILITY || acct->nparents != 0) */
    /*         continue; */
    /*     account_print(acct, year, month, 0); */
    /* } */
    /* TODO: Display net worth */
    bal = account_balance(assets, date) - account_balance(liabilities, date);
    printf("\nNet Worth: ");
    print_amount(stdout, bal);
    fputs("\n", stdout);
}

static void
print_income(time_t date)
{
    account *income;
    account *expenses;
    long bal;
    puts("");
    puts("# Income Statement");
    puts("");
    puts("## Income");
    puts("");
    income = account_find("income");
    account_print(income, date, 0);
    puts("");
    puts("## Expenses");
    puts("");
    expenses = account_find("expenses");
    account_print(expenses, date, 0);
    printf("\nNet Income: ");
    bal = account_balance(income, date) - account_balance(expenses, date);
    print_amount(stdout, bal);
    fputs("\n", stdout);
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

    /* fprintf(stderr, "Adding transaction to %s\n", a->name); */

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
    } else {
        a->startdr += tr->debit;
        a->startcr += tr->credit;
    }

    /* Update parents */
    if (a->parent != NULL) {
        addtrans(a->parent, tr);
    }
}
