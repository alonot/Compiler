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
	#include <stdio.h>
	#include "../include/includes.h"
extern SymbolTables* symbol_tables;
	int yylex();
	void yyerror( const char* );
	int i;	
	short write = 0;
%}

%define parse.error verbose

%token BEG END
%token READ WRITE
%token DECL ENDDECL
%token IF THEN ELSE ENDIF
%token LOGICAL_AND LOGICAL_NOT LOGICAL_OR
%token EQUALEQUAL LESSTHANOREQUAL GREATERTHANOREQUAL NOTEQUAL
%token WHILE DO ENDWHILE FOR 
%token T F 
%token MAIN RETURN

%union {
	char* name; // variable name
	int num; // integer value
}
%token <name> VAR 
%token <num> NUM
%token <num> T_INT T_BOOL

 

%left '<' '>'
%left EQUALEQUAL LESSTHANOREQUAL GREATERTHANOREQUAL NOTEQUAL
%left '+' '-'
%left '*' '/'
%left '%'
%left LOGICAL_AND LOGICAL_OR
%left LOGICAL_NOT

%type <num> data_type
%type <num> expr
%type <name> var_expr str_expr
%%

	Prog :	Gdecl_sec prog_block
		;

	prog_block : /*Nothing*/ {}
		| prog_block stmt_list
		| prog_block BEG stmt_list END {/*all changes here are to globals variables*/}
		| Fdef_sec MainBlock	{ }
		;
	
	func_body : BEG stmt_list ret_stmt END
		;

	data_type : 	T_INT		{ $$ = 1; /*for integer */}
		| T_BOOL { $$ = 2; /*for boolean*/}
		;

	Gdecl_sec:	DECL decl_list  ENDDECL {
		char** keys = keys_at(symbol_tables);
		int len = len_at(symbol_tables);
		printf("DECL ");
		for (int i =0; i < len; i ++) {
			printf("%s", keys[i]);
			if (i != len - 1) {
				printf(",");
			}
		}
		free(keys);
		printf("\n");
	} 
		;
		
	decl_list: 
		| 	decl decl_list
		;
		
	decl 	:	data_type list ';' { 	 }
		;
		
		
	list 	:	id
		| 	func 
		|	id ',' list 
		|	func ',' list
		;
	
	id	:	VAR		{ 		upsert_to(symbol_tables, $1, 0);		}
		|	id '[' NUM ']'	{       }
		;
		
	func 	:	VAR '(' arg_list ')' 					{ 					}
		;
			
	arg_list:	
		|	arg_list1
		;
		
	arg_list1:	arg_list1 ';' arg
		|	arg
		;
		
	arg 	:	data_type var_list	
		;

	var_list:	VAR 		 { }
		|	VAR ',' var_list { 	}
		;
		
	Fdef_sec:	
		|	Fdef_sec Fdef
		;
		
	Fdef	:	func_ret_type func_name {add_local_symbol_table(symbol_tables);} '(' FargList ')' '{' Ldecl_sec func_body '}'	{	remove_local_symbol_table(symbol_tables); 				}
		;
		
	func_ret_type:	data_type {}
		;
			
	func_name:	VAR		{ 					}
		;
		
	FargList:	arg_list	{ 					}
		;

	ret_stmt:	RETURN expr ';'	{ 					}
		;
			
	MainBlock: 	func_ret_type main {add_local_symbol_table(symbol_tables);} '('')''{' Ldecl_sec func_body  '}'		{ 	remove_local_symbol_table(symbol_tables);}
					  
		;
		
	main	:	MAIN		{ 					}
		;
		
	Ldecl_sec:	DECL {} decl_list ENDDECL
		;

	stmt_list:	/* NULL */		{  }
		|	statement stmt_list	{		}
		|	error ';' 		{  }
		;

	statement:	assign_stmt  ';'		{ 							 }
		|	read_stmt ';'		{ }
		|	write_stmt ';'		{ }
		|	cond_stmt 		{ }
		|	func_stmt ';'		{ }
		;

	read_stmt:	READ '(' var_expr ')' {						 }
		;

	write_stmt:	WRITE '(' '"' {printf("call write \"");} str_expr '"' ')'      { printf("%s", $5); printf("\"\n");}
		| WRITE  '(' { write = 1; printf("call write ");} param_list ')' 	{ write= 0; printf("\n");}
		;
	
	assign_stmt:	var_expr '=' {printf("ASSIGN %s ", $1);} expr 	{ 		upsert_to(symbol_tables, $1, (char*)$4);			}
		;

	cond_stmt:	IF expr THEN stmt_list ENDIF 	{ 						}
		|  IF expr THEN stmt_list ELSE stmt_list ENDIF 	{ 						}
	    |  FOR '(' assign_stmt  ';'  expr ';'  assign_stmt ')' '{' stmt_list '}' {}
		|  WHILE '(' expr ')' DO stmt_list ENDWHILE
		;
	
	func_stmt:	func_call 		{ 						}
		;
		
	func_call:	VAR '(' param_list ')'	{ 						   }
		;
		
	param_list:				
		|	param_list1		
		;
		
	param_list1:	para			
		|	para ',' {printf(", ");} param_list1	
		;

	para	:	expr			{ 			
		if (write == 1) {
			// printf("%d\n", $1);
		}	
	}

		;

	expr	:	NUM 			{ 	$$ = $1; printf("%d\n",$1 );	}
		|	'-' NUM			{  			$$ = -1 * $2; printf("-%d\n",$2 );			   }
		|	var_expr		{ 	$$ = value_Of(symbol_tables, (char*)$1); printf("%s", $1);	}
		|	T			{ 			$$ = 1;			  	}
		|	F			{  $$ = 0;	}
		|	'(' expr ')'		{  			}

		|	expr '+' expr 		{ 		$$ = $1 + $3;		printf("+");		}
		|	expr '-' expr	 	{ 			$$ = $1 - $3;			printf("-");}
		|	expr '*' expr 		{ 	$$ = $1 * $3;		printf("*");}
		|	expr '/' expr 		{ 			$$ = $1 / $3;		printf("DIV ");}
		|	expr '%' expr 		{ 		$$ = $1 % $3;				printf(" MOD ");}
		|	expr '<' expr		{ 			$$ = $1 < $3;			printf("+");}
		|	expr '>' expr		{ 			$$ = $1 > $3;			printf("+");}
		|	expr GREATERTHANOREQUAL expr				{ $$ = $1 >= $3;printf("+");}
		|	expr LESSTHANOREQUAL expr	{  			$$ = $1 <= $3;			printf("+");}
		|	expr NOTEQUAL expr			{ 			$$ = $1 != $3;			printf("+");}
		|	expr EQUALEQUAL expr	{ 		$$ = $1 == $3;				printf("+");}
		|	LOGICAL_NOT expr	{ 				$$ = !$2;		}
		|	expr LOGICAL_AND expr	{ 			$$ = $1 & $3;			printf("+");}
		|	expr LOGICAL_OR expr	{ 		$$ = $1 | $3;				printf("+");}
		|	func_call		{  }

		;
	str_expr :  VAR                       { $$ = $1; }
                  | str_expr VAR   { $$ = strcat($1, $2);}
                ;
	
	var_expr:	VAR	{ 		$$ = $1;	  }
		|	var_expr '[' expr ']'	{ $$ = $1 ;}
		;
%%

int	lineno = 1;

void yyerror ( const char  *s) {
	/* fprintf(stderr, "%s: %s", progname, s); */
	/* fprintf(stderr, " %s %d\n", yylval.name, yylval.num); */
	fprintf (stderr, "%s", s);
	fprintf(stderr, " near line %d\n", lineno);
	exit(0);
 }


