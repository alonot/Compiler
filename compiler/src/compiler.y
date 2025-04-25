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
	extern SymbolTable* current_symbol_table;
	extern HashMap* labels;
	extern String* label_data;
	extern int interpret;
	extern Node* prog_root;

	void check_validity_ste(Node* node);
	Node* create_global_node(Node* body);
	void printTreeIdent(Node* node);
	int yylex();
	int i;	
	short inside_loop = 0;
	short curr_dtype = 0;
	VARTYPE var_type = LOCAL;

	void free_string(lli val) {
		free((char*) (val));
	}
%}

%define parse.error verbose

%token BEG END
%token DECL ENDDECL
%token PLUSPLUS MINUSMINUS
%token IF THEN ELSE ENDIF
%token LOGICAL_AND LOGICAL_NOT LOGICAL_OR
%token EQUALEQUAL LESSTHANOREQUAL GREATERTHANOREQUAL NOTEQUAL
%token WHILE DO ENDWHILE FOR 
%token T F
%token BREAK CONTINUE 
%token RETURN

%union {
	char* name; // variable name
	double num; // integer value
	double final_val; // double answer
	struct com_val* pointer; // to store variables pointer
	struct __tree* node; // bst tree
	struct _string* str; // str tree
}

%token <name> VAR 
%token <str> STR 
%token <num> NUM
%token <num> NUM_FLOAT
%token T_INT T_BOOL T_DOUBLE 

%left EQUALEQUAL LESSTHANOREQUAL GREATERTHANOREQUAL NOTEQUAL
%left '+' '-'
%left '*' '/'
%left '%'
%left LOGICAL_AND LOGICAL_OR
%left LOGICAL_NOT
%left '<' '>'

%type <name> func_name

%type <str> str_expr

%type <node> expr var_expr id increament_expr increament_stmt
func_body func_call for_assign_stmt for_expr_eval
func_stmt statement stmt_list stmt_list1  assign_stmt cond_stmt list decl 
decl_list data_type Gdecl_sec prog_block prog_block_stmt_list Fdef_sec expr_eval
param_list_with_str param_list1_with_str arg_list arg_list1 arg ret_stmt Fdef Ldecl_sec

%%

	Prog :	Gdecl_sec prog_block {
		if ($1 != NULL) {
			add_neighbour($1, $2);
			prog_root = $1;
		} else {
			prog_root = $2;
		}
	}
		;

	prog_block : Fdef_sec prog_block_stmt_list Fdef_sec {
				Node* node = init_node((lli)NULL, t_PROG, NULL);
				add_all_children(node, $2, NULL);
				add_all_children(node, $1, NULL);
				add_all_children(node, $3, NULL);
				$$ = node;
			}
           ;

	prog_block_stmt_list :  stmt_list {
			$$ = create_global_node($1);
		}
		| BEG stmt_list END {
			$$ = create_global_node($2);
		}
		;

	Gdecl_sec:	DECL decl_list  ENDDECL {
			Node* node = init_node((lli)"Gdecl", t_DECL, NULL);
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
			STEntry* ste = (STEntry*)find_local(current_symbol_table, (lli)$1);
			if (ste != NULL && (lli)ste != LLONG_MIN) {
				fprintf(stderr,"Variable already declared : %s\n", $1);
				yyerror("");
			}
			ste = create_stentry((STETYPE)(curr_dtype), $1, (VARTYPE)(var_type));
			upsert_to(current_symbol_table, 
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
		
			
	arg_list:	{ 
		Node * node = init_node((lli)NULL, (NODETYPE)(t_ARG_LIST), NULL);
		$$ = node;
	 }
		| {
			var_type = (VARTYPE)(ARG);
		}	arg_list1 { 
			
			Node * node = init_node((lli)NULL, (NODETYPE)(t_ARG_LIST), NULL);
			add_all_children(node, $2, NULL);
			$$ = node; 
		}
		;
		
	arg_list1:	arg_list1 ',' arg { 
		add_neighbour($1, $3);
		$$ = $1;
	}
		|	arg {
			$$ = $1;
		}
		;
		
	arg 	:	data_type list	{
		add_all_children($1, $2, NULL);
		$$ = $1;
	}
		;
		
	Fdef_sec:	 {
		$$ = NULL;
	}
		|	Fdef_sec Fdef {
				add_neighbour($2, $1);
				$$ = $2;
		}
		;
		
	Fdef	:	data_type func_name {
			lli label;
			if ((label = get(labels, (lli)$2)) == LONG_MIN || (char*)label == NULL) {
				insert(labels, (lli)$2, (lli)$2);
			}
			current_symbol_table = add_sym_tables(current_symbol_table, $2, (void (*)(char *))free);
		} '(' arg_list ')' '{' Ldecl_sec func_body '}'	{	
			Node * node = init_node((lli)current_symbol_table, (NODETYPE)(t_FUNC_DEF), (void (*)(lli))free_symbol_table);
			// add return type
			Node * return_node = init_node((lli)NULL, (NODETYPE)(t_FUNC_RET), NULL);
			add_child(return_node, $1);

			add_all_children(node, return_node, NULL);
			// arg list
			add_all_children(node, $5, NULL);
			// decl
			add_all_children(node, $8, NULL);
			// body
			add_all_children(node, $9, NULL);

			$$ = node;
			current_symbol_table = parent_sym_table(current_symbol_table);	
		}
		;

	
	func_body : BEG stmt_list END { 
			Node* node = init_node((lli)NULL, t_FUNC_BODY, NULL);
			add_all_children(node, $2, NULL);
			$$ = node;
	 }
		;
			
	func_name:	VAR		{ 		$$ = $1;			}
		;

	ret_stmt:	RETURN expr_eval ';'	{ 		
					Node * node = init_node((lli)NULL, (NODETYPE)(t_RETURN), NULL);
					add_all_children(node, $2, NULL);
					$$ = node; 
				}
		;
		
	Ldecl_sec:	DECL {
		var_type = (VARTYPE)(LOCAL);
	} decl_list ENDDECL {
			Node* node = init_node((lli)"Local Dec", t_DECL, NULL);
			add_all_children(node, $3, NULL);
			$$ = node;
	}
		;

	stmt_list :  { $$ = NULL; }
		| stmt_list1 { $$ = $1; }
		;

	stmt_list1:	statement		{ $$ = $1; }
		|	statement stmt_list1	{		add_neighbour($1, $2);$$ = $1;				}
		|	error ';' 		{ }
		;

	statement:	assign_stmt  ';'		{ 			$$ = $1;				 }
		/* |	read_stmt ';'		{ $$ = $1; }
		|	write_stmt ';'		{  $$ = $1; } */
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
		| increament_stmt ';' { $$ = $1; }
		| ret_stmt { $$ = $1; }
		;

	/* read_stmt:	READ '(' var_expr ')'  { 
			Node * node = init_node((lli)"READ", (NODETYPE)(t_FUNC_CALL), NULL);
			add_all_children(node, $3, NULL);
			$$ = node; 
		}
		;

	write_stmt:	WRITE  '(' param_list_with_str ')' 	{
			Node * node = init_node((lli)"WRITE", (NODETYPE)(t_FUNC_CALL), NULL);
			add_all_children(node, $3, NULL);
			$$ = node;
			}
		; */
	
	assign_stmt:	var_expr '=' expr_eval 	{ 		
			Node* node = $1;
			if (node->n_type != t_STE) {
				yyerror("How is this possible? var expression must return node_type t_STE");
			}
			STEntry* ste = (STEntry*)node ->val;
			if (ste->is_array != 0) { // array
				DARRAY* arrval = ste->value.arrval;
				if (arrval->arr_curr_depth < arrval -> arr_depth) {
					fprintf(yyout,"Array %s should be derefrenced %d times only.\n", ste -> name, arrval -> arr_depth);
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
		|  DO {inside_loop ++;} '{' stmt_list '}' WHILE '(' expr_eval ')' ';' {
			Node* node = init_node(0, t_DOWHILE, NULL);
			add_child(node, $8);
			Node*  loop_node = init_node(0, t_LOOP_BLOCK, NULL);
			add_all_children(loop_node, $4, NULL);
			add_child(node, loop_node);
			inside_loop --;
			$$ = node;
		}
		;

	for_assign_stmt: { $$ = init_node(0, t_NOP, NULL); }
		| assign_stmt { $$ = $1; }
		| increament_stmt { $$ = $1; }
		;
	for_expr_eval: { $$ = init_node(0, t_NOP, NULL); }
		| expr_eval { $$ = $1; }
		;
	
	func_stmt:	func_call 		{ 		$$ = $1;			}
		;
		
	func_call:	VAR '(' param_list_with_str ')'	{ 		
			Node * node = init_node((lli)$1, (NODETYPE)(t_FUNC_CALL), free_string);
			Node * param_node = init_node((lli)NULL, (NODETYPE)(t_PARAM_LIST), NULL);
			add_all_children(param_node, $3, NULL);
			add_child(node, param_node);
			$$ = node;	
		}
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
		| STR {
			Node* node = init_node((lli)$1, (NODETYPE)(t_STR), (void (*)(lli))(freeString));
			lli label;
			if ((label = get(labels, (lli)($1 -> val))) == LONG_MIN || (char*)label == NULL) {
				char* val = (char*)(calloc(6,sizeof(char)));
				val[0]= '$';
				val[1]= 'L';
				val[2]= 'C';
				snprintf(val + 3, 3, "%d", labels->len);
				insert(labels, (lli)($1 -> val), (lli)val);
				write_instr_val(label_data, 0,"%s:", val);
				write_instr_val(label_data, 1,"%-7s\t\"%s\\000\"", ".ascii", $1 -> val);
			}
			Node * expr_node = init_node(0, (NODETYPE)(t_EXPR), NULL);
			add_child(expr_node, node);
			expr_node -> depth = node -> depth + 1;
			$$ = expr_node;
		}
		| STR ',' param_list1_with_str {
			Node* node = init_node((lli)$1, (NODETYPE)(t_STR), (void (*)(lli))(freeString));
			lli label;
			if (((label = get(labels, (lli)($1 -> val) )) == LONG_MIN) || ((char*)label == NULL )) {
				char* val = (char*)(calloc(6,sizeof(char)));
				val[0]= '$';
				val[1]= 'L';
				val[2]= 'C';
				snprintf(val + 3, 3, "%d", labels->len);
				insert(labels, (lli)($1 -> val), (lli)val);
				write_instr_val(label_data, 0,"%s:", val);
				write_instr_val(label_data, 1,"%-7s\t\"%s\\000\"", ".ascii", $1 -> val);
			}
			Node * expr_node = init_node(0, (NODETYPE)(t_EXPR), NULL);
			add_child(expr_node, node);
			expr_node -> depth = node -> depth + 1;
			add_neighbour(expr_node, $3);
			$$ = node;
		}
		;
	increament_stmt : increament_expr {
		Node* node = init_node((lli)NULL, t_INC_STMT , NULL);
		add_child(node, $1);
		$$ = node;
	};

	increament_expr: var_expr PLUSPLUS	{			
								check_validity_ste($1);
								Node* node = init_node('+', t_PLUSPLUS_POST, NULL);
								node -> depth = $1 -> depth + 1;
								add_child(node,$1);
								$$ = node;
							}
		|	var_expr MINUSMINUS	{			
								check_validity_ste($1);
								Node* node = init_node('+', t_MINUSMINUS_POST, NULL);
								node -> depth = $1 -> depth + 1;
								add_child(node,$1);
								$$ = node;
							}
		|	PLUSPLUS var_expr	{			
								check_validity_ste($2);
								Node* node = init_node('+', t_PLUSPLUS_PRE, NULL);
								node -> depth = $2 -> depth + 1;
								add_child(node,$2);
								$$ = node;
							}
		|	MINUSMINUS var_expr	{			
								check_validity_ste($2);
								Node* node = init_node('+', t_MINUSMINUS_PRE, NULL);
								node -> depth = $2 -> depth + 1;
								add_child(node,$2);
								$$ = node;
							};

	expr_eval: expr {
		Node * node = init_node(0, (NODETYPE)(t_EXPR), NULL);
		add_child(node , $1);
		node -> depth = $1 -> depth + 1;
		$$ =node;
	}
		; 

	expr	:	NUM 			{
								// fprintf(yyout,"%lf\n",$1);
								double* val = (double*) (malloc(sizeof(double)));
								*val = $1;
								$$ = init_node((lli)val, (NODETYPE)(t_NUM_I), free_string);
								}
		| NUM_FLOAT 			{
								// fprintf(yyout,"%lf\n",$1);
								// to be stored as variable

								double* val = (double*) (malloc(sizeof(double)));
								*val = $1;
								$$ = init_node((lli)val, (NODETYPE)(t_NUM_F), free_string);
								}
		| '-' expr	{			
								Node* node = init_node((lli)NULL, t_MINUS, NULL);
								node -> depth = $2 -> depth + 1;
								add_child(node,$2);
								$$ = node;
							}
		|	var_expr		{			
								check_validity_ste($1);
								$$ = $1;
							}
		| increament_expr {
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
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
								}
		|	expr '-' expr	 	{
									Node* node = init_node('-', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
								}
		|	expr '*' expr 		{ 	Node* node = init_node('*', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
								}
		|	expr '/' expr 		{ 	Node* node = init_node('/', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
								}
		|	expr '%' expr 		{ 	Node* node = init_node('%', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
								}
		|	expr '<' expr		{ 	Node* node = init_node('<', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
								}
		|	expr '>' expr		{ 	Node* node = init_node('>', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
								}
		|	expr GREATERTHANOREQUAL expr				{ 
									Node* node = init_node(0, t_GTE, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
								}
		|	expr LESSTHANOREQUAL expr	{ Node* node = init_node(0, t_LTE, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
									}
		|	expr NOTEQUAL expr			{ 		Node* node = init_node(0, t_NE, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
									}
		|	expr EQUALEQUAL expr	{ 	Node* node = init_node(0, t_EE, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
										}
		|	LOGICAL_NOT expr	{ 	Node* node = init_node('!', t_OP, NULL);
									add_child(node,$2);
									node -> depth = ($2 -> depth);
									node -> depth ++;
									$$ = node;
			}
		|	expr LOGICAL_AND expr	{ 	
									Node* node = init_node('&', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
									}
		|	expr LOGICAL_OR expr	{ 	
									Node* node = init_node('|', t_OP, NULL);
									add_child(node,$1);
									add_child(node,$3);
									node -> depth = max(($1 -> depth), ($3 -> depth));
									node -> depth ++;
									$$ = node;
									}
		|	func_call		{  $$ = $1; }

		;
	str_expr :  VAR                       { 
		String* str = init_string($1, -1);
		$$ = str;
	}
	| str_expr VAR   { 
		add_str($1, " " , 1);
		add_str($1, $2 , -1);
		$$ = $1;
	};
	
	var_expr:	VAR	{
					STEntry* ste = (STEntry*)value_Of(current_symbol_table, (lli)$1);
					if (ste == NULL || (lli)ste == LLONG_MIN) {
						fprintf(yyout,"Variable not declared : %s\n", $1);
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
				fprintf(yyout,"%s is not array \n", ste -> name);
				yyerror("");
			}
			DARRAY* val = ste -> value.arrval;
			if (val -> arr_curr_depth >= val->arr_depth) {
				fprintf(yyout,"Array %s should be derefrenced %d times only.\n", ste -> name, val -> arr_depth);
				yyerror("");
			}
			val -> arr_curr_depth ++;
			add_child_in_front($1, $3);
			$1 -> depth = $3 -> depth + 1;
			$$ = $1;	
		}
		;
%%

void check_validity_ste(Node* node) {
	if (node->n_type != t_STE) {
		yyerror("How is this possible? var expression must return node_type t_STE");
	}
	STEntry* ste = (STEntry*)node ->val;
	if (ste->is_array != 0) { // array
		DARRAY* arrval = ste->value.arrval;
		if (arrval->arr_curr_depth < arrval -> arr_depth) {
			fprintf(yyout,"Array %s should be derefrenced %d times only.\n", ste -> name, arrval -> arr_depth);
			yyerror("");
		}
	}
}

Node* create_global_node(Node* body) {
	Node* node = init_node((lli)(lli)current_symbol_table, t_FUNC_DEF, (void (*)(lli))free_symbol_table);
						
	Node * return_node = init_node((lli)NULL, (NODETYPE)(t_FUNC_RET), NULL);
	Node * arg_node = init_node((lli)NULL, (NODETYPE)(t_ARG_LIST), NULL);
	Node * decl_node = init_node((lli)NULL, (NODETYPE)(t_DECL), NULL);
	Node * body_node = init_node((lli)NULL, (NODETYPE)(t_FUNC_BODY), NULL);

	add_all_children(node, return_node, NULL);
	add_all_children(node, arg_node, NULL);
	add_all_children(node, decl_node, NULL);

	add_all_children(body_node, body, NULL);

	add_all_children(node, body_node, NULL);
	return node;
}

extern int	lineno;
extern char* filename;

void yyerror ( const char  *s) {
	fprintf (stderr, "%s", s);
	fprintf(stderr, " near line %d ", lineno);
	fprintf(stderr, " in %s\n", filename);
	exit(1);
 }


