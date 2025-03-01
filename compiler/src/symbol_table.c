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
    HashMap* table = symtables->local_tables[symtables->no_local_tables];
    lli* table_keys = keys(table);
    for (int i =0; i < table->len; i ++) {
        STEntry* value = (STEntry*) get(table, table_keys[i]);
        free((char*)table_keys[i]);
        free_ste(value);
    }
    free(table_keys);
    free_hashmap(table);
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
lli upsert_to(SymbolTables* symt, lli key,lli value) {
    // printf("..%d\n", len_at(symt));
    return upsert(symt->local_tables[symt->no_local_tables - 1], key, value);
}

/**
 * Update a variable and its value to the local-most symbol table if present else to the gloabl symbol table
 */
lli update_to(SymbolTables* symt, lli key,lli value) {
    return update(symt->local_tables[symt->no_local_tables - 1], key, value);
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

/*************STEntry********* */

STEntry* create_stentry(STETYPE dtype) {
    STEntry* ste = (STEntry*) calloc(1, sizeof(STEntry));
    ste->dtype = dtype;
}

int add_array_layer(STEntry* ste, int len) {
    if (len <= 0) {
        return -1;
    }
    if (ste->array_depth == 0) {
        ste->total_array_length = 1;
    }
    ste->total_array_length *= len;
    ste->array_depth ++;
    ste->array_length = (int*) realloc(ste->array_length, sizeof(int) * ste->array_depth);
    ste->array_length[ste->array_depth - 1] = len;
    switch (ste->dtype)
    {
    case DOUBLE:
        free(ste->arr);
        ste->arr = (char*) (malloc(sizeof(double) * ste->total_array_length));
        break;
    
    case INT:
        free(ste->arr);
        ste->arr = (char*) (malloc(sizeof(lli) * ste->total_array_length));
        break;
    
    case BOOL:
        free(ste->arr);
        ste->arr = (char*) (malloc(sizeof(short) * ste->total_array_length));
        break;

    default:
        break;
    }
    return 0;
}

lli update_double(STEntry* ste, double val,int pos) {
    if (pos > -1 && ste->array_depth == 0) { // not array but still a position is given
        return LLONG_MIN + 1;
    } 
    if (pos == -1 && ste->array_depth != 0) { // array but no position is given
        return LLONG_MIN;
    } 
    if (ste->dtype != DOUBLE) { // wrong type
        return LLONG_MIN + 2;
    }
    if (pos >= ste->total_array_length) {
        return LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1) {
        ste->value.dval = val;
    } else {
        *((double*)(ste->arr) + pos) = val;
    }
    return 0;
}

lli update_int(STEntry* ste, lli val,int pos) {
    if (pos > -1 && ste->array_depth == 0) { // not array but still a position is given
        return LLONG_MIN + 1;
    } 
    if (pos == -1 && ste->array_depth != 0) { // array but no position is given
        return LLONG_MIN;
    } 
    if (ste->dtype != INT) { // wrong type
        return LLONG_MIN + 2;
    }
    if (pos == -1) {
        ste->value.lval = val;
    } else {
        *((lli*)(ste->arr) + pos) = val;
    }
    return 0;
}

lli update_bool(STEntry* ste, short val,int pos) {
    if (pos > -1 && ste->array_depth == 0) { // not array but still a position is given
        return LLONG_MIN + 1;
    } 
    if (pos == -1 && ste->array_depth != 0) { // array but no position is given
        return LLONG_MIN;
    } 
    if (ste->dtype != BOOL) { // wrong type
        return LLONG_MIN + 2;
    }
    if (pos == -1) {
        ste->value.lval =val != 0;
    } else {
        *((short*)(ste->arr) + pos) = val;
    }
    return 0;
}

lli* get_double(STEntry* ste, int pos) {
    if (pos > -1 && ste->array_depth == 0) { // not array but still a position is given
        return (lli*)(INT_MIN + 1);
    } 
    if (pos == -1 && ste->array_depth != 0) { // array but no position is given
        return (lli*)ste->arr;
    } 
    if (ste->dtype != DOUBLE) { // wrong type
        return (lli*)(INT_MIN + 2);
    }
    if (pos == -1) {
        return (lli*)&(ste->value.dval);
    } else {
        return (lli*)((double*)ste->arr + pos);
    }
    return 0;
}

lli* get_int(STEntry* ste, int pos) {
    if (pos > -1 && ste->array_depth == 0) { // not array but still a position is given
        return (lli*)(INT_MIN + 1);
    } 
    if (pos == -1 && ste->array_depth != 0) { // array but no position is given
        return (lli*)ste->arr;
    } 
    if (ste->dtype != INT) { // wrong type
        return (lli*)(INT_MIN + 2);
    }
    if (pos == -1) {
        return &(ste->value.lval);
    } else {
        return ((lli*)ste->arr + pos);
    }
    return 0;
}

lli* get_bool(STEntry* ste, int pos) {
    if (pos > -1 && ste->array_depth == 0) { // not array but still a position is given
        return (lli*)(INT_MIN + 1);
    } 
    if (pos == -1 && ste->array_depth != 0) { // array but no position is given
        return (lli*)ste->arr;
    } 
    if (ste->dtype != BOOL) { // wrong type
        return (lli*)(INT_MIN + 2);
    }
    if (pos == -1) {
        return &(ste->value.lval);
    } else {
        return (lli*)((short*)(ste->arr) + pos);
    }
    return 0;
}

void free_ste(STEntry* ste) {
    if (ste->arr != NULL) free(ste->arr);
    if (ste->array_length != NULL) free(ste->array_length);
    free(ste);
}