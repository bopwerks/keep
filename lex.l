%{
#include <time.h>
#include "keep.h"
#include "y.tab.h"
%}
%option noyywrap

%%

[ \t\n] { ; }
";" { return ';'; }
[0-9][0-9][0-9][0-9][ ](jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[ ]([1-9]|1[0-9]|2[0-9]|3[01]) {
  strptime(yytext, "%d %b %y", &datetime);
  yylval.time = mktime(&datetime);
  /*fprintf(stderr, "DATE<%.*s>\n", (int) yyleng, yytext);*/
  return DATE;
}
[0-9]+\.?|[0-9]*\.[0-9]+ { sscanf(yytext, "%lf", &yylval.val); /*fprintf(stderr, "NUMBER<%lf>\n", yylval.val);*/ return NUMBER; }
account { /*fprintf(stderr, "ACCOUNT\n");*/ return ACCOUNT; }
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
