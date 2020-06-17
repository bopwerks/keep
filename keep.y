%{
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "account.h"
#include "track.h"
#include "transaction.h"

static long totdr = 0;
static long totcr = 0;
static char explanation[1024];
static char trexplanation[1024];
static double dval;

struct tm currdate;

extern int yyerror(const char *s);
extern int yylex(void);

typedef struct transaction transaction;

extern long cents(char *num);


%}
%union {
    long val;
    char str[1024];
    account *acct;
    expr *exp;
    double dval;
}
%token DATE
%token ARROW
%token CREDIT
%token DEBIT
%token TRACK
%token <val> ACCOUNT
%token <str> STRING
%token <str> NUMBER
%token <str> NAME
%type <acct> connections
%type <acct> accountexpr
%type <acct> account
%type <acct> tracker
%type <exp> trackexpr

%left ARROW
%left DEBIT CREDIT
%left '+' '-'
%left '*' '/'
%right '^'

%%

list: /* nothing */
    | list '\n'
    | list transaction ';' { if (totdr != totcr) {
                               return yyerror("Total debits do not match total credits");
                             } }
    | list connections ';'
    | list tracker ';'
    ;

connections: connections ARROW accountexpr { if ($1->type != $3->type) yyerror("Can't connect different account types"); account_connect($1, $3);
                                             $$ = $3; }
           | accountexpr { $$ = $1; }
           ;

accountexpr: accountexpr DEBIT NUMBER comment { totdr += cents($3);
                                                addtrans($1, newtrans(&currdate, trexplanation, cents($3), 0.0));
                                                $$ = $1; }
           | accountexpr CREDIT NUMBER comment { totcr += cents($3);
                                                 addtrans($1, newtrans(&currdate, trexplanation, 0, cents($3)));
                                                 $$ = $1; }
           | account { $$ = $1; }
           ;

comment: STRING { sprintf(trexplanation, "%s - %s", explanation, $1); }
       | /* nothing */ { strcpy(trexplanation, explanation); }
       ;

account: ACCOUNT NAME STRING { $$ = account_new($1, $2, $3); }
       | NAME { $$ = account_find($1); }
       ;

transaction: DATE STRING {
    strcpy(explanation, $2);
    totdr = totcr = 0;
} accountexprs;

accountexprs: accountexprs accountexpr;
            | accountexpr
            ;

tracker: TRACK NAME STRING trackexpr { $$ = tracker_new($2, $3, $4); };

trackexpr: trackexpr '+' trackexpr { $$ = expr_new(EXPR_ADD, NULL, 0, $1, $3); }
         | trackexpr '-' trackexpr { $$ = expr_new(EXPR_SUB, NULL, 0, $1, $3); }
         | trackexpr '*' trackexpr { $$ = expr_new(EXPR_MUL, NULL, 0, $1, $3); }
         | trackexpr '/' trackexpr { $$ = expr_new(EXPR_DIV, NULL, 0, $1, $3); }
         | trackexpr '^' trackexpr { $$ = expr_new(EXPR_EXP, NULL, 0, $1, $3); }
         | '(' trackexpr ')' { $$ = $2; }
| NUMBER { sscanf($1, "%lf", &dval); $$ = expr_new(EXPR_NUM, NULL, dval, NULL, NULL); }
         | NAME { $$ = expr_new(EXPR_ID, account_find($1), 0, NULL, NULL); }
         ;

%%
