
#include "string.h"
#include "limits.h"
#include "stdlib.h"
#include "math.h"
#include <stdio.h>
#include <stdarg.h>


#define ulli unsigned long long int
#define lli long long int
#define MIN_HASH 5

void yyerror( const char* );

/**
 * Converts string to int for hashing it
 * Gives different weights to different positions hence enforces ordering.
 */
lli hash_string(lli str);

short compare_string(lli s1, lli s2);

lli hash_int(lli num);

typedef struct _HashMap {
    lli* values; // either values are int or memory address
    lli* keys; // bit map is set or not
    int size; // total length
    int len; // actual length
    lli (*hash)(lli); // hash function
    short (*compare)(lli , lli);  // compare keys , must return 0 if equal
} HashMap;

HashMap* init_hash_map(lli (*hash)(lli), short (*compare)(lli , lli));

lli insert(HashMap* hm, lli key, lli value);

lli upsert(HashMap* hm, lli key, lli value);

lli update(HashMap* hm, lli key, lli value);
/**
 * Reallocates the HashMap
 */
int reallocate_hash(HashMap* hm, int prev_size);

lli get(HashMap* hm, lli key);

/**
 * Its users responsibility to free up the keys and values if they are pointers
 */
int remove_hm(HashMap* hm, lli key);

/**
 * returns keys set of the hashMap
 */
lli* keys(HashMap* hm);

/**
 * Its users responsibility to free up the keys and values if they are pointers
 */
int free_hashmap(HashMap* hm);



typedef struct _SymbolTable {
    HashMap* local_table;
    char* name;
    void (*free)(char*);
    struct _SymbolTable* parent ;
} SymbolTable;

/**
 * Adds a symbol table with given name with symboltable(argument) as parent
 */
SymbolTable *add_sym_tables(SymbolTable* symboltable,char* name,void (*fnfree)(char*));

/**
 * returns parent symbol table
 */
SymbolTable *parent_sym_table(SymbolTable* symboltable);

/**
 * finds wether a variable is present in the current local table.
 */
lli find_local(SymbolTable* symboltable, lli variable);

/**
 * free given symbol table
 */
int free_symbol_table(SymbolTable *symtable);

/**
 * returns value of the variable, searching from local-most symbol table all way to the global symbol table
 * return INT_MIN if not present in ANY;
 */
lli value_Of(SymbolTable* symtables, lli variable);

/**
 * Adds a variable and its value to the local-most symbol table if present else to the gloabl symbol table
 */
lli upsert_to(SymbolTable* symt, lli key,lli value);

/**
 * Update a variable(if present) and its value to the local-most symbol table if present else to the gloabl symbol table
 */
lli update_to(SymbolTable* symt, lli key,lli value);

/**
 * Returns all the keys at top most level symbol table
 */
lli* keys_at(SymbolTable* symt);

int len_at(SymbolTable* symt);

/**************STACK *************** */

typedef struct __stack {
    lli* values;
    int top;
    int size;
} Stack;

/**
 * init stack with top = 0 and size = 1
 */
Stack* init_stack();

void free_stack(Stack* st);

/** 
 * Pushes a value to the stack
 */
void push_stack(Stack* st, lli val);

/**
 * pops the top of the stack
 * LONG_MIN if empty
 */
lli pop_stack(Stack* st);

/**
 * return the top of the stack
 * LONG_MIN if empty
 */
lli top_stack(Stack* st);

/************************************ */

/**************Queue *************** */


// QNode structure
typedef struct QNode {
    lli data;
    struct QNode* next;
} QNode;

// Queue structure
typedef struct {
    QNode* front;
    QNode* rear;
    int len;
} Queue;

/**
 * init queue with top = 0 and size = 1
 */
Queue* init_queue();

void free_queue(Queue*);

/** 
 * enqueue a value to the queue
 */
void enqueue_queue(Queue* st, lli val);

/**
 * pops the front of the queue
 * LONG_MIN if empty
 */
lli dequeue_queue(Queue* st);

/**
 * return the front of the queue
 * LONG_MIN if empty
 */
lli front_queue(Queue* st);

/**
 * find and remove oldest instance of given value 
 */
lli remove_item(Queue* qt, lli val);

/************************************ */

typedef enum __ty {
    t_STR,
    t_IDENTIFIER,
    t_ARRAY_SIZE,
    t_VAR,
    t_NUM_I,
    t_NUM_F, // just to know if initial number was float or not
    t_ASSIGN,
    t_OP,
    t_MINUS,
    t_GTE,
    t_EE,
    t_LTE,
    t_NE,
    t_PLUSPLUS_POST,
    t_MINUSMINUS_POST,
    t_PLUSPLUS_PRE,
    t_MINUSMINUS_PRE,
    t_FUNC_CALL, // for function calls
    t_FUNC_BODY, // for function body
    t_FUNC_RET, // for function return
    t_DECL, // for declaration blocks
    t_FUNC_DEF, // function definition
    t_ARG_LIST,
    t_PARAM_LIST,
    t_BOOLEAN,
    t_PROG,
    t_OTHER,
    t_KEYWORD,
    t_EXPR,
    t_BREAK,
    t_CONTINUE,
    t_STE, // symbol table entry
    t_COND, // for conditional 
    t_IF_BLOCK, // if block
    t_ELSE_BLOCK, // else block
    t_LOOP_BLOCK, // for block
    t_WHILE, // for while loop
    t_DOWHILE, // for do-while loop
    t_FOR, // for loop
    t_NOP, // no operation
} NODETYPE;

/**************Tree*********** */

typedef struct __tree {
    struct __tree* first_child;
    struct __tree* next;
    struct __tree* last_child;
    lli val;
    int depth;
    NODETYPE n_type;
    void (*free)(lli);
} Node;

Node* init_node(lli val, NODETYPE n_type, void (*free)(lli));

/**
 * adds new node to the end of the list of children
 * makes child.next = NULL
 */
int add_child(Node* node, Node* child);

int add_child_in_front(Node* node, Node* child);

int add_all_children(Node* node, Node* child, Node* lastchild);

void printTree(Node* node);

void printTreeIdent(Node* node);

void free_tree(Node*);

int add_neighbour(Node* node, Node* child);

int update_last_child(Node* node);

void printSymbolTables(Node* root);

/*************String***************** */
typedef struct _string {
    char* val;
    int length;
    int size;
} String;

String* init_string(char* val, int len);

/**
 * adds given val upto len characters to the str
 * len must be excluding last \0
 * if len == -1 then len = strlen(val);
 */
void add_str(String* str, char* val, int len);

/**
 * repeasts the char n times and add this new char to given str
 */
void repeat_n_add(String* str, char val, int times);

void freeString(String* str);

int length(String* str);

/************************************ */

#include "compiler.h"

typedef enum _ste_dtype {
    BOOL,
    INT,
    DOUBLE
} STETYPE;

typedef enum _ste_var_type {
    ARG,
    LOCAL
} VARTYPE;

typedef struct _array_info {
    char* arr; // null for non-array
    int* arr_lengths; // stores length at each level
    int arr_max_pos; // stores total length of array including all depth
    int arr_depth; // 0 for non array
    int arr_curr_depth; // current depth while accessing a variable
    int arr_curr_pos; // current position while accessing this variable
} DARRAY;

typedef union _ste_val {
    double dval;
    lli lval;
    DARRAY* arrval;
} STEVAL;

typedef struct STEntry {
    STEVAL value;
    STETYPE dtype;
    VARTYPE var_type; // type of variable (argument , local, etc)
    char* name; // variable entry name
    short is_array; // to tell whether this entry is array or not
    int loc_from_ref_reg; 
    RegPromise* ref_reg; // this is always among : {fp, sp, gp}
} STEntry ;

/**
 * creates an entry with empty val and given dtype
 */
STEntry* create_stentry(STETYPE dtype, char* name, VARTYPE var_type);

/**
 * sprintf the ste info in given val
 */
int sprintf_ste(STEntry*, char*, int);

/**
 * updates the double entry . requires that type is declared double
 * pos : position in array where value must be updated
 * returns LLONG_MIN + 1 if fails with not array but still a position is given
 * returns LLONG_MIN + 2 if fails with wrong type
 * returns LLONG_MIN if fails with array but no position is given
 */
lli update_double(STEntry* ste, double val,int pos);

/**
 * updates the int entry . requires that type is declared int
 * returns LLONG_MIN + 1 if fails with not array but still a position is given
 * returns LLONG_MIN + 2 if fails with wrong type
 * returns LLONG_MIN if fails with array but no position is given
 */
lli update_int(STEntry* ste, lli val,int pos);

/**
 * updates the bool entry . requires that type is declared bool
 * returns LLONG_MIN + 1 if fails with not array but still a position is given
 * returns LLONG_MIN + 2 if fails with wrong type
 * returns LLONG_MIN if fails with array but no position is given
 */
lli update_bool(STEntry* ste, short val,int pos);

/**
 * returns the double entry . requires that type is declared double
 * pos : position in array where value must be updated
 * returns INT_MIN + 1 if fails with not array but still a position is given
 * returns INT_MIN + 2 if fails with wrong type
 */
lli* get_double(STEntry* ste,int pos);

/**
 * returns the int entry . requires that type is declared int
 * returns INT_MIN + 1 if fails with not array but still a position is given
 * returns INT_MIN + 2 if fails with wrong type
 */
lli* get_int(STEntry* ste,int pos);

/**
 * returns the bool entry . requires that type is declared bool
 * returns INT_MIN + 1 if fails with not array but still a position is given
 * returns INT_MIN + 2 if fails with wrong type
 */
lli* get_bool(STEntry* ste,int pos);

/**
 * Adds a layer of array. (increase depth of array with len elements) .
 * If initially not array , then creates one with len elements
 */
int add_array_layer(STEntry* ste, int len);

void free_ste(STEntry* ste) ;


/************************************ */

/**
 * This struct is used by interpret to store values
 */

/****************Run******************** */

/**
 * returns 0 for success
 *         1 for break
*          2 for continue
 */
int run(Node* node);


/**
 * assigns the value in addr to the val according to dtype
 */
void assign_val(lli *addr, int dtype, double* val);

void evaluate_function(Node* node, double* val);

/**
 * requires the node to be t_ASSIGN type
 */
void evaluate_assign(Node *node);

String *evaluate_string(Node *node);

int check_eq_to_int(double val, int to, int dtype);

/**
 * requires the node to be t_STE type
 * returns type of the variable
 */
int resolve_ste(Node *node, lli **addr);

/**
 * expected child of the t_EXPR node
 * * returns the type of expression
 */
int evalute_expr(Node *node, double *val);

/**
 * assigns the val to the addr according to dtype
 */
void assign_addr(lli *addr, int dtype, double val);

/************************************ */
