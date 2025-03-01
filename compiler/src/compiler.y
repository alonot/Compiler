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
	void printTreeIdent(Node* node);
	void pop_single_n_push(lli val,NODETYPE n_type,  void (*free)(lli));
	void pop_double_n_push(lli val,NODETYPE n_type,  void (*free)(lli));
	void pop_all_n_push(lli val,NODETYPE n_type,  void (*free)(lli));
	int yylex();
	void yyerror( const char* );
	int i;	
	short write = 0;
	short curr_dtype = 0;

	void free_string(lli val) {
		free((char*) (val));
	}
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
	struct com_val* pointer; // to store variables pointer
}
%token <name> VAR 
%token <num> NUM
%token T_INT T_BOOL T_DOUBLE

%left '<' '>'
%left EQUALEQUAL LESSTHANOREQUAL GREATERTHANOREQUAL NOTEQUAL
%left '+' '-'
%left '*' '/'
%left '%'
%left LOGICAL_AND LOGICAL_OR
%left LOGICAL_NOT

%type <final_val> expr
%type <pointer> var_expr
%type <final_val> data_type
%type <name> str_expr
%type <name> id
%%

	Prog :	Gdecl_sec prog_block {
		pop_all_n_push((lli)("Prog"),(NODETYPE)(t_OTHER), NULL);
		print_stack();
	}
		;

	prog_block : prog_block_stmt_list
           | Fdef_sec MainBlock { }
           ;

	prog_block_stmt_list : /*Nothing*/ {} 
                    | {push_stack(stack, (lli)NULL);} stmt_list {
						pop_all_n_push((lli)("Statements"),(NODETYPE)(t_OTHER), NULL);
					}
                    | BEG {
						push_stack(stack, (lli)NULL);
						Node* node = init_node((lli)"BEG", (NODETYPE)(t_KEYWORD), NULL);
						push_stack(stack, (lli)node);
						push_stack(stack, (lli)NULL);
					} stmt_list END {
						pop_all_n_push((lli)("Statements"),(NODETYPE)(t_OTHER), NULL);
						Node* node = init_node((lli)"END", (NODETYPE)(t_KEYWORD), NULL);
						push_stack(stack, (lli)node);
						pop_all_n_push((lli)("ProgBlock"),(NODETYPE)(t_OTHER), NULL);
					}
                    ;
	
	func_body : BEG stmt_list ret_stmt END
		;

	data_type : 	T_INT		{ curr_dtype = INT; /*for integer */}
		| T_BOOL { curr_dtype = BOOL; /*for boolean*/}
		| T_DOUBLE { curr_dtype = DOUBLE; /*for double*/}
		;

	Gdecl_sec:	DECL {
			push_stack(stack, (lli)NULL);
			Node* node = init_node((lli)"DECL", (NODETYPE)(t_KEYWORD), NULL);
			push_stack(stack, (lli)node);
		} decl_list  ENDDECL {
			Node* node = init_node((lli)"ENDDECL", (NODETYPE)(t_KEYWORD), NULL);
			push_stack(stack, (lli)node);
			pop_all_n_push((lli)("Gdecl_sec"),(NODETYPE)(t_OTHER), NULL);
			print_stack();
	} 
		;
		
	decl_list: 
		| 	decl decl_list
		;
		
	decl 	:	data_type {push_stack(stack, (lli)NULL);} list ';' { 
		pop_all_n_push((lli)($1 == 1 ? "Integer": "Bool"),(NODETYPE)(t_KEYWORD), NULL);
	}
		;
	

		
	list 	:	id { 
		pop_all_n_push((lli)$1, (NODETYPE)(t_IDENTIFIER), free_string);
			}
		|	id {
			pop_all_n_push((lli)$1, (NODETYPE)(t_IDENTIFIER), free_string);
		} ',' list 
		;
	
	id	:	VAR		{ 		
						upsert_to(symbol_tables, (lli)$1, (lli)create_stentry((STETYPE)(curr_dtype)));
						$$ = $1;
						push_stack(stack, (lli)NULL); 

					}
		|	id '[' NUM ']'	{
			$$ = $1;
			STEntry* ste = (STEntry*)value_Of(symbol_tables, (lli)$1);
			if (ste == NULL || (lli)ste == LLONG_MIN) {
				printf("Variable not declared : %s\n", $1);
				yyerror("");
			} else if ($3 == 0) {
				yyerror("Array length cannot be 0\n");
			} else {
				add_array_layer(ste, $3);
			}
			Node* node = init_node((lli)($3), (NODETYPE)(t_ARRAY_SIZE), NULL);
			push_stack(stack, (lli)node);
		   }
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
			Node* node = init_node( (lli)$5, (NODETYPE)(t_STR), free_string);
			push_stack(stack, (lli)node);
			pop_single_n_push((lli)"Write", (NODETYPE)(t_KEYWORD), NULL);
		}
		| WRITE  '(' { 
				write = 1;
				push_stack(stack, (lli)NULL);
			} param_list ')' 	{
				write= 0;
				pop_all_n_push((lli)"Write",(NODETYPE)(t_KEYWORD), NULL);
			}
		;
	
	assign_stmt:	var_expr '=' expr 	{ 		
			COMVAL* val = $1;
			if (val -> array_depth < val -> array_depth_required) {
				printf("Array %s should be refrenced %d times.\n", val -> name, val -> array_depth_required);
				yyerror("");
			}
			Node* node;
			switch(val -> dtype) {
				case DOUBLE:
				node = init_node(*(double*) (val -> value), (NODETYPE)(t_VAR), NULL);
				*(double*) (val -> value) = (double) $3;
				break;
				case BOOL:
				node = init_node((*(short*) (val -> value)) != 0, (NODETYPE)(t_VAR), NULL);
				*(short*) (val -> value) = (short) ($3 != 0);
				break;
				case INT:
				node = init_node(* (lli*) (val -> value), (NODETYPE)(t_VAR), NULL);
				*(lli*) (val -> value) = (lli) $3;
				break;
				default:
				yyerror("");
			}
			free(val -> name);
			free(val);
			push_stack(stack, (lli)node);
			pop_double_n_push(0, (NODETYPE)(t_ASSIGN), NULL);
			
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
							Node* node = init_node($1, (NODETYPE)(t_NUM), NULL);
							push_stack(stack, (lli)node);
								//  printf("%d\n",$1 );	
								}
		|	'-' NUM			{  			$$ = -1 * $2;
							pop_single_n_push(0, (NODETYPE)(t_MINUS), NULL);
								//  printf("-%d\n",$2 );			   
							}
		|	var_expr		{
								COMVAL* val = $1;
								if (val -> array_depth < val -> array_depth_required) {
									printf("Array %s should be refrenced %d times.\n", val -> name, val -> array_depth_required);
									yyerror("");
								}
								switch(val -> dtype) {
									case DOUBLE:
									$$ = * (double*) (val -> value);
									break;
									case BOOL:
									$$ = (*(short*) (val -> value)) != 0;
									break;
									case INT:
									$$ = * (lli*) (val -> value);
									break;
									default:
									yyerror("");
								}
								free(val -> name);
								free(val);
								Node* node = init_node($$, (NODETYPE)(t_VAR), NULL);
								push_stack(stack, (lli)node);
							}
		|	T			{	$$ = 1;			  
							Node* node = init_node(1, (NODETYPE)(t_BOOLEAN), NULL);
							push_stack(stack, (lli)node);
			}
		|	F			{   $$ = 0;	
							Node* node = init_node(0, (NODETYPE)(t_BOOLEAN), NULL);
							push_stack(stack, (lli)node);
						}
		|	'(' expr ')'		{  	$$ = $2;	}

		|	expr '+' expr 		{ 		$$ = $1 + $3;
									pop_double_n_push('+', (NODETYPE)(t_OP), NULL);
									// printf("+");		
								}
		|	expr '-' expr	 	{ 			$$ = $1 - $3;
									pop_double_n_push('-', (NODETYPE)(t_OP), NULL);
										// printf("-");
									}
		|	expr '*' expr 		{ 	$$ = $1 * $3;
									pop_double_n_push('*', (NODETYPE)(t_OP), NULL);
									// printf("*");
								}
		|	expr '/' expr 		{ 			$$ = $1 / $3;
									pop_double_n_push('/', (NODETYPE)(t_OP), NULL);
									// printf("DIV ");
								}
		|	expr '%' expr 		{ 		$$ = (lli)$1 % (lli)$3;
									pop_double_n_push('%', (NODETYPE)(t_OP), NULL);
									// printf(" MOD ");
								}
		|	expr '<' expr		{ 			$$ = $1 < $3;
									pop_double_n_push('<', (NODETYPE)(t_OP), NULL);
									// printf("+");
								}
		|	expr '>' expr		{ 			$$ = $1 > $3;
									pop_double_n_push('>', (NODETYPE)(t_OP), NULL);
									// printf("+");
								}
		|	expr GREATERTHANOREQUAL expr				{ $$ = $1 >= $3;
									pop_double_n_push(0, (NODETYPE)(t_GTE), NULL);
									// printf("+");
								}
		|	expr LESSTHANOREQUAL expr	{  			$$ = $1 <= $3;
										// printf("+");
									pop_double_n_push(0, (NODETYPE)(t_LTE), NULL);
									}
		|	expr NOTEQUAL expr			{ 			$$ = $1 != $3;
										// printf("+");
									pop_double_n_push(0, (NODETYPE)(t_NE), NULL);
									}
		|	expr EQUALEQUAL expr	{ 		$$ = $1 == $3;
									pop_double_n_push(0, (NODETYPE)(t_EE), NULL);
											// printf("+");
										}
		|	LOGICAL_NOT expr	{ 				$$ = !(lli)$2;
									pop_single_n_push('!', (NODETYPE)(t_OP), NULL);
			}
		|	expr LOGICAL_AND expr	{ 			$$ = (lli)$1 & (lli)$3;
									pop_double_n_push('&', (NODETYPE)(t_OP), NULL);
										// printf("+");
									}
		|	expr LOGICAL_OR expr	{ 		$$ = (lli)$1 | (lli)$3;
									pop_double_n_push('|', (NODETYPE)(t_OP), NULL);
											// printf("+");
										}
		|	func_call		{  }

		;
	str_expr :  VAR                       { $$ = $1; }
                  | str_expr VAR   { $$ = strcat($1, $2);}
                ;
	
	var_expr:	VAR	{ 		
					STEntry* ste = (STEntry*)value_Of(symbol_tables, (lli)$1);
					if (ste == NULL || (lli)ste == LLONG_MIN) {
						printf("Variable not declared : %s\n", $1);
						yyerror("");
					} else {
						COMVAL* val = (COMVAL*) (calloc(1, sizeof(COMVAL)));
						val->dtype = ste->dtype;
						val->array_max_pos = ste->total_array_length;
						val->array_depth_required = ste->array_depth;
						val -> array_lengths = ste -> array_length;
						val -> array_depth = -1;
						val -> name = $1;
						switch (ste->dtype) {
							case DOUBLE:
								val->value = (lli*)get_double(ste, -1);
							break;
							case INT:
								val->value = (lli*)get_int(ste, -1);
							break;
							case BOOL:
								val->value = (lli*)get_bool(ste, -1);
							break;
							default:
							yyerror("");
						}
						val -> array_depth = 0;
						$$ = val;
					}
			  }
		|	var_expr '[' expr ']'	{ 
			COMVAL* val = $1 ;
			if (val -> array_depth >= val->array_depth_required) {
				if (val -> array_depth_required == 0) {
					printf("%s is not array \n", val -> name);
				} else {
					printf("Array %s should be refrenced %d times.\n", val -> name, val -> array_depth_required);
				}
				yyerror("");
			} else {
				val -> array_pos += val->array_lengths[val -> array_depth ++] * (int)$3;
				if (val -> array_pos >= val -> array_max_pos) {
					printf("Index %d out of bounds (%d) for Array %s \n", (int)$3, val->array_lengths[val -> array_depth - 1], val -> name);
					yyerror("");
				}
			}
			$$ = val;
			
			
		}
		;
%%

extern int	lineno;

void print_stack() {
	Node* node;
	while((node = (Node*)pop_stack(stack)) != NULL && (lli)node != LLONG_MIN) {
		/* printf("%p\n", node); */
		/* printTree(node); */
		printTreeIdent(node);
		free_tree(node);
		printf("-----------------------------------------\n");
	}
}


void pop_double_n_push(lli val,NODETYPE n_type,  void (*free)(lli)) {
	Node* node = init_node(val, (NODETYPE)(n_type), free);
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

void pop_all_n_push(lli val,NODETYPE n_type,  void (*free)(lli)) {
	Node* node = init_node(val, (NODETYPE)(n_type), free);
	Node* child;
	Node* prev = NULL;
	Node* head = NULL;
	while ((child = (Node*)pop_stack(stack)) != NULL  && (lli)child != LLONG_MIN) {
		/* add_child(node, child); */
		child -> next = prev;
		if (head == NULL) {
			head = child;	
		} 
		prev = child;
	}
	if (head != NULL  && (lli)head != LLONG_MIN ) {
		add_all_children(node, prev, head);
	}
	push_stack(stack, (lli)node);
}

void pop_single_n_push(lli val,NODETYPE n_type,  void (*free)(lli)) {
	Node* node = init_node(val, (NODETYPE)(n_type), free);
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


