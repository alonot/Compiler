#include "../include/includes.h"

/**
 * Inits the struct with one global symbol table
 */
SymbolTable *init_sym_tables(char* name,void (*fnfree)(char*))
{
    SymbolTable *symt = (SymbolTable *)(malloc(sizeof(SymbolTable)));
    symt->local_table = init_hash_map(hash_string, compare_string);
    symt->parent = NULL; // global symbol table
    symt->free = fnfree;
    symt->name = name;
    return symt;
}

SymbolTable *add_sym_tables(SymbolTable* symboltable,char* name,void (*fnfree)(char*))
{
    SymbolTable *symt = init_sym_tables(name, fnfree);
    symt->parent = symboltable;
    return symt;
}

SymbolTable *parent_sym_table(SymbolTable* symboltable) {
    return symboltable->parent;
}



int free_symbol_table(SymbolTable *symtable)
{
    HashMap *table = symtable->local_table;
    lli *table_keys = keys(table);
    for (int i = 0; i < table->len; i++)
    {
        STEntry *value = (STEntry *)get(table, table_keys[i]);
        free((char *)table_keys[i]);
        free_ste(value);
    }
    if (symtable->free != NULL) {
        symtable->free(symtable->name);
    }
    free(table_keys);
    free_hashmap(table);
    free(symtable);
    return 0;
}

/**
 * returns value of the variable, searching from local-most symbol table all way to the global symbol table
 * return INT_MIN if not present in ANY;
 */
lli value_Of(SymbolTable *symtable, lli variable)
{
    lli value = (lli)LONG_MIN;
    while (symtable != NULL && (value = get(symtable->local_table, variable)) == (lli)LONG_MIN) {
        symtable = symtable->parent;
    }
    return value;
}

lli find_local(SymbolTable *symtable, lli variable)
{
    return get(symtable->local_table, variable);
}

void printSTEInSymbolTable(STEntry *ste)
{
    char *dtype;
    switch (ste->dtype)
    {
    case DOUBLE:
        dtype = "DOUBLE";
        break;
    case INT:
        dtype = "INT";
        break;
    case BOOL:
        dtype = "BOOL";
        break;
    default:
        break;
    }
    if (ste->is_array)
    {
        DARRAY *arrval = ste->value.arrval;
        lli max_mult_one_less = 1;
        char arr_indx[100];
        int start = 0;
        start = snprintf(arr_indx,100,"%s", dtype);
        int max_len_indx = 100 -start;
        for (int i = 1; i <= arrval->arr_depth; i++)
        {
            start += snprintf(arr_indx + start, max_len_indx,"[%d]", arrval->arr_lengths[i]);
            if (i != 1)
            {
                max_mult_one_less *= arrval->arr_lengths[i];
            }
        }
        fprintf(debug,"%20s\t | \n",arr_indx);
        // array positions
        int last_depth_pos = arrval->arr_depth;
        int pos = 0;
        lli *addr = (lli*)arrval->arr;
        while (pos < arrval->arr_max_pos)
        {
            fprintf(debug,"|%10s | ", "");
            char arr_indx[100];
            int start = 0;
            start = snprintf(arr_indx,100,"%s", dtype);
            int max_len_indx = 100 -start;
            lli max_pos_less = max_mult_one_less;
            for (int i = 2; i <= arrval->arr_depth; i++)
            {
                start += snprintf(arr_indx + start, max_len_indx,"[%lld]", pos / max_pos_less);
                max_len_indx = 100 - start;
                max_pos_less /= arrval->arr_lengths[i];
            }
            start += snprintf(arr_indx + start, max_len_indx,"[..]");
            fprintf(debug,"%20s\t | ",arr_indx);
            for (int i = 0; i < arrval->arr_lengths[last_depth_pos]; i++)
            {
                switch (ste->dtype)
                {
                case DOUBLE:
                    fprintf(debug,"%lf", *((double*)addr + pos));
                    break;
                case INT:
                    fprintf(debug,"%lld", *(addr + pos));
                    break;
                case BOOL:
                    fprintf(debug,"%s", *((short*)addr + pos) != 0 ? "True" : "False");
                    break;
                default:
                    break;
                }
                pos ++;
                fprintf(debug,", ");
            }
            fprintf(debug,"\n");
        }
    }
    else
    {
        fprintf(debug,"%20s\t | ",dtype);
        switch (ste->dtype)
        {
        case DOUBLE:
            fprintf(debug,"%lf\n", ste->value.dval);
            break;
        case INT:
            fprintf(debug,"%lld\n", ste->value.lval);
            break;
        case BOOL:
            fprintf(debug,"%s\n", ste->value.lval != 0 ? "True" : "False");
            break;
        default:
            break;
        }
    }
}

void printSymbolTable(SymbolTable *symt)
{
    HashMap* table = symt->local_table;
    lli *table_keys = keys(table);
    for (int i = 0; i < table->len; i++)
    {
        STEntry *value = (STEntry *)get(table, table_keys[i]);
        fprintf(debug,"|%10s | ", (char *)table_keys[i]);
        printSTEInSymbolTable(value);
    }
    free(table_keys);
}

void printSymbolTables(Node * root)
{
    if (root->n_type != t_PROG) {
        fprintf(debug, "root given is not of type t_PROG\n");
        return;
    }
    Node* child = root -> first_child;
    while (child !=  NULL) {
        printSymbolTable((SymbolTable*)(child->val));
        child = child -> next;
    }
}


lli upsert_to(SymbolTable *symt, lli key, lli value)
{
    // fprintf(debug,"..%d\n", len_at(symt));
    return upsert(symt->local_table, key, value);
}

/**
 * Update a variable and its value to the local-most symbol table if present else to the gloabl symbol table
 */
lli update_to(SymbolTable *symt, lli key, lli value)
{
    return update(symt->local_table, key, value);
}

lli *keys_at(SymbolTable *symt)
{
    return keys(symt->local_table);
}

int len_at(SymbolTable *symt)
{
    // fprintf(debug,"..%d\n", symt->no_local_tables);
    return (symt->local_table)->len;
}

int freeAll(SymbolTable *symt)
{
    while (symt != NULL)
    {
        free_symbol_table(symt);
        symt = symt -> parent;
    }
}

/*************STEntry********* */

STEntry *create_stentry(STETYPE dtype, char *name, VARTYPE var_type)
{
    STEntry *ste = (STEntry *)calloc(1, sizeof(STEntry));
    ste->dtype = dtype;
    ste->var_type = var_type;
    ste->name = name;
}

int add_array_layer(STEntry *ste, int len)
{
    if (len <= 0)
    {
        return -1;
    }
    if (ste->is_array == 0)
    {
        // adding first layer
        ste->is_array = 1;
        ste->value.arrval = (DARRAY *)calloc(1, sizeof(DARRAY));
        ste->value.arrval->arr_max_pos = 1;
    }
    DARRAY *arrval = ste->value.arrval;
    arrval->arr_max_pos *= len;
    arrval->arr_depth++;
    arrval->arr_lengths = (int *)realloc(arrval->arr_lengths, sizeof(int) * (arrval->arr_depth + 1));
    arrval->arr_lengths[0] = 1;
    arrval->arr_lengths[arrval->arr_depth] = len;
    switch (ste->dtype)
    {
    case DOUBLE:
        free(arrval->arr);
        arrval->arr = (char *)(calloc(arrval->arr_max_pos,sizeof(double)));
        break;

    case INT:
        free(arrval->arr);
        arrval->arr = (char *)(calloc(arrval->arr_max_pos,sizeof(lli)));
        break;

    case BOOL:
        free(arrval->arr);
        arrval->arr = (char *)(calloc(arrval->arr_max_pos,sizeof(short)));
        break;

    default:
        break;
    }
    return 0;
}

lli update_double(STEntry *ste, double val, int pos)
{
    if (pos > -1 && ste->is_array == 0)
    { // not array but still a position is given
        return LLONG_MIN + 1;
    }
    if (pos == -1 && ste->is_array != 0)
    { // array but no position is given
        return LLONG_MIN;
    }
    if (ste->dtype != DOUBLE)
    { // wrong type
        return LLONG_MIN + 2;
    }
    if (pos >= ste->value.arrval->arr_max_pos)
    {
        return LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1)
    {
        ste->value.dval = val;
    }
    else
    {
        *((double *)(ste->value.arrval->arr) + pos) = val;
    }
    return 0;
}

lli update_int(STEntry *ste, lli val, int pos)
{
    if (pos > -1 && ste->is_array == 0)
    { // not array but still a position is given
        return LLONG_MIN + 1;
    }
    if (pos == -1 && ste->is_array != 0)
    { // array but no position is given
        return LLONG_MIN;
    }
    if (ste->dtype != INT)
    { // wrong type
        return LLONG_MIN + 2;
    }
    if (pos >= ste->value.arrval->arr_max_pos)
    {
        return LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1)
    {
        ste->value.lval = val;
    }
    else
    {
        *((lli *)(ste->value.arrval->arr) + pos) = val;
    }
    return 0;
}

lli update_bool(STEntry *ste, short val, int pos)
{
    if (pos > -1 && ste->is_array == 0)
    { // not array but still a position is given
        return LLONG_MIN + 1;
    }
    if (pos == -1 && ste->is_array != 0)
    { // array but no position is given
        return LLONG_MIN;
    }
    if (ste->dtype != BOOL)
    { // wrong type
        return LLONG_MIN + 2;
    }
    if (pos >= ste->value.arrval->arr_max_pos)
    {
        return LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1)
    {
        ste->value.lval = val != 0;
    }
    else
    {
        *((short *)(ste->value.arrval->arr) + pos) = val;
    }
    return 0;
}

lli *get_double(STEntry *ste, int pos)
{
    if (pos > -1 && ste->is_array == 0)
    { // not array but still a position is given
        return (lli *)(INT_MIN + 1);
    }
    if (pos == -1 && ste->is_array != 0)
    { // array but no position is given
        return (lli *)ste->value.arrval->arr;
    }
    if (ste->dtype != DOUBLE)
    { // wrong type
        return (lli *)(INT_MIN + 2);
    }
    if (pos >= ste->value.arrval->arr_max_pos)
    {
        return (lli *)LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1)
    {
        return (lli *)&(ste->value.dval);
    }
    else
    {
        return (lli *)((double *)ste->value.arrval->arr + pos);
    }
    return 0;
}

lli *get_int(STEntry *ste, int pos)
{
    if (pos > -1 && ste->is_array == 0)
    { // not array but still a position is given
        return (lli *)(INT_MIN + 1);
    }
    if (pos == -1 && ste->is_array != 0)
    { // array but no position is given
        return (lli *)ste->value.arrval->arr;
    }
    if (ste->dtype != INT)
    { // wrong type
        return (lli *)(INT_MIN + 2);
    }
    if (pos >= ste->value.arrval->arr_max_pos)
    {
        return (lli *)LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1)
    {
        return &(ste->value.lval);
    }
    else
    {
        return ((lli *)ste->value.arrval->arr + pos);
    }
    return 0;
}

lli *get_bool(STEntry *ste, int pos)
{
    if (pos > -1 && ste->is_array == 0)
    { // not array but still a position is given
        return (lli *)(INT_MIN + 1);
    }
    if (pos == -1 && ste->is_array != 0)
    { // array but no position is given
        return (lli *)ste->value.arrval->arr;
    }
    if (ste->dtype != BOOL)
    { // wrong type
        return (lli *)(INT_MIN + 2);
    }
    if (pos >= ste->value.arrval->arr_max_pos)
    {
        return (lli *)LLONG_MIN + 3; // out of bounds
    }
    if (pos == -1)
    {
        return &(ste->value.lval);
    }
    else
    {
        return (lli *)((short *)(ste->value.arrval->arr) + pos);
    }
    return 0;
}

void free_ste(STEntry *ste)
{
    if (ste->is_array != 0)
    {
        if (ste->value.arrval->arr != NULL)
            free(ste->value.arrval->arr);
        if (ste->value.arrval->arr_lengths != NULL)
            free(ste->value.arrval->arr_lengths);
        free(ste->value.arrval);
    }
    free(ste);
}

int sprintf_ste(STEntry *ste, char *val, int len)
{
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
    if (ste->is_array)
    {
        start += snprintf(val + start, len, "[]");
    }
    start += snprintf(val + start, len, "(%s)", ste->name);
    return start;
}