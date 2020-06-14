%{
#include <time.h>
#include "account.h"
#include "y.tab.h"

FILE *yyin;
extern struct tm currdate;
int nlines = 1;
static int cents;
static int i;
%}
%option noyywrap

%%

#[^\n]*[\n] { ; }
[ \t] { ; }
[\n] { ++nlines; }
";" { return ';'; }
[0-9][0-9][0-9][0-9][ ](jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[ ]([1-9]|1[0-9]|2[0-9]|3[01]) {
  strptime(yytext, "%Y %b %d", &currdate);
  /*yylval.time = mktime(&datetime);*/
  /*fprintf(stderr, "DATE<%.*s>\n", (int) yyleng, yytext);*/
  return DATE;
}
[0-9]+\.?|[0-9]*\.[0-9][0-9] { yylval.val = 0;
                               for (i = 0; i < yyleng && yytext[i] != '.'; ++i) {
                                 yylval.val = yylval.val * 10 + (yytext[i] - '0');
                               }
                               yylval.val *= 100;
                               cents = 0;
                               for (++i; i < yyleng; ++i) {
                                 cents = cents * 10 + (yytext[i] - '0');
                               }
                               yylval.val += cents;
                           /*sscanf(yytext, "%lf", &yylval.val); fprintf(stderr, "NUMBER<%lf>\n", yylval.val);*/ return NUMBER; }
asset { yylval.val = ASSET; return ACCOUNT; }
liability { yylval.val = LIABILITY; return ACCOUNT; }
income { yylval.val = INCOME; return ACCOUNT; }
expense { yylval.val = EXPENSE; return ACCOUNT; }
["]([^"\\]|\\.)*["] {
  /*fprintf(stderr, "STRING<%.*s>\n", (int) yyleng, yytext);*/
  strncpy(yylval.str, yytext+1, yyleng-2);
  yylval.str[yyleng-2] = '\0';
  return STRING;
}
"dr" { /*fprintf(stderr, "DEBIT\n");*/ return DEBIT; }
"cr" { /*fprintf(stderr, "CREDIT\n");*/ return CREDIT; }
[a-zA-Z][a-zA-Z0-9-]* {
  /*fprintf(stderr, "NAME<%.*s>\n", (int) yyleng, yytext);*/
  strncpy(yylval.str, yytext, yyleng);
  yylval.str[yyleng] = '\0';
  /* TODO: Store symbol as needed for creating an account. Do lookup? */
  return NAME;
}
"<-" { /*fprintf(stderr, "ARROW\n");*/ return ARROW; }
