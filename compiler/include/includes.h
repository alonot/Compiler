
#include "string.h"
#include "limits.h"
#include "stdlib.h"
#include "math.h"

#define ulli unsigned long long int
#define MIN_HASH 5
/**
 * Converts string to int for hashing it
 * Gives different weights to different positions hence enforces ordering.
 */
ulli hash_string(char* str);

short compare_string(char* s1, char* s2);

ulli hash_int(char* num);

typedef struct _HashMap {
    char** values; // either values are int or memory address
    char** keys; // bit map is set or not
    int size; // total length
    int len; // actual length
    ulli (*hash)(char*); // hash function
    short (*compare)(char* , char*);  // compare keys , must return 0 if equal
} HashMap;

HashMap* init_hash_map(ulli (*hash)(char*), short (*compare)(char* , char*));

int insert(HashMap* hm, char* key, char* value);

int upsert(HashMap* hm, char* key, char* value);

/**
 * Reallocates the HashMap
 */
int reallocate_hash(HashMap* hm, int prev_size);

char* get(HashMap* hm, char* key);

int remove_hm(HashMap* hm, char* key);

/**
 * returns keys set of the hashMap
 */
char** keys(HashMap* hm);

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
char* value_Of(SymbolTables* symtables, char* variable);

/**
 * Adds a variable and its value to the local-most symbol table if present else to the gloabl symbol table
 */
int upsert_to(SymbolTables* symt, char* key,char* value);

/**
 * Returns all the keys at top most level symbol table
 */
char** keys_at(SymbolTables* symt);

int len_at(SymbolTables* symt);

int freeAll(SymbolTables* symt);

