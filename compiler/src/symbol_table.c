#include "../include/includes.h"

/**
 * Inits the struct with one global symbol table
 */
SymbolTables* init_sym_tables() { 
    SymbolTables* symt = (SymbolTables*) (malloc(sizeof(SymbolTables)));
    // symt->global = init_hash_map(hash_string);
    symt->local_tables = (HashMap**)(malloc(sizeof(HashMap*)));
    symt->local_tables[0] = init_hash_map(hash_string, compare_string);
    symt->no_local_tables = 1; // global symbol table
    return symt;
}

int add_local_symbol_table(SymbolTables* symtables) {
    symtables->no_local_tables ++;
    symtables->local_tables = (HashMap**) (realloc(symtables->local_tables, sizeof(HashMap*) * symtables->no_local_tables)) ;
    symtables->local_tables[symtables->no_local_tables - 1] = init_hash_map(hash_string, compare_string);
    return 0;
}

int remove_local_symbol_table(SymbolTables* symtables) {
    symtables->no_local_tables --;
    free_hashmap(symtables->local_tables[symtables->no_local_tables]);
    symtables->local_tables = (HashMap**) (realloc(symtables->local_tables, sizeof(HashMap*) * symtables->no_local_tables)) ;
    return 0;
}

/**
 * returns value of the variable, searching from local-most symbol table all way to the global symbol table
 * return INT_MIN if not present in ANY;
 */
lli value_Of(SymbolTables* symtables, lli variable) {
    lli value = (lli)INT_MIN;
    int pos = symtables->no_local_tables - 1;
    while (pos >= 0 && (value = get(symtables->local_tables[pos --], variable)) == (lli)INT_MIN);
    return value;
}
lli* keys_at(SymbolTables* symt);
/**
 * Upserts a variable and its value to the local-most symbol table if present else to the gloabl symbol table
 */
int upsert_to(SymbolTables* symt, lli key,lli value) {
    // printf("..%d\n", len_at(symt));
    return upsert(symt->local_tables[symt->no_local_tables - 1], key, value);
}

lli* keys_at(SymbolTables* symt) {
    return keys(symt->local_tables[symt->no_local_tables - 1]);
}

int len_at(SymbolTables* symt) {
    // printf("..%d\n", symt->no_local_tables);
    return (symt->local_tables[symt->no_local_tables - 1])->len;
}

int freeAll(SymbolTables* symt) {
    while (symt->no_local_tables != 0) {
        remove_local_symbol_table(symt);
    }
}