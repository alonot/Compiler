/*
 *   This file is part of SIL Compiler.
 *
 *  SIL Compiler is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SIL Compiler is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SIL Compiler.  If not, see <http://www.gnu.org/licenses/>.
 */

%{
	#include "string.h"
	#include "y.tab.h"
	int 	lineno = 1;
	int     wflag;

%}

%option noyywrap
digit	[0-9]
letter	[a-zA-Z]

%%
main	                 	{ return MAIN; }	
return 			{ return RETURN; }
begin				{ return BEG; }	
end			{ return END; }	
read				{ return READ; }
write			{ return WRITE; }	
integer			{ yylval.num = 1; return T_INT; }	
boolean				{ yylval.num = 2; return T_BOOL; }	
double				{ yylval.num = 2; return T_DOUBLE; }	
decl				{ return DECL; }
begindecl				{ return DECL; }
enddecl				{ return ENDDECL; }
if				{ return IF; }
then				{ return THEN; }	
else				{ return ELSE; }	
endif				{ return ENDIF; }
do				{ return DO;}
while				{ return WHILE; }
endwhile			{ return ENDWHILE; }
for 				{ return FOR; }		
"=="				{ return EQUALEQUAL; }	
"<="				{ return LESSTHANOREQUAL; }
">="				{ return GREATERTHANOREQUAL; }
"!="				{ return NOTEQUAL; }	
AND				{ return LOGICAL_AND; }	
OR				{ return LOGICAL_OR; }
NOT			{ return LOGICAL_NOT; }
true			{ return T; }	
false			      { return F; }

{digit}+			{ yylval.num = atoi(yytext);	return NUM; }

{letter}({letter}|{digit})*	{ yylval.name = strdup(yytext);return VAR; /* variable name */}
[-+*/=()><;,\{\}\[\]\%]         { return yytext[0]; }
			
["]                             { return yytext[0]; }
"\\n"                           { }
[\n]					{ lineno ++; }
[ \t]
.				{ printf("Unrecognized token : %s\n", yytext); exit(1); }		

%%
#include "../include/includes.h"
SymbolTables* symbol_tables;
int main(){
	symbol_tables = init_sym_tables();
	yyparse();
	freeAll(symbol_tables);
	free(symbol_tables);
	return 0;
}
