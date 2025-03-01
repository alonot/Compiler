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
    if (symtables->no_local_tables == 0) {
        free(symtables->local_tables);
    } else {
        symtables->local_tables = (HashMap**) (realloc(symtables->local_tables, sizeof(HashMap*) * symtables->no_local_tables)) ;
    }
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

STEntry* create_stentry(STETYPE dtype, char* name) {
    STEntry* ste = (STEntry*) calloc(1, sizeof(STEntry));
    ste->dtype = dtype;
    ste->name = name;
}

int add_array_layer(STEntry* ste, int len) {
    if (len <= 0) {
        return -1;
    }
    if (ste->is_array == 0 ) {
        // adding first layer
        ste->is_array = 1;
        ste->value.arrval = (DARRAY*) calloc(1, sizeof(DARRAY));
        ste->value.arrval->arr_max_pos = 1;
    }
    ste->value.arrval->arr_max_pos *= len;
    ste->value.arrval->arr_depth ++;
    ste->value.arrval->arr_lengths = (int*) realloc(ste->value.arrval->arr_lengths, sizeof(int) * ste->value.arrval->arr_depth);
    ste->value.arrval->arr_lengths[ste->value.arrval->arr_depth - 1] = len;
    switch (ste->dtype)
    {
    case DOUBLE:
        free(ste->value.arrval->arr);
        ste->value.arrval->arr = (char*) (malloc(sizeof(double) * ste->value.arrval->arr_max_pos));
        break;
    
    case INT:
        free(ste->value.arrval->arr);
        ste->value.arrval->arr = (char*) (malloc(sizeof(lli) * ste->value.arrval->arr_max_pos));
        break;
    
    case BOOL:
        free(ste->value.arrval->arr);
        ste->value.arrval->arr = (char*) (malloc(sizeof(short) * ste->value.arrval->arr_max_pos));
        break;

    default:
        break;
    }
    return 0;
}

lli update_double(STEntry* ste, double val,int pos) {
    if (pos > -1 && ste->is_array == 0) { // not array but still a position is given
        return LLONG_MIN + 1;
    } 
    if (pos == -1 && ste->is_array != 0) { // array but no position is given
        return LLONG_MIN;
    } 
    if (ste->dtype != DOUBLE) { // wrong type
        return LLONG_MIN + 2;
    }
    if (pos >= ste->value.arrval->arr_max_pos) {
        return LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1) {
        ste->value.dval = val;
    } else {
        *((double*)(ste->value.arrval->arr) + pos) = val;
    }
    return 0;
}

lli update_int(STEntry* ste, lli val,int pos) {
    if (pos > -1 && ste->is_array == 0) { // not array but still a position is given
        return LLONG_MIN + 1;
    } 
    if (pos == -1 && ste->is_array != 0) { // array but no position is given
        return LLONG_MIN;
    } 
    if (ste->dtype != INT) { // wrong type
        return LLONG_MIN + 2;
    }
    if (pos >= ste->value.arrval->arr_max_pos) {
        return LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1) {
        ste->value.lval = val;
    } else {
        *((lli*)(ste->value.arrval->arr) + pos) = val;
    }
    return 0;
}

lli update_bool(STEntry* ste, short val,int pos) {
    if (pos > -1 && ste->is_array == 0) { // not array but still a position is given
        return LLONG_MIN + 1;
    } 
    if (pos == -1 && ste->is_array != 0) { // array but no position is given
        return LLONG_MIN;
    } 
    if (ste->dtype != BOOL) { // wrong type
        return LLONG_MIN + 2;
    }
    if (pos >= ste->value.arrval->arr_max_pos) {
        return LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1) {
        ste->value.lval =val != 0;
    } else {
        *((short*)(ste->value.arrval->arr) + pos) = val;
    }
    return 0;
}

lli* get_double(STEntry* ste, int pos) {
    if (pos > -1 && ste->is_array == 0) { // not array but still a position is given
        return (lli*)(INT_MIN + 1);
    } 
    if (pos == -1 && ste->is_array != 0) { // array but no position is given
        return (lli*)ste->value.arrval->arr;
    } 
    if (ste->dtype != DOUBLE) { // wrong type
        return (lli*)(INT_MIN + 2);
    }
    if (pos >= ste->value.arrval->arr_max_pos) {
        return (lli*)LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1) {
        return (lli*)&(ste->value.dval);
    } else {
        return (lli*)((double*)ste->value.arrval->arr + pos);
    }
    return 0;
}

lli* get_int(STEntry* ste, int pos) {
    if (pos > -1 && ste->is_array == 0) { // not array but still a position is given
        return (lli*)(INT_MIN + 1);
    } 
    if (pos == -1 && ste->is_array != 0) { // array but no position is given
        return (lli*)ste->value.arrval->arr;
    } 
    if (ste->dtype != INT) { // wrong type
        return (lli*)(INT_MIN + 2);
    }
    if (pos >= ste->value.arrval->arr_max_pos) {
        return (lli*)LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1) {
        return &(ste->value.lval);
    } else {
        return ((lli*)ste->value.arrval->arr + pos);
    }
    return 0;
}

lli* get_bool(STEntry* ste, int pos) {
    if (pos > -1 && ste->is_array == 0) { // not array but still a position is given
        return (lli*)(INT_MIN + 1);
    } 
    if (pos == -1 && ste->is_array != 0) { // array but no position is given
        return (lli*)ste->value.arrval->arr;
    } 
    if (ste->dtype != BOOL) { // wrong type
        return (lli*)(INT_MIN + 2);
    }
    if (pos >= ste->value.arrval->arr_max_pos) {
        return (lli*)LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1) {
        return &(ste->value.lval);
    } else {
        return (lli*)((short*)(ste->value.arrval->arr) + pos);
    }
    return 0;
}

void free_ste(STEntry* ste) {
    if (ste->is_array != 0) {
        if (ste->value.arrval->arr != NULL) free(ste->value.arrval->arr);
        if (ste->value.arrval->arr_lengths != NULL) free(ste->value.arrval->arr_lengths);
        free(ste->value.arrval);
    }
    free(ste);
}

int sprintf_ste(STEntry* ste, char* val, int len) {
    int start = 0;
    int total_len = len;
    len = total_len - start;
    switch (ste->dtype)
    {
        case DOUBLE:
        start += snprintf(val + start, len, "(double)");
        break;
        case INT:
        start += snprintf(val + start, len, "(int)");
        break;
        case BOOL:
        start += snprintf(val + start, len, "(bool)");
        break;
        default:
        break;
    }
    len = total_len - start;
    if (ste->is_array) {
        start += snprintf(val + start, len, "[]");
    }
    start += snprintf(val + start, len, "(%s)", ste->name);
    return start;
}