%{
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "account.h"

static long totdr = 0;
static long totcr = 0;
static char explanation[1024];
static char trexplanation[1024];

struct tm currdate;

extern int yyerror(const char *s);
extern int yylex(void);

typedef struct transaction transaction;

extern transaction * newtrans(struct tm *date, char *description, long debit, long credit);
extern void addtrans(account *acct, transaction *tr);

%}
%union {
    long val;
    char str[1024];
    struct account *acct;
}
%token ACCOUNT
%token DATE
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
    | list transaction ';' { if (totdr != totcr) {
                               return yyerror("Total debits do not match total credits");
                             } }
    | list connections
    ;

connections: connections ARROW accountexpr { account_connect($1, $3);
                                             $$ = $3; }
           | accountexpr { $$ = $1; }
           ;

accountexpr: accountexpr DEBIT NUMBER comment { totdr += $3;
                                                addtrans($1, newtrans(&currdate, trexplanation, $3, 0.0));
                                                $$ = $1; }
           | accountexpr CREDIT NUMBER comment { totcr += $3;
                                                 addtrans($1, newtrans(&currdate, trexplanation, 0.0, $3));
                                                 $$ = $1; }
           | account { $$ = $1; }
           ;

comment: STRING { sprintf(trexplanation, "%s - %s", explanation, $1); }
       | /* nothing */ { strcpy(trexplanation, explanation); }
       ;

account: ACCOUNT NAME STRING { $$ = account_new($2, $3); }
       | NAME { $$ = account_find($1); }
       ;

transaction: DATE STRING {
    strcpy(explanation, $2);
    totdr = totcr = 0;
} accountexprs;

accountexprs: accountexprs accountexpr;
            | accountexpr
            ;

%%
