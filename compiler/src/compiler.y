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
extern Stack* stack;
extern SymbolTables* symbol_tables;
void print_stack();
void pop_single_n_push(lli val,NODETYPE n_type);
void pop_double_n_push(lli val,NODETYPE n_type);
void pop_all_n_push(lli val,NODETYPE n_type);
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
	double final_val; // double answer
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

%type <final_val> expr
%type <num> data_type
%type <name> var_expr str_expr
%type <name> func id
%%

	Prog :	Gdecl_sec prog_block
		;

	prog_block : prog_block_stmt_list
           | Fdef_sec MainBlock { }
           ;

	prog_block_stmt_list : /*Nothing*/ {} 
                    | stmt_list
                    | BEG stmt_list END {/*all changes here are to globals variables*/}
                    ;
	
	func_body : BEG stmt_list ret_stmt END
		;

	data_type : 	T_INT		{ $$ = 1; /*for integer */}
		| T_BOOL { $$ = 2; /*for boolean*/}
		;

	Gdecl_sec:	DECL decl_list  ENDDECL {
		// lli* keys = keys_at(symbol_tables);
		// int len = len_at(symbol_tables);
		// printf("DECL ");
		// for (int i =0; i < len; i ++) {
			// printf("%s", keys[i]);
		// 	if (i != len - 1) {
				// printf(",");
		// 	}
		// }
		// free(keys);
		// printf("\n");
	} 
		;
		
	decl_list: 
		| 	decl decl_list
		;
		
	decl 	:	data_type list ';' { 
		pop_all_n_push((lli)($1 == 1 ? "Integer": "Bool"),(NODETYPE)(t_FUNC));
		print_stack();	
	}
		;
		
		
	list 	:	id { 
			Node* node = init_node((lli)($1), (NODETYPE)(t_STR));
			push_stack(stack, (lli)node); 
			}
		| 	func {
			Node* node = init_node((lli)($1), (NODETYPE)(t_STR));
			push_stack(stack, (lli)node); 
		}
		|	id ',' list {
			Node* node = init_node((lli)($1), (NODETYPE)(t_STR));
			push_stack(stack, (lli)node); 
		}
		|	func ',' list {
			Node* node = init_node((lli)($1), (NODETYPE)(t_STR));
			push_stack(stack, (lli)node); 
		}
		;
	
	id	:	VAR		{ 		
						upsert_to(symbol_tables, (lli)$1, 0);
						$$ = $1;
					}
		|	id '[' NUM ']'	{       }
		;
		
	func 	:	VAR '(' arg_list ')' 					{ 		$$ = $1;			}
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

	stmt_list:	statement		{  }
		|	statement stmt_list	{						}
		|	error ';' 		{  }
		;

	statement:	assign_stmt  ';'		{ 							 }
		|	read_stmt ';'		{ }
		|	write_stmt ';'		{ }
		|	cond_stmt 		{ }
		|	func_stmt ';'		{ }
		;

	read_stmt:	READ '(' var_expr ')' 
		;

	write_stmt:	WRITE '(' '"' {
			// printf("call write \"");
		} str_expr '"' ')'      { 
			Node* node = init_node( (lli)$5, (NODETYPE)(t_STR));
			push_stack(stack, (lli)node);
			// printf("%s", $5); printf("\n");
			pop_single_n_push((lli)"Write", (NODETYPE)(t_FUNC));
			print_stack();
		}
		| WRITE  '(' { 
				write = 1; 
				// printf("call write ");
			} param_list ')' 	{ 
				write= 0; 
				// printf("\n");
				pop_all_n_push((lli)"Write",(NODETYPE)(t_FUNC));
				print_stack();
			}
		;
	
	assign_stmt:	var_expr '=' {
			// printf("ASSIGN %s ", $1);
		} expr 	{ 		
			upsert_to(symbol_tables, (lli)$1, (lli)$4);
			Node* node = init_node((double)value_Of(symbol_tables, (lli)$1), (NODETYPE)(t_VAR));
			push_stack(stack, (lli)node);
			pop_double_n_push(0, (NODETYPE)(t_ASSIGN));
			print_stack();
		}
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
		|	para ',' {
				// printf(", ");
			} param_list1	
		;

	para	:	expr			{ 			
		if (write == 1) {
			printf("%f\n", $1);
			// Node* node = init_node($1, (NODETYPE)(t_VAR));
			// push_stack(stack, (lli)node);
		}	
	}

		;

	expr	:	NUM 			{ 	$$ = $1;
							Node* node = init_node($1, (NODETYPE)(t_NUM));
							push_stack(stack, (lli)node);
								//  printf("%d\n",$1 );	
								}
		|	'-' NUM			{  			$$ = -1 * $2;
							pop_single_n_push(0, (NODETYPE)(t_MINUS));
								//  printf("-%d\n",$2 );			   
							}
		|	var_expr		{ 	$$ = (double)value_Of(symbol_tables, (lli)$1); 
								Node* node = init_node($$, (NODETYPE)(t_VAR));
								push_stack(stack, (lli)node);
							}
		|	T			{	$$ = 1;			  
							Node* node = init_node(1, (NODETYPE)(t_BOOLEAN));
							push_stack(stack, (lli)node);
			}
		|	F			{   $$ = 0;	
							Node* node = init_node(0, (NODETYPE)(t_BOOLEAN));
							push_stack(stack, (lli)node);
						}
		|	'(' expr ')'		{  			}

		|	expr '+' expr 		{ 		$$ = $1 + $3;
									pop_double_n_push('+', (NODETYPE)(t_OP));
									// printf("+");		
								}
		|	expr '-' expr	 	{ 			$$ = $1 - $3;
									pop_double_n_push('-', (NODETYPE)(t_OP));
										// printf("-");
									}
		|	expr '*' expr 		{ 	$$ = $1 * $3;
									pop_double_n_push('*', (NODETYPE)(t_OP));
									// printf("*");
								}
		|	expr '/' expr 		{ 			$$ = $1 / $3;
									pop_double_n_push('/', (NODETYPE)(t_OP));
									// printf("DIV ");
								}
		|	expr '%' expr 		{ 		$$ = (lli)$1 % (lli)$3;
									pop_double_n_push('%', (NODETYPE)(t_OP));
									// printf(" MOD ");
								}
		|	expr '<' expr		{ 			$$ = $1 < $3;
									pop_double_n_push('<', (NODETYPE)(t_OP));
									// printf("+");
								}
		|	expr '>' expr		{ 			$$ = $1 > $3;
									pop_double_n_push('>', (NODETYPE)(t_OP));
									// printf("+");
								}
		|	expr GREATERTHANOREQUAL expr				{ $$ = $1 >= $3;
									pop_double_n_push(0, (NODETYPE)(t_GTE));
									// printf("+");
								}
		|	expr LESSTHANOREQUAL expr	{  			$$ = $1 <= $3;
										// printf("+");
									pop_double_n_push(0, (NODETYPE)(t_LTE));
									}
		|	expr NOTEQUAL expr			{ 			$$ = $1 != $3;
										// printf("+");
									pop_double_n_push(0, (NODETYPE)(t_NE));
									}
		|	expr EQUALEQUAL expr	{ 		$$ = $1 == $3;
									pop_double_n_push(0, (NODETYPE)(t_EE));
											// printf("+");
										}
		|	LOGICAL_NOT expr	{ 				$$ = !(lli)$2;
									pop_single_n_push('!', (NODETYPE)(t_OP));
			}
		|	expr LOGICAL_AND expr	{ 			$$ = (lli)$1 & (lli)$3;
									pop_double_n_push('&', (NODETYPE)(t_OP));
										// printf("+");
									}
		|	expr LOGICAL_OR expr	{ 		$$ = (lli)$1 | (lli)$3;
									pop_double_n_push('|', (NODETYPE)(t_OP));
											// printf("+");
										}
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

void print_stack() {
	Node* node;
	while((node = (Node*)pop_stack(stack)) != NULL && (lli)node != LLONG_MIN) {
		/* printf("%p\n", node); */
		printTree(node);
		printf("-----------------------------------------\n");
	}
}


void pop_double_n_push(lli val,NODETYPE n_type) {
	Node* node = init_node(val, (NODETYPE)(n_type));
	Node* right_child = (Node*)pop_stack(stack); 
	/* printf("double %d %c %p\n", n_type, (char)val, right_child); */
	if (right_child == NULL) {
		fprintf(stderr,"No right child\n");
		return;
	}
	Node* left_child = (Node*)pop_stack(stack);
	if (left_child == NULL) {
		fprintf(stderr,"No left child\n");
		return;
	}
	add_child(node, left_child);
	add_child(node, right_child);
	push_stack(stack, (lli)node);
}

void pop_all_n_push(lli val,NODETYPE n_type) {
	Node* node = init_node(val, (NODETYPE)(n_type));
	Node* child;
	while ((child = (Node*)pop_stack(stack)) != NULL  && (lli)child != LLONG_MIN) {
		add_child(node, child);
	}
	push_stack(stack, (lli)node);
}

void pop_single_n_push(lli val,NODETYPE n_type) {
	Node* node = init_node(val, (NODETYPE)(n_type));
	Node* right_child = (Node*)pop_stack(stack); 
	/* printf("single %d\n", n_type); */
	if (right_child == NULL) {
		fprintf(stderr,"No right child");
		return;
	}
	add_child(node, right_child);
	push_stack(stack, (lli)node);
}

void yyerror ( const char  *s) {
	/* fprintf(stderr, "%s: %s", progname, s); */
	/* fprintf(stderr, " %s %d\n", yylval.name, yylval.num); */
	fprintf (stderr, "%s", s);
	fprintf(stderr, " near line %d\n", lineno);
	exit(0);
 }


