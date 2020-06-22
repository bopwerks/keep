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

time_t currdate;

extern int yyerror(const char *s);
extern int yylex(void);

typedef struct transaction transaction;

extern long cents(char *num);

extern void connect(void);


%}
%union {
    long val;
    char *str;
    account *acct;
    expr *exp;
    double dval;
    time_t time;
    struct transaction *trans;
}
%token <time> DATE
%token ARROW
%token CREDIT
%token DEBIT
%token TRACK
%token <val> INCOMETOK
%token SEPARATOR
%token <val> ACCOUNT
%token <str> STRING
%token <str> NUMBER
%token <str> NAME
%token TERM
%type <acct> connection
/*%type <acct> accountexpr*/
%type <trans> initial_balance
%type <acct> accountdef
/*%type <acct> account*/
%type <acct> variable
%type <str> comment
%type <exp> varexpr
%type <acct> account

%left ARROW
%left DEBIT CREDIT
%left '+' '-'
%left '*' '/'
%right '^'

%%

file: defs SEPARATOR { connect(); } transactions
    | /* nothing */
    ;

defs: defs connection TERM
    | defs variable TERM
    | /* nothing */
    ;

transactions: transactions transaction TERM
            |
            ;

connection: connection ARROW accountdef { if (!account_connect($1, $3)) {
                                            return yyerror("Can't connect different account types");
                                          }
                                          $$ = $3;
                                        }
          | accountdef { $$ = $1; }
          | NAME { $$ = account_find($1); }
          ;

accountdef: ACCOUNT NAME STRING initial_balance {
  account *a = account_new($1, $2, $3);
  $$ = a;
  if ($4 != NULL)
    addtrans(a, $4);
} | INCOMETOK NAME STRING initial_balance {
  account *a = account_new($1, $2, $3);
  $$ = a;
  if ($4 != NULL)
    addtrans(a, $4);
}

initial_balance: DEBIT  NUMBER comment { $$ = newtrans(0, $3, cents($2), 0); }
               | CREDIT NUMBER comment { $$ = newtrans(0, $3, 0, cents($2)); }
               | /* nothing */         { $$ = NULL; }
               ;

comment: STRING
       | { $$ = NULL; }
       ;

transaction: DATE STRING {
    currdate = $1;
    strcpy(explanation, $2);
    totdr = totcr = 0;
} entries;

entries: entries entry
       | entry
       ;

entry: account DEBIT NUMBER comment { totdr += cents($3);
                                   addtrans($1, newtrans(currdate, trexplanation, cents($3), 0.0));
                                   }
     | account CREDIT NUMBER comment { totcr += cents($3);
                                    addtrans($1, newtrans(currdate, trexplanation, 0, cents($3)));
                                    }
     ;

account: NAME { $$ = account_find($1); }

variable: TRACK NAME STRING varexpr { $$ = tracker_new($2, $3, $4); };

varexpr: varexpr '+' varexpr { $$ = expr_new(EXPR_ADD, NULL, 0, $1, $3); }
       | varexpr '-' varexpr { $$ = expr_new(EXPR_SUB, NULL, 0, $1, $3); }
       | varexpr '*' varexpr { $$ = expr_new(EXPR_MUL, NULL, 0, $1, $3); }
       | varexpr '/' varexpr { $$ = expr_new(EXPR_DIV, NULL, 0, $1, $3); }
       | varexpr '^' varexpr { $$ = expr_new(EXPR_EXP, NULL, 0, $1, $3); }
       | '(' varexpr ')' { $$ = $2; }
       | NUMBER { sscanf($1, "%lf", &dval); $$ = expr_new(EXPR_NUM, NULL, dval, NULL, NULL); }
       | NAME { $$ = expr_new(EXPR_ID, account_find($1), 0, NULL, NULL); }
       | INCOMETOK { $$ = expr_new(EXPR_ID, account_find("income"), 0, NULL, NULL); }
       ;
%%
