%{
	#include "stdio.h"
	#include "string.h"
	#include "y.tab.h"
%}
digi [0-9]+
alpha [a-zA-Z]+
ln \n
%%
{digi}\.{digi} {yylval.fnum = atof(yytext);return LITERAL;}
{digi} {yylval.num = atol(yytext);return LITERAL;}
{alpha} {yylval.str = strdup(yytext);return IDENTIFIER;}
[+./*-] {yylval.op = yytext[0];return OP;}
= {return ASSIGN;}
{ln} {return PRINT;}
; {return SEMICOLON;}
\( {return BRACKET_OPEN;} 
\) {return BRACKET_CLOSED;}
[ ] {;}
. {printf("Unrecognized token : %s\n", yytext); exit(1); }
%%
char	*progname;	/* for error messages */
int main(char argc, char* argv[]) {
	progname = argv[0];
	yyparse();
	return 0;
}

int yywrap()
 {
  return 1;
 }
