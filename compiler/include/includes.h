
#include "string.h"
#include "limits.h"
#include "stdlib.h"
#include "math.h"
#include <stdio.h>

#define ulli unsigned long long int
#define lli long long int
#define MIN_HASH 5
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



typedef struct _SymbolTables {
    // HashMap* global;
    HashMap** local_tables;
    int no_local_tables;
} SymbolTables;

/**
 * Inits the struct with one global symbol table
 */
SymbolTables* init_sym_tables();

int add_local_symbol_table(SymbolTables* symtables);

int remove_local_symbol_table(SymbolTables* symtables);

/**
 * returns value of the variable, searching from local-most symbol table all way to the global symbol table
 * return INT_MIN if not present in ANY;
 */
lli value_Of(SymbolTables* symtables, lli variable);

/**
 * Adds a variable and its value to the local-most symbol table if present else to the gloabl symbol table
 */
lli upsert_to(SymbolTables* symt, lli key,lli value);

/**
 * Update a variable(if present) and its value to the local-most symbol table if present else to the gloabl symbol table
 */
lli update_to(SymbolTables* symt, lli key,lli value);

/**
 * Returns all the keys at top most level symbol table
 */
lli* keys_at(SymbolTables* symt);

int len_at(SymbolTables* symt);

int freeAll(SymbolTables* symt);

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

/** 
 * Pushes a value to the stack
 */
void push_stack(Stack* st, lli val);

/**
 * pops the top of the stack
 * LONG_MIN if empty
 */
lli pop_stack(Stack* st);

/************************************ */

typedef enum __ty {
    t_STR,
    t_IDENTIFIER,
    t_ARRAY_SIZE,
    t_VAR,
    t_NUM,
    t_ASSIGN,
    t_OP,
    t_MINUS,
    t_GTE,
    t_EE,
    t_LTE,
    t_NE,
    t_FUNC,
    t_BOOLEAN,
    t_OTHER,
    t_KEYWORD,
    t_STE // symbol table entry
} NODETYPE;

/**************Tree*********** */

typedef struct __tree {
    struct __tree* first_child;
    struct __tree* next;
    struct __tree* last_child;
    lli val;
    NODETYPE n_type;
    void (*free)(lli);
} Node;

Node* init_node(lli val, NODETYPE n_type, void (*free)(lli));

/**
 * adds new node to the end of the list of children
 * makes child.next = NULL
 */
int add_child(Node* node, Node* child);
int add_all_children(Node* node, Node* child, Node* lastchild);

void printTree(Node* node);

void printTreeIdent(Node* node);

void free_tree(Node*);

int add_neighbour(Node* node, Node* child);

int update_last_child(Node* node);

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

typedef enum _ste_dtype {
    DOUBLE,
    INT,
    BOOL
} STETYPE;

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
    char* name; // variable entry name
    short is_array; // to tell whether this entry is array or not
} STEntry ;

/**
 * creates an entry with empty val and given dtype
 */
STEntry* create_stentry(STETYPE dtype, char* name);

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

/************************************ */


