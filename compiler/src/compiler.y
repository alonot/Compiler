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
	void printTreeIdent(Node* node);
	int yylex();
	int i;	
	short inside_loop = 0;
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
%token BREAK CONTINUE 
%token MAIN RETURN

%union {
	char* name; // variable name
	int num; // integer value
	double final_val; // double answer
	struct com_val* pointer; // to store variables pointer
	struct __tree* node; // bst tree
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

%type <node> expr var_expr str_expr id
param_list param_list1 func_body func_call func_name func_ret_type for_assign_stmt for_expr_eval
func_stmt statement stmt_list assign_stmt write_stmt read_stmt cond_stmt list decl 
decl_list data_type Gdecl_sec prog_block prog_block_stmt_list Fdef_sec MainBlock expr_eval
param_list_with_str param_list1_with_str

%%

	Prog :	Gdecl_sec prog_block {
		if ($1 != NULL) {
			add_neighbour($1, $2);
			printTreeIdent($1);
			run($1);
			printSymbolTables(symbol_tables);
			free_tree($1);
		} else {
			printTreeIdent($2);
			free_tree($2);
		}
	}
		;

	prog_block : prog_block_stmt_list { 
				Node* node = init_node((lli)"prog_block", t_OTHER, NULL);
				add_all_children(node, $1, NULL);
				$$ = node;
			}
			| Fdef_sec MainBlock { 
				Node* node = init_node((lli)"prog_block", t_OTHER, NULL);
				add_all_children(node, $1, NULL);
				add_all_children(node, $2, NULL);
				$$ = node;
			}
           ;

	prog_block_stmt_list : /*Nothing*/ { $$ = NULL; } 
                    |  stmt_list {
						Node* node = init_node((lli)"prog_block_stmt_list", t_OTHER, NULL);
						add_all_children(node, $1, NULL);
						$$ = node;
					}
                    | BEG stmt_list END {
						Node* node = init_node((lli)"prog_block_stmt_list", t_OTHER, NULL);
						add_all_children(node, $2, NULL);
						$$ = node;
					}
                    ;
	
	func_body : BEG stmt_list ret_stmt END { $$ = NULL; }
		;

	Gdecl_sec:	DECL decl_list  ENDDECL {
			Node* node = init_node((lli)"Gdecl", t_OTHER, NULL);
			add_all_children(node, $2, NULL);
			$$ = node;
	} 
		;
		
	decl_list:  { $$ = NULL; }
		| 	decl decl_list {
			add_neighbour($1, $2);
			$$ = $1;
		}
		;
		
	decl 	:	data_type list ';' { 
		add_all_children($1, $2, NULL);
		$$ = $1;
	}
		;

	data_type : 	T_INT		{ 
			$$ = init_node((lli)("INTEGER"),(NODETYPE)(t_KEYWORD), NULL);
			curr_dtype = INT; /*for integer */
		}
		| T_BOOL { curr_dtype = BOOL; /*for boolean*/ $$ = init_node((lli)("BOOL"),(NODETYPE)(t_KEYWORD), NULL);}
		| T_DOUBLE { curr_dtype = DOUBLE; /*for double*/ $$ = init_node((lli)("DOUBLE"),(NODETYPE)(t_KEYWORD), NULL);}
		;
	

		
	list 	:	id { 
			$$ = $1;
			}
		|	id ',' list {
			add_neighbour($1, $3);
			$$ = $1;
		}
		;
	
	id	:	VAR		{ 		
						STEntry* ste = create_stentry((STETYPE)(curr_dtype), $1);
						upsert_to(symbol_tables, 
						(lli)$1, 
						(lli)ste);
						$$ = init_node((lli)(ste), t_STE, NULL);
					}
		|	id '[' NUM ']'	{
			STEntry* ste = (STEntry*)$1 -> val;
			if ($3 == 0) {
				yyerror("Array length cannot be 0\n");
			} else {
				add_array_layer(ste, $3);
			}
			Node* node = init_node((lli)($3), (NODETYPE)(t_ARRAY_SIZE), NULL);
			add_child($1 , node);
			$$ = $1;
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
		
	Fdef_sec:	 {}
		|	Fdef_sec Fdef {}
		;
		
	Fdef	:	func_ret_type func_name {add_local_symbol_table(symbol_tables);} '(' FargList ')' '{' Ldecl_sec func_body '}'	{	remove_local_symbol_table(symbol_tables); 				}
		;
		
	func_ret_type:	data_type {}
		;
			
	func_name:	VAR		{ 					}
		;
		
	FargList:	arg_list	{ 					}
		;

	ret_stmt:	RETURN expr_eval ';'	{ 					}
		;
			
	MainBlock: 	func_ret_type main {add_local_symbol_table(symbol_tables);} '('')''{' Ldecl_sec func_body  '}'		{ 	remove_local_symbol_table(symbol_tables);}
					  
		;
		
	main	:	MAIN		{ 					}
		;
		
	Ldecl_sec:	DECL {} decl_list ENDDECL
		;

	stmt_list:	statement		{ $$ = $1; }
		|	statement stmt_list	{		add_neighbour($1, $2);$$ = $1;				}
		|	error ';' 		{ }
		;

	statement:	assign_stmt  ';'		{ 			$$ = $1;				 }
		|	read_stmt ';'		{ $$ = $1; }
		|	write_stmt ';'		{  $$ = $1; }
		|	cond_stmt 		{ $$ = $1; }
		|	func_stmt ';'		{ $$ = $1; }
		|	BREAK ';'		{ 
			if (!inside_loop) {
				yyerror("break can only be used inside a loop block\n");
			}
			$$ = init_node(0, t_BREAK, NULL); 
		}
		|	CONTINUE ';'		{ 
			if (!inside_loop) {
				yyerror("continue can only be used inside a loop block\n");
			}
			$$ = init_node(0, t_CONTINUE, NULL); 
		}
		;

	read_stmt:	READ '(' var_expr ')'  { 
			Node * node = init_node((lli)"READ", (NODETYPE)(t_FUNC), NULL);
			add_all_children(node, $3, NULL);
			$$ = node; 
		}
		;

	write_stmt:	WRITE  '(' param_list_with_str ')' 	{
			Node * node = init_node((lli)"WRITE", (NODETYPE)(t_FUNC), NULL);
			add_all_children(node, $3, NULL);
			$$ = node;
			}
		;
	
	assign_stmt:	var_expr '=' expr_eval 	{ 		
			Node* node = $1;
			if (node->n_type != t_STE) {
				yyerror("How is this possible? var expression must return node_type t_STE");
			}
			STEntry* ste = (STEntry*)node ->val;
			if (ste->is_array != 0) { // array
				DARRAY* arrval = ste->value.arrval;
				if (arrval->arr_curr_depth < arrval -> arr_depth) {
					printf("Array %s should be derefrenced %d times only.\n", ste -> name, arrval -> arr_depth);
					yyerror("");
				}
			}
			node = init_node(0, t_ASSIGN, NULL);
			add_child(node,$1);
			add_child(node , $3);
			$$ = node;
		}
		;

	cond_stmt:	IF '(' expr_eval ')' THEN stmt_list ENDIF 	{ 		
						Node*  node = init_node(0, t_COND, NULL);
						add_child(node, $3);
						Node*  if_node = init_node(0, t_IF_BLOCK, NULL);
						add_all_children(if_node, $6, NULL);
						add_child(node, if_node);
						$$ = node;
					}
		|  IF '(' expr_eval ')' THEN stmt_list ELSE stmt_list ENDIF 	{ 		
						Node*  node = init_node(0, t_COND, NULL);
						add_child(node, $3);
						Node*  if_node = init_node(0, t_IF_BLOCK, NULL);
						add_all_children(if_node, $6, NULL);
						add_child(node, if_node);
						Node*  else_node = init_node(0, t_ELSE_BLOCK, NULL);
						add_all_children(else_node, $8, NULL);
						add_child(node, else_node);
						$$ = node;
						}
	    |  FOR '(' for_assign_stmt  ';'  for_expr_eval ';'  for_assign_stmt ')' '{' {inside_loop ++;} stmt_list '}' {
			Node* node = init_node(0, t_FOR, NULL);
			add_child(node, $3);
			add_child(node, $5);
			add_child(node, $7);
			Node*  loop_node = init_node(0, t_LOOP_BLOCK, NULL);
			add_all_children(loop_node, $11, NULL);
			add_child(node, loop_node);
			$$ = node;
			inside_loop --;
		}
		|  WHILE '(' expr_eval ')' DO {inside_loop ++;} stmt_list ENDWHILE {
			Node* node = init_node(0, t_WHILE, NULL);
			add_child(node, $3);
			Node*  loop_node = init_node(0, t_LOOP_BLOCK, NULL);
			add_all_children(loop_node, $7, NULL);
			add_child(node, loop_node);
			inside_loop --;
			$$ = node;
		}
		;

	for_assign_stmt: { $$ = init_node(0, t_NOP, NULL); }
		| assign_stmt { $$ = $1; }
		;
	for_expr_eval: { $$ = init_node(0, t_NOP, NULL); }
		| expr_eval { $$ = $1; }
		;
	
	func_stmt:	func_call 		{ 		$$ = $1;			}
		;
		
	func_call:	VAR '(' param_list ')'	{ 		
			Node * node = init_node((lli)$1, (NODETYPE)(t_FUNC), free_string);
			add_all_children(node, $3, NULL);
			$$ = node;				   }
		;
	
	param_list_with_str: {$$ = NULL; }
		| param_list1_with_str {$$ = $1;}
		;

	param_list1_with_str: expr_eval { 
			$$ = $1;
		}
		| expr_eval ',' param_list1_with_str {
			add_neighbour($1, $3);
			$$ = $1;
		}	
		| '"' str_expr '"' {
			$$ = $2;
		}
		| '"' str_expr '"' ',' param_list1_with_str {
			add_neighbour($2, $5);
			$$ = $2;
		}
		;

	param_list:				{ $$ = NULL; }
		|	param_list1	{ $$ = $1; }
		;
		
	param_list1:	expr_eval { 
			$$ = $1;
		}
		|	expr_eval ',' param_list1 {
			add_neighbour($1, $3);
			$$ = $1;
		}	
		;

	expr_eval: expr {
		Node * node = init_node(0, (NODETYPE)(t_EXPR), NULL);
		add_child(node , $1);
		$$ =node;
	}
		; 

	expr	:	NUM 			{
								$$ = init_node($1, (NODETYPE)(t_NUM), NULL);
								}
		|	'-' NUM			{  
								$$ = init_node(-1 * $2, (NODETYPE)(t_NUM), NULL);
							}
		|	var_expr		{			
								Node* node = $1;
								if (node->n_type != t_STE) {
									yyerror("How is this possible? var expression must return node_type t_STE");
								}
								STEntry* ste = (STEntry*)node ->val;
								if (ste->is_array != 0) { // array
									DARRAY* arrval = ste->value.arrval;
									if (arrval->arr_curr_depth < arrval -> arr_depth) {
										printf("Array %s should be derefrenced %d times only.\n", ste -> name, arrval -> arr_depth);
										yyerror("");
									}
								}
								$$ = $1;
							}
		|	T			{	  
							$$ = init_node(1 , (NODETYPE)(t_BOOLEAN), NULL);
			}
		|	F			{   
							$$ = init_node(0 , (NODETYPE)(t_BOOLEAN), NULL);
						}
		|	'(' expr ')'		{  	$$ = $2;	}

		|	expr '+' expr 		{ 	
									Node* node = init_node('+', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
								}
		|	expr '-' expr	 	{
									Node* node = init_node('-', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
								}
		|	expr '*' expr 		{ 	Node* node = init_node('*', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
								}
		|	expr '/' expr 		{ 	Node* node = init_node('/', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
								}
		|	expr '%' expr 		{ 	Node* node = init_node('%', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
								}
		|	expr '<' expr		{ 	Node* node = init_node('<', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
								}
		|	expr '>' expr		{ 	Node* node = init_node('>', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
								}
		|	expr GREATERTHANOREQUAL expr				{ 
									Node* node = init_node(0, t_GTE, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
								}
		|	expr LESSTHANOREQUAL expr	{ Node* node = init_node(0, t_LTE, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
									}
		|	expr NOTEQUAL expr			{ 		Node* node = init_node(0, t_NE, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
									}
		|	expr EQUALEQUAL expr	{ 	Node* node = init_node(0, t_EE, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
										}
		|	LOGICAL_NOT expr	{ 	Node* node = init_node('!', t_OP, NULL);
									add_child(node,$2);
									$$ = node;
			}
		|	expr LOGICAL_AND expr	{ 	Node* node = init_node('&', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									$$ = node;
									}
		|	expr LOGICAL_OR expr	{ 	
										Node* node = init_node('|', t_OP, NULL);
										add_child(node,$1);
										add_child(node,$3);
										$$ = node;
										}
		|	func_call		{  }

		;
	str_expr :  VAR                       { 
						$$ = init_node((lli)(0), t_STR, NULL); 
						Node* node = init_node((lli)($1), t_STR, free_string); 
						add_child($$, node);
					}
                  | str_expr VAR   { 
					Node* node = init_node((lli)$2, t_STR, free_string);
					add_child($1, node);
					$$ = $1;
				}
                ;
	
	var_expr:	VAR	{
					STEntry* ste = (STEntry*)value_Of(symbol_tables, (lli)$1);
					if (ste == NULL || (lli)ste == LLONG_MIN) {
						printf("Variable not declared : %s\n", $1);
						yyerror("");
					} else {
						if (ste -> is_array) {
							ste -> value.arrval -> arr_curr_depth = 0;
							ste -> value.arrval -> arr_curr_pos = 0;
						}
						$$ = init_node((lli)ste, t_STE, NULL);
					}
					free($1);
			  }
		|	var_expr '[' expr_eval ']'	{ 
			Node* node = $1;
			if (node -> n_type != t_STE) {
				yyerror("How is this possible? var expression must return node_type t_STE");
			}
			STEntry* ste = (STEntry*)node ->val;
			if (ste -> is_array == 0) {
				printf("%s is not array \n", ste -> name);
				yyerror("");
			}
			DARRAY* val = ste -> value.arrval;
			if (val -> arr_curr_depth >= val->arr_depth) {
				printf("Array %s should be derefrenced %d times only.\n", ste -> name, val -> arr_depth);
				yyerror("");
			}
			val -> arr_curr_depth ++;
			add_child_in_front($1, $3);
			$$ = $1;	
		}
		;
%%

extern int	lineno;

void yyerror ( const char  *s) {
	fprintf (stderr, "%s", s);
	fprintf(stderr, " near line %d\n", lineno);
	exit(0);
 }


