
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

int insert(HashMap* hm, lli key, lli value);

int upsert(HashMap* hm, lli key, lli value);

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
int upsert_to(SymbolTables* symt, lli key,lli value);

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
    t_VAR,
    t_NUM,
    t_ASSIGN,
    t_OP,
    t_MINUS,
    t_GTE,
    t_EE,
    t_LTE,
    t_NE,
    t_BOOLEAN,
    t_FUNC
} NODETYPE;

/**************Tree*********** */

typedef struct __tree {
    struct __tree* first_child;
    struct __tree* next;
    struct __tree* last_child;
    lli val;
    NODETYPE n_type;
} Node;

Node* init_node(lli val, NODETYPE n_type);

/**
 * adds new node to the end of the list of children
 * makes child.next = NULL
 */
int add_child(Node* node, Node* child);

void printTree(Node* node);

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