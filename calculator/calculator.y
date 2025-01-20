%{
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Function prototypes
int warning(const char *msg, const char *details);
int yylex(void);
int yyerror(const char *s);
long fetchVar(char* name);
void upsert(char* name, long value);
int find(char* name);
void printVars();
long handleOP(char op, long first, long second);
long ans = 0;
int displayLast= 0;
%}
%left '(' ')'
%left '*' '/'
%left '+' '-'
%right ASSIGN
%union {
	float fnum; // store floating number
	char* str; // veriable name
	long num; // store actual number
	char op; // to store the op
}
%token <num> LITERAL
%token <str> IDENTIFIER
%token <op> OP
%token SEMICOLON
%token PRINT
%token BRACKET_OPEN
%token BRACKET_CLOSED
%start statements
%define parse.error verbose
%type <num> expression TERM
%%
statements:
	| statements statement
	;
statement:
	| end_of_line
	| assignment end_of_line
	| expression end_of_line {
	displayLast = 1;
	ans = $1;
	}
	;
end_of_line: SEMICOLON
	| PRINT {
	printVars();
	}
	;
assignment: IDENTIFIER ASSIGN expression {
	  upsert($1, $3);
	  }
	;
expression: BRACKET_OPEN expression BRACKET_CLOSED {$$ = $2;}
	| TERM
	| expression OP expression {$$ =handleOP($2, $1, $3);}
	;
TERM: LITERAL { $$ = $1; }
   | IDENTIFIER {
	   $$ = fetchVar($1);
	}
    ;
%%
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
long* var_values = NULL;
char** var_names = NULL;
int size = 0;
extern char	*progname;	/* for error messages */
int	lineno = 1;

int yyerror(const char* s)	/* called for yacc syntax error */
{
	return warning(s, (char *) 0);
}


int warning(const char *s, const char *t)	/* print warning message */
{
	fprintf(stderr, "%s: %s", progname, s);
	fprintf(stderr, " %s %ld %c\n", yylval.str, yylval.num, yylval.op);
	if (t)
		fprintf(stderr, " %s %s %ld %c\n", t, yylval.str, yylval.num, yylval.op);
	fprintf(stderr, " near line %d\n", lineno);
	return 0;
}
int find(char* name) {
	int ind = -1;
	for (int i =0; i < size; i ++) {
		if (strcmp(var_names[i],name) == 0 ) {
			ind = i;
			break;
			}
	}
	return ind;
}

long handleOP(char op, long first, long second) {
	switch(op) {
                  case '+':
                          return first + second;
                  break;
                  case '-':
                          return first - second;
                  break;
                  case '/':
                          return first / second;
                  break;
                  case '*':
                          return first * second;
                 break;
		 default:
			 fprintf(stderr, "Undefined operation:  %c.\n", op);
			 exit(1);
                }

}

void upsert(char* name, long value) {
	int pos = find(name);
	if (pos == -1 ) {
		// insert
		pos = size++;
		var_values = (long*) realloc(var_values,size * sizeof(long));
		var_names = (char**) realloc(var_names,size * sizeof(char*));
		var_names[pos] = strdup(name);
	}
	// update
	var_values[pos] = value;
}
long fetchVar(char* name) {
	int pos = find(name);
	if (pos == -1) {
//		fprintf(stderr, "Undefined variable %s\n", name);
//		exit(1);
		return 0;
	}
	return var_values[pos];
}
void printVars() {
	if (displayLast != 0) {
		printf("%ld\n",ans);
		displayLast = 0;
	}
	printf("No of Variables: %d\n---------------\n" , size);
	for (int i =0; i < size;i ++) {
		printf("%s : %ld\n",var_names[i], var_values[i]);
	}
	printf("--------------\n");
}
