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

static int error;

extern void yyparse(void);
extern FILE *yyin;

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
static int ledger(char *argv[], int argc);

static void
tr_print(FILE *fp, account *a, transaction *t)
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
    if (t->date == 0) {
        fprintf(fp, "---- --- --");
    } else {
        fprintf(fp, "%d %s %2d", tm.tm_year + 1900,
                monthname(tm.tm_mon),
                tm.tm_mday);
    }
    fprintf(fp, " %s %s %6ld.%02ld \"%s\"\n",
            a->name,
            t->debit == 0.0 ? "cr" : "dr",
            dollars,
            cents,
            t->description);
}

int
main(int argc, char *argv[])
{
    int year;
    int month;
    time_t clock;
    struct tm *tm;

    time(&clock);
    tm = localtime(&clock);

    year = tm->tm_year + 1900;
    month = tm->tm_mon+1;
    setprogname(argv[0]);
    filename = argv[1];
    if (argc < 3) {
        fprintf(stderr, "Usage: %s journal-file {balance|income|ledger|plot} ...\n", argv[0]);
        return EXIT_FAILURE;
    }
    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
    }
    account_new(ASSET, "assets", "Assets");
    account_new(LIABILITY, "liabilities", "Liabilities");
    account_new(EXPENSE, "expenses", "Expenses");
    account_new(INCOME, "income", "Income");

    const_new("e", M_E);
    const_new("pi", M_PI);
    
    yyparse();
    if (error) {
        return EXIT_FAILURE;
    }
    
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
    } else if (!strcmp(argv[2], "ledger")) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s ledger account ...\n", argv[0]);
            return EXIT_FAILURE;
        }
        return ledger(&argv[2], argc-2);
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

struct date {
    int y;
    int m;
};
typedef struct date date;

static int
date_cmp(date *a, date *b)
{
    assert(a != NULL);
    assert(b != NULL);

    if (a->y != b->y)
        return a->y - b->y;
    return a->m - b->m;
}

static void
date_init(date *d, int y, int m)
{
    assert(d != NULL);
    assert(m >= 0 && m <= 11);
    d->y = y;
    d->m = m;
}

static void
date_dec(date *d)
{
    assert(d != NULL);
    if (d->m == 0) {
        d->y -= 1;
        d->m = 11;
    } else {
        d->m -= 1;
    }
}

#define abs(x) ((x < 0) ? -x : x)

static void
date_inc(date *d)
{
    assert(d != NULL);
    if (d->m == 11) {
        d->y += 1;
        d->m = 0;
    } else {
        d->m += 1;
    }
}

static void
date_add(date *d, int delta)
{
    int i;
    for (i = 0; i < abs(delta); ++i)
        ((delta < 0) ? date_dec : date_inc)(d);
}

static int
ledger(char *argv[], int argc)
{
    account *a;
    int i;
    int j;
    for (i = 1; i < argc; ++i) {
        a = account_find(argv[i]);
        if (a == NULL) {
            fprintf(stderr, "%s: %s: Can't find account: %s\n", getprogname(), argv[0], argv[i]);
            return EXIT_FAILURE;
        }
        for (j = 0; j < a->ntr; ++j) {
            tr_print(stdout, a, a->tr[j]);
        }
    }
    return EXIT_SUCCESS;
}

static int
plot(char *argv[], int argc)
{
    enum { NMONTHS = 6 };
    account *accts[10];
    int na;
    FILE *fp;
    account *a;
    char *path;
    int i;
    int nplotted = 0;
    double val;
    char tmp[] = "/tmp/keep.XXX";
    date start;
    date end;
    time_t t;
    
    for (na = 0; na < argc-1; ++na) {
        accts[na] = account_find(argv[na+1]);
        if (accts[na] == NULL) {
            fprintf(stderr, "Can't find trackable object: %s\n", argv[na+1]);
            return EXIT_FAILURE;
        }
    }
    /* fp = stderr; */
    t = time(NULL);
    fprintf(stdout, "set xlabel \"Time\"\n");
    fprintf(stdout, "set grid\n");
    fprintf(stdout, "set xdata time\n");
    fprintf(stdout, "set timefmt \"%%Y-%%m\"\n");
    date_init(&start, getyear(t), getmonth(t)-1);
    end = start;
    date_add(&start, -NMONTHS);
    fprintf(stdout, "set xrange [\"%d-%d\":\"%d-%d\"]\n", start.y, start.m+1, end.y, end.m+1);
    fprintf(stdout, "set format x \"%%Y/%%m\"\n");
    fprintf(stdout, "set xtics (");
    for (i = 0; date_cmp(&start, &end) <= 0; ++i) {
        if (i > 0)
            fprintf(stdout, ", ");
        fprintf(stdout, "\"%d-%d\"", start.y, start.m+1);
        date_inc(&start);
    }
    fprintf(stdout, ")\n");
    /* fprintf(stdout, "set term dumb\n"); */
    /* fprintf(stdout, "set output \"%s.png\"\n", varname); */
    for (i = 0; i < na; ++i) {
        date_init(&start, getyear(t), getmonth(t)-1);
        end = start;
        date_add(&start, -NMONTHS);
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
        while (date_cmp(&start, &end) <= 0) {
            val = a->eval(a, start.y, start.m+1);
            fprintf(fp, "%d-%d\t%lf\n", start.y, start.m+1, val);
            date_inc(&start);
        }
        fclose(fp);
        assert(a != NULL);
        assert(path != NULL);
        assert(a->longname != NULL);
        if (na == 1) {
            fprintf(stdout, "set title \"%s\"\n", a->longname);
        }

        if (nplotted++ == 0) {
            fprintf(stdout, "plot \"%s\" using 1:2 title \"%s\" with linespoints", path, a->longname);
        } else {
            fprintf(stdout, ", \"%s\" using 1:2 title \"%s\" with linespoints", path, a->longname);
        }
    }
    fputs("\n", stdout);
    return EXIT_SUCCESS;
}

static void
print_balance(time_t date)
{
    account *assets;
    account *liabilities;
    double bal;
    
    puts("# Balance Sheet");
    puts("");
    puts("## Assets");
    puts("");
    assets = account_find("assets");
    account_print(assets, date, 0);
    puts("");
    puts("## Liabilities");
    puts("");
    liabilities = account_find("liabilities");
    account_print(liabilities, date, 0);
    bal = account_eval(assets, date) - account_eval(liabilities, date);
    printf("\nNet Worth: %.2f", bal);
    fputs("\n", stdout);
}

static void
print_income(time_t date)
{
    account *income;
    account *expenses;
    double bal;
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
    bal = account_eval(income, date) - account_eval(expenses, date);
    printf("%.2f\n", bal);
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
    transaction **t;
    transaction *tmptr;
    int i;

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
    if (tr->date == 0) {
        a->dr += tr->debit;
        a->cr += tr->credit;
    } else {
        account_bin(a, tr->date, tr->debit, tr->credit);
    }        

    /* Update parent */
    if (a->parent != NULL) {
        addtrans(a->parent, tr);
    }
}
