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
	#include "../include/includes.h"
	char* filename;
	struct _string* str;
	int 	lineno = 1;
	int     wflag;
	int comments = 0;
	int in_string = 0;
%}

%option noyywrap
digit	[0-9]
letter	[a-zA-Z]

%%
return 			{ return RETURN; }
begin				{ return BEG; }	
end			{ return END; }	
integer			{ yylval.num = 1; return T_INT; }	
boolean				{ yylval.num = 2; return T_BOOL; }	
float				{ yylval.num = 2; return T_DOUBLE; }	
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
"++"				{ return PLUSPLUS; }	
"--"				{ return MINUSMINUS; }	
"<="				{ return LESSTHANOREQUAL; }
">="				{ return GREATERTHANOREQUAL; }
"!="				{ return NOTEQUAL; }	
(&|AND)          { return LOGICAL_AND; }
("|"|OR)			{ return LOGICAL_OR; }
(NOT|"!")		{ return LOGICAL_NOT; }
true			{ return T; }	
false			      { return F; }
break			      { return BREAK; }
continue			      { return CONTINUE; }

({digit}+(\.{digit}+))			{ yylval.num = atof(yytext);	return NUM_FLOAT; }

({digit}+)			{ yylval.num = atof(yytext);	return NUM; }

{letter}({letter}|{digit})*	{ yylval.name = strdup(yytext);return VAR; /* variable name */}
[-+*/=()><;,\{\}\[\]\%]         { return yytext[0]; }
("//".*\n)							{}
\"([^\"\\]|\\.)*\"                             { 	
	yytext[yyleng - 1] = '\0';
	str = init_string(yytext + 1, yyleng - 2);
	yylval.str = str; return STR;
}
[\n]					{ lineno ++; comments = 0; }
[ \t]
.				{ 
	fprintf(yyout,"Unrecognized token : %s\n", yytext); exit(1); 
	
}		

%%
#include <unistd.h>
SymbolTable* current_symbol_table;
Node* prog_root = NULL;
HashMap* labels;
String* label_data;
FILE* debug;
int interpret = 0;

void parse_optional_argument(int argc, char* argv[]) {
	int opt;

	char* output_file = "a.s";
	int verbose = 0;

	while ((opt = getopt(argc, argv, "ivo:d:")) != -1) {
        switch (opt) {
        case 'i': interpret = 1;  break;
        case 'v': verbose = 1;  break;
        case 'o': 
			output_file = optarg;
		break;
        case 'd': 
			if (optarg == NULL) {
				fprintf(stderr,"Unable to parse filename for -o\n");
				exit(1);
			}

			debug = fopen(optarg, "w+");

			if (debug == NULL ) {
				fprintf(stderr, "Unable to open file %s\n", optarg);
				exit(1);
			}
		break;
        default:
            fprintf(stderr, "Usage: %s [-ivod] [file...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

	extern FILE* yyout;

	if (output_file == NULL) {
		fprintf(stderr,"Unable to parse filename for -o\n");
		exit(1);
	}

	yyout = fopen(output_file, "w+");

	if (yyout == NULL ) {
		fprintf(stderr, "Unable to open file %s\n", output_file);
		exit(1);
	}

	if (verbose == 0) {
		debug = fopen("/dev/null", "w");
	} else if (debug == NULL) {
		debug = stdout;
	}

	// optind (extern) have index of first non option argument
}

Node* call_main() {
	Node * node = init_node((lli)"main", (NODETYPE)(t_FUNC_CALL), NULL);
	Node * param_node = init_node((lli)NULL, (NODETYPE)(t_PARAM_LIST), NULL);
	add_child(node, param_node);
	return node;	
}


int main(int argc , char* argv[]){
	parse_optional_argument(argc, argv);

	// optind (extern) have index of first non option argument

	if (optind < argc) {
		extern FILE* yyin;

		if (argv[optind] == NULL) {
			fprintf(yyout,"Unable to parse filename for input\n");
			exit(1);
		}

		filename = argv[optind];
		yyin = fopen(argv[optind], "r");

		if (yyin == NULL ) {
			fprintf(stderr, "Unable to open file %s\n", argv[optind]);
			exit(1);
		}
	} else {
		filename = "stdin";
	}

	labels = init_hash_map(hash_string, compare_string);
	label_data = init_string("", 0);

	current_symbol_table = add_sym_tables(NULL, "main", NULL);
	insert(labels, (lli)current_symbol_table->name, (lli)current_symbol_table -> name);
	
	yyparse();

	// prog_root -> neighbour  is prog_block
	// prog_block -> first_child is "global"
	// global -> 4th child is func_body
	Node* global = prog_root->next->first_child->first_child;
	Node* global_body = global->next->next->next;
	/* add_child(global_body, call_main()); */

	Node* global_decl = global->next->next;
	global->next->next = prog_root;
	Node* tmp = prog_root -> next;
	prog_root -> next = global_body;
	prog_root = tmp;
	global_decl -> next = NULL;

	printTreeIdent(prog_root);
	fflush(debug);
	free(global_decl);

	instr_out("%s\n", label_data->val);
	if (interpret == 1) {
		fprintf(yyout,"------------------------------------------------------\nRunning the program\n----------------------------\n");
		run(prog_root);
		fprintf(yyout,"----------------------------\nProgram Execution finished\n------------------------------------------------------\n");
	} else {
		/* fprintf(debug,"------------------------------------------------------\nCompiling the program\n----------------------------\n"); */
		compile(prog_root);
		/* fprintf(yyout,"----------------------------\nCompiled\n------------------------------------------------------\n"); */
	}
	printSymbolTables(prog_root);

	free_tree(prog_root);
	freeString(label_data);
	free_hashmap(labels);
	if (debug != yyout) {
		fclose(debug);
	}
	fclose(yyout);
	fclose(yyin);

	return 0;
}
