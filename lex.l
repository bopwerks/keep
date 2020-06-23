%{
#include <time.h>
#include "account.h"
#include "track.h"
#include "id.h"
#include "y.tab.h"

FILE *yyin;
static struct tm tm;
int nlines = 1;

char name[64];
char str[512];
char num[16];

%}
%option noyywrap
%option noinput
%option nounput

%%

#[^\n]*[\n] { ; }
[ \t] { ; }
[\n] { ++nlines; }
"%" { return SEPARATOR; }
";" { return TERM; }
[0-9][0-9][0-9][0-9][ ](jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[ ]([1-9]|1[0-9]|2[0-9]|3[01]) {
  strptime(yytext, "%Y %b %d", &tm);
  yylval.time = mktime(&tm);
  return DATE;
}
[0-9]+\.?|[0-9]*\.[0-9][0-9] { strncpy(num, yytext, yyleng);
                               num[yyleng] = '\0';
                               yylval.str = num;
                               return NUMBER; }
"track" { return TRACK; }
asset { yylval.val = ASSET; return ACCOUNT; }
liability { yylval.val = LIABILITY; return ACCOUNT; }
income { yylval.val = INCOME; return INCOMETOK; }
expense { yylval.val = EXPENSE; return ACCOUNT; }
["]([^"\\]|\\.)*["] {
  strncpy(str, yytext+1, yyleng-2);
  yylval.str = str;
  str[yyleng-2] = '\0';
  return STRING;
}
[+-/*^()] { return *yytext; }
"dr" { return DEBIT; }
"cr" { return CREDIT; }
[a-zA-Z][a-zA-Z0-9_]* {
  yylval.str = install(yytext, yyleng);
  return NAME;
}
"<-" { return ARROW; }
