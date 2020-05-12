%{
#include <string.h>
#include <time.h>

enum { MAXSUB = 64, MAXACCT = 128 };

struct account {
    char *name;
    char *longname;

    double debit;
    double credit;
    
    struct account *accounts[MAXSUB];
    int naccounts;

    int nparents;
    struct transaction **tr;
    int ntr;
    int trcap;
};
typedef struct account account;

static struct account *curacct = NULL;

static account *accounts[MAXACCT];
static int naccounts = 0;

static time_t currdate;
static char explanation[1024];

struct transaction {
    time_t date;
    char *description;
    double debit;
    double credit;
    struct transaction *next;
};
typedef struct transaction transaction;

static int yyerror(const char *s);
int yylex(void);

static void connect(account *parent, account *child);
static account * newacct(char *name, char *longname);
static account * findacct(char *name);
static transaction * newtrans(time_t date, char *description, double debit, double credit);
static void addtrans(account *acct, transaction *tr);

struct tm datetime;

%}
%union {
    time_t time;
    double val;
    char str[1024];
    struct account *acct;
}
%token ACCOUNT
%token <time> DATE
%token ARROW
%token CREDIT
%token DEBIT
%token <str> STRING
%token <val> NUMBER
%token <str> NAME
%type <acct> connections
%type <acct> accountexpr
%type <acct> account

%left ARROW
%left DEBIT CREDIT

%%

list: /* nothing */
    | list '\n'
    | list transaction ';'
    | list connections
    ;

connections: connections ARROW accountexpr { connect($1, $3); $$ = $3; }
           | accountexpr { $$ = $1; }
           ;

accountexpr: accountexpr DEBIT NUMBER  { addtrans($1, newtrans(currdate, explanation, $3, 0.0)); $$ = $1; }
| accountexpr CREDIT NUMBER { addtrans($1, newtrans(currdate, explanation, 0.0, $3)); $$ = $1; }
           | account { $$ = $1; }
           ;

account: ACCOUNT NAME STRING { $$ = newacct($2, $3); }
       | NAME { $$ = findacct($1); }
       ;

transaction: DATE STRING {
    currdate = $1;
    strcpy(explanation, $2);
} accountexprs;

accountexprs: accountexprs accountexpr;
            | accountexpr
            ;

%%

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double
debits(account *acct)
{
    double total;
    int i;

    total = acct->debit;
    for (i = 0; i < acct->naccounts; ++i) {
        total += debits(acct->accounts[i]);
    }
    return total;
}

double
credits(account *acct)
{
    double total;
    int i;

    total = acct->credit;
    for (i = 0; i < acct->naccounts; ++i) {
        total += credits(acct->accounts[i]);
    }
    return total;
}

void
print(account *acct, int level)
{
    int i;
    double dr;
    double cr;
    double min;
    double max;

    for (i = 0; i < level; ++i) {
        putchar('\t');
    }
    dr = debits(acct);
    cr = credits(acct);
    min = (dr > cr) ? cr : dr;
    max = (dr > cr) ? dr : cr;
    printf("%s %.2f\n", acct->longname, max - min);
    for (i = 0; i < acct->naccounts; ++i) {
        print(acct->accounts[i], level+1);
    }
}

int
main(void)
{
    int i;
    yyparse();

    for (i = 0; i < naccounts; ++i) {
        if (accounts[i]->nparents == 0)
            print(accounts[i], 0);
        /* printf("%s \"%s\"\n", accounts[i]->name, accounts[i]->longname); */
    }
    return EXIT_SUCCESS;
}

int
yyerror(const char *s)
{
    fprintf(stderr, "%s\n", s);
    return 0;
}

account *
newacct(char *name, char *longname)
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
    return a;
}

account *
findacct(char *name)
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

transaction *
newtrans(time_t date, char *description, double debit, double credit)
{
    transaction *tr;

    tr = malloc(sizeof *tr);
    if (tr == NULL) {
        return NULL;
    }
    tr->date = date;
    tr->description = malloc(strlen(description) + 1);
    if (tr->description == NULL) {
        free(tr);
        return NULL;
    }
    tr->debit = debit;
    tr->credit = credit;
    tr->next = NULL;
    return tr;
}

void
addtrans(account *acct, transaction *tr)
{
    transaction **tmp;
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
    if (tr->debit == 0.0) {
        acct->credit += tr->credit;
        /* fprintf(stderr, "CR %.2f %s\n", tr->credit, acct->name); */
    } else {
        acct->debit += tr->debit;
        /* fprintf(stderr, "DR %.2f %s\n", tr->debit, acct->name); */
    }
}

void
connect(account *parent, account *child)
{
    child->nparents += 1;
    parent->accounts[parent->naccounts++] = child;
}
