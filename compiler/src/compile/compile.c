
#include "../../include/includes.h"

Register* zero_reg, * sp_reg, * fp_reg, *ra_reg, *gp_reg, * s0_reg;
RegPromise* zero_reg_promise, * sp_reg_promise, * fp_reg_promise, * ra_reg_promise, * gp_reg_promise, *s0_reg_promise;
Queue* register_usage_queue;
Queue* ste_register_promises;

Register* registers[32];
Register* fregisters[32]; // floating point registers

void loadw(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1);
void __free_regpromise(RegPromise* reg_promise) ;


void clear_offset(RegPromise* reg_promise) {
    if (reg_promise->reg == NULL) {
        return;
    }
    reg_promise->reg->offset = NO_ENTRY;
    reg_promise->immediate = NULL;
    reg_promise->ste = NULL;
}

/**
 * assigns p2 to p1 . i.e. p1 = p2
 */
void assign_reg_promise(RegPromise* p1, RegPromise* p2) {
    p1->immediate = p2->immediate;
    p1->reg = p2->reg;
    p1->reg->reg_promise = p1;
    p1->loc = p2->loc;
    p1->label = p2->label;
    if (p2->ste != NULL) {
        enqueue_queue(ste_register_promises, (lli)p1);
    }
    if (p1->ste != NULL) {
        remove_item(ste_register_promises, (lli)p1);
    }
    p1->ste = p2->ste;
}

void write_instr_val(String* instr_val, int tab, const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len >= sizeof(buffer)) {
        fprintf(stderr, "Formatted string too long\n");
        return;
    }
    if (tab == 1) {
        add_str(instr_val, "\t", 1);
    }
    add_str(instr_val, buffer, len);
    add_str(instr_val, "\n", 1);
}

void write_instr(Instructions_info* instr_info, const char* format, ...) {
    if (instr_info->gen_code == 0) {
        return;
    }

    String* instr_val = (String*)top_stack(instr_info->instructions_stack);
    char buffer[512];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len >= sizeof(buffer)) {
        fprintf(stderr, "Formatted string too long\n");
        return;
    }
    add_str(instr_val, "\t", 1);
    add_str(instr_val, buffer, len);
    add_str(instr_val, "\n", 1);
}

void __write_instr(Instructions_info* instr_info, const char* format, ...) {
    if (instr_info->gen_code == 0) {
        return;
    }

    String* instr_val = (String*)top_stack(instr_info->instructions_stack);
    char buffer[512];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len >= sizeof(buffer)) {
        fprintf(stderr, "Formatted string too long\n");
        return;
    }
    add_str(instr_val, buffer, len);
}

void reload_reg(RegPromise* reg_promise, Instructions_info* instr_info) {
    if (reg_promise->immediate != NULL) {
        // immediate
        yyerror("Got immediate for reloading\n");
        return;
    }
    if (reg_promise->label != NULL) {
        // printf("here %p\n", reg_promise->label);
        RegPromise* new_reg_promise = get_free_register_promise(instr_info);
        load_label(instr_info,new_reg_promise,reg_promise->label->val);
        assign_reg_promise(reg_promise, new_reg_promise);
        free(new_reg_promise);
        return;
    }
    if (reg_promise->reg == NULL) {
        RegPromise* new_reg_promise = get_free_register_promise(instr_info);
        if (reg_promise->ste != NULL) {
            // this means this register had an ste value
            RegPromise* ref_reg_promise = reg_promise->ste->ref_reg;
            ref_reg_promise->reg->offset = reg_promise->ste->loc_from_ref_reg;
            __loadw(instr_info, new_reg_promise->reg, ref_reg_promise->reg); // to prevent infinite rec, using __load
        } else {
            write_instr(instr_info, "#\tFILL");
            // it was an expression
            fp_reg->offset = reg_promise->loc; // load from fp
            __loadw(instr_info,new_reg_promise->reg,fp_reg);
            new_reg_promise->reg->offset = reg_promise->offset;
        }
        assign_reg_promise(reg_promise, new_reg_promise);
        free(new_reg_promise);
    } else {
        // printf("RELOAD%s %d\n", reg_promise->reg->name, reg_promise->reg->offset);
    }
}


void reload_reg_withoffset(RegPromise* reg, Instructions_info* instr_info) {
    if (reg == NULL || reg->immediate) return;
    reload_reg(reg,instr_info);
    if (reg->reg->offset != NO_ENTRY) {
        loadw(instr_info,reg, reg);
    }
    // printf("%s %d\n", reg->reg->name, reg->reg->offset);
}


Register* init_reg(char type, int pos) {
    Register* reg = (Register*) (calloc(1, sizeof(Register)));
    char* val = (char*) calloc(4, sizeof(char));
    val[0] = '$';val[1] = type;val[2] = '0' + pos;
    reg->name  = val;
    reg->offset = NO_ENTRY;
    return reg;
}

void free_reg_struct(Register* reg) {
    free(reg->name);
    free(reg);
}


void initialize_registers() {
    int pos = 0;
    for (int i = 0;i < 10; i ++ ) {
        registers[pos ++] = init_reg('t', i);
    }
    for (int i = 0;i < 4; i ++ ) {
        registers[pos ++] = init_reg('a', i);
    }
    for (int i = 0;i < 2; i ++ ) {
        registers[pos ++] = init_reg('v', i);
    }
    for (int i = 1;i < 8; i ++ ) {
        registers[pos ++] = init_reg('s', i);
    }
    zero_reg = init_reg('0', -'0');
    sp_reg = init_reg('s', 'p' - '0');
    fp_reg = init_reg('f', 'p' - '0');
    gp_reg = init_reg('g', 'p' - '0');
    ra_reg = init_reg('r', 'a' - '0');
    s0_reg = init_reg('s', 0);

    register_usage_queue = init_queue();
    ste_register_promises = init_queue();

    zero_reg_promise = init_reg_promise(zero_reg);
    sp_reg_promise = init_reg_promise(sp_reg);
    fp_reg_promise = init_reg_promise(fp_reg);
    gp_reg_promise = init_reg_promise(gp_reg);
    ra_reg_promise = init_reg_promise(ra_reg);
    s0_reg_promise = init_reg_promise(s0_reg);

    pos = 0;
    for (int i = 0;i < 32; i ++ ) {
        fregisters[pos ++] = init_reg('f', i);
    }
}

void free_registers_struct() {
    int pos = 0;
    for (int i = 0;i < 23; i ++ ) {
        free_reg_struct(registers[i]);
    }
    free_reg_struct(zero_reg);
    free_reg_struct(sp_reg);
    free_reg_struct(gp_reg);
    free_reg_struct(fp_reg);
    free_reg_struct(ra_reg);
    free_reg_struct(s0_reg);
    free(zero_reg_promise);
    free(sp_reg_promise);
    free(gp_reg_promise);
    free(fp_reg_promise);
    free(ra_reg_promise);
    free(s0_reg_promise);

    free_queue(register_usage_queue);
    free_queue(ste_register_promises);

    pos = 0;
    for (int i = 0;i < 32; i ++ ) {
        free_reg_struct(fregisters[pos ++]);
    }
}

void __free_reg(Register* reg) {
    if (reg == NULL) {
        return;
    }
    if (remove_item(register_usage_queue,(lli)reg)) {
        // printf("Unable %s\n", reg->name);
    }
    if (reg->reg_promise != NULL) {
        reg->reg_promise->reg = NULL;
    }
    reg->reg_promise = NULL;
    reg->occupied = 0;
    reg->offset = NO_ENTRY;
}

void __free_regpromise(RegPromise* reg_promise) {
    if (reg_promise == NULL || reg_promise == zero_reg_promise || reg_promise == gp_reg_promise || reg_promise == sp_reg_promise || reg_promise == fp_reg_promise || reg_promise == s0_reg_promise) {
        return;
    }
    remove_item(ste_register_promises, (lli)reg_promise);
    __free_reg(reg_promise->reg);
    free(reg_promise);
}

void store_arg_n_temp_registers(Instructions_info* instr_info, int read) {
    Register* reg;
    while ((lli)(reg = (Register*)dequeue_queue(register_usage_queue)) != LONG_MIN && reg != NULL) {
        // printf("%s\n", reg->name);
        if (reg->name[1] == 't' || reg->name[1] == 'a' || reg->name[1] == 'v') {
            save_n_free(reg, instr_info);
            // later will be reloaded as needed
        } else {
            enqueue_queue(register_usage_queue,(lli)reg);
        }
    }
    if (read == 0) {
        RegPromise* reg_promise;
        while ((lli)(reg_promise = (RegPromise*)dequeue_queue(ste_register_promises)) != LLONG_MIN && reg_promise != NULL) {
            if (reg_promise->ste == NULL) {
                // printf("%p\n", reg_promise);
                yyerror("Empty ste in ste_reg_promises\n");
            }
            reload_reg(reg_promise,instr_info); // got ste value
            save_n_free(reg_promise->reg, instr_info);
        }
    }
}

void load_label(Instructions_info* instr_info, RegPromise* rd, char* label) {
    if (rd->reg == NULL) {
        yyerror("No rd for label\n");
    }
    char* assembler_label = (char*)get(labels,(lli)(label));
    if (assembler_label == NULL || ((lli)assembler_label) == LLONG_MIN) {
        yyerror("Can't find assembler label for given label\n");
    }
    write_instr(instr_info, "%-7s\t%s,\t %%hi(%s)", "lui", rd->reg->name, assembler_label);
    write_instr(instr_info, "%-7s\t%s,\t%s,\t%%lo(%s)", "addiu", rd->reg->name, rd->reg->name, assembler_label);
}

/**
 * Saves the register and set it free
 * updates the current sp
 */
void save_n_free(Register* reg, Instructions_info* instructions_info) {
    // code to save the reg
    if (reg->reg_promise == NULL) {
      // printf("%s %d\n", reg->name, reg->occupied);
        yyerror("Ari baap re\n");
    }
    if (reg->reg_promise->ste == NULL) {
        // printf("%s : %p %d %p\n", reg->name, reg->reg_promise->ste, reg->occupied, reg);
        write_instr(instructions_info, "%-7s\t%s,\t%d($fp)\t #\tSPILL", "sw", reg->name, instructions_info->curr_size);
        reg->reg_promise->loc = instructions_info->curr_size;
        reg->reg_promise->offset = reg->offset;
        instructions_info->curr_size += 4;
    }
    // printf("Freeeing %s\n", reg->name);
    __free_reg(reg);
}


/**
 * frees the registers occupied by locals of symbol table
 */
int free_reg_symt(SymbolTable* symt) {
    HashMap *table = symt->local_table;
    lli *table_keys = keys(table);

    for (int i = 0; i < table->len; i++)
    {
        STEntry *ste = (STEntry *)get(table, table_keys[i]);
        // Register* reg = ste->reg;
        // if (reg){
        //     reg->ste = NULL;
        // }
        // ste->reg = NULL;   
    }
    
    free(table_keys);
    return 0;
}

/**
 * internal function
 * returns a free register... 
 * if occupied is true, then can save and return an occupied register
 */
Register* __get_free_register(Register* regs[], Instructions_info* instructions_info, int start, int end, int occupied) {
    Register* reg = NULL;
    for (int i = start ;i < end; i ++) {
        if (registers[i]->occupied == 0) {
            reg = registers[i];
            break;
        }
    }
    if (reg != NULL) {
        return reg;
    }
    // all are occupied
    int len = register_usage_queue->len;
    int gotit = 0;
    while (len-- > 0) {
        Register* curr_reg = (Register*)dequeue_queue(register_usage_queue);
        if (gotit == 0 && curr_reg->reg_promise->ste != NULL || occupied == 1) { 
            reg = curr_reg;
            gotit = 1;
            save_n_free(reg, instructions_info);
        } else {
            enqueue_queue(register_usage_queue,(lli)curr_reg);
        }
    }
    return reg;
}


// Register* get_free_caller_saved_register(Register* exempt_reg, int check) {
//     return __get_free_register(registers, exempt_reg, check, 16, 23);    
// }

// Register* get_free_caller_saved_fregister(Register* exempt_reg, int check) {
//     return __get_free_register(fregisters, exempt_reg, check, 17, 28);    
// }

RegPromise* init_reg_promise(Register* reg) {
    RegPromise* reg_promise = (RegPromise*)(malloc(sizeof(RegPromise)));
    reg_promise->loc = NO_ENTRY;
    reg_promise->reg = reg;
    if (reg != NULL) {
        reg->reg_promise = reg_promise;
    }
    reg_promise->ste = NULL;
    reg_promise->label = NULL;
    reg_promise->immediate = NULL;
    return reg_promise;
}

void check_n_add_extra(Instructions_info* instr_info, Register* reg) {
    for (int i = 0; i < instr_info->extra_reg_count; i ++) {
        if (reg == instr_info->extra_regs[i]) {
            return;
        }
    }
    instr_info->extra_regs[instr_info->extra_reg_count ++] = reg;
}

/**
 * Frees if required and return the same register
 */
RegPromise* get_specific_register_promise(Instructions_info* instructions_info, int pos) {
    Register* reg = __get_free_register(registers, instructions_info, pos , pos + 1, 1);
    if (pos >= 16 && pos < 23) {
        check_n_add_extra(instructions_info, reg);
    }
    enqueue_queue(register_usage_queue, (lli)reg);
    reg->occupied = 1;
    RegPromise* reg_promise =  init_reg_promise(reg);
    return reg_promise;
}

/**
 * returns a free register... 
 * if none is free,
    * check == 0 ,  returns any but the given exempt register
    * check == 1 ,  returns NULL
    * 
 * first checks for empty registers or one which are linked to an ste(you can always load them back :) )
 * if still none, that is all the registers are occupied and holding an expression value(we cannot discard them :) )
 * then saves any one of them to the stack and returns that register
 * it marks the register occupied so its responsibility of client to free this.
 */
RegPromise* get_free_register_promise(Instructions_info* instructions_info) {
    Register* reg = __get_free_register(registers, instructions_info, 0 , 16, 0);
    if (reg == NULL) {
        reg = __get_free_register(registers, instructions_info, 16 , 23, 0);
        check_n_add_extra(instructions_info, reg);
    }
    if (reg == NULL) {
        reg = __get_free_register(registers, instructions_info, 0, 16, 1);
    }
    if (reg == NULL) {
        reg = __get_free_register(registers, instructions_info, 16, 23, 1);
        check_n_add_extra(instructions_info, reg);
    }
    enqueue_queue(register_usage_queue, (lli)reg);
    reg->occupied = 1;
    RegPromise* reg_promise =  init_reg_promise(reg);
    return reg_promise;
}

/**
 * returns a free fregister... 
 * Search is in following order
 * t0 - 9 : callee saved
 * s0 - 7 : caller saved (if caller_regs == 1)
 * if none is free,
    * check == 0 ,  returns any but the given exempt fregister
    * check == 1 ,  returns NULL
    * 
 * first checks for empty registers or one which are linked to an ste(you can always load them back :) )
 * if still none, that is all the registers are occupied and holding an expression value(we cannot discard them :) )
 * then saves any one of them to the stack and returns that fregister
 */
// Register* get_free_fregister(Register* exempt_reg, Instructions_info* instructions_info) {
//     Register* reg = __get_free_register(fregisters, exempt_reg, instructions_info, 1 , 23, 0);
//     if (reg != NULL) {
//         return reg;
//     }
//     reg = __get_free_register(fregisters, exempt_reg, instructions_info, 23 , 32, 0);
//     if (reg != NULL) {
//         return reg;
//     }
//     reg = __get_free_register(fregisters, exempt_reg, instructions_info, 1, 23, 1);
//     if (reg != NULL) {
//         return reg;
//     }
//     reg = __get_free_register(fregisters, exempt_reg, instructions_info, 23, 32, 1);
//     return reg;
// }

/**
 * gets a register and writes instructions to load it into the instruction_info
 * returns the register used
 */
RegPromise* get_ste(Node* node, SymbolTable* symt, Instructions_info* instructions_info) {
    if (node == NULL || node->n_type != t_STE) {
        fprintf(stderr, "Wrong node received for STE\n");
        return NULL;
    }
    // printf("STE\n");
    STEntry* ste = (STEntry*) (node->val);
    // code to return the variables value directly from register if already stored
    // printf("%s\n", ste->name);
    //

    RegPromise* pos_reg = NULL;

    if (ste->is_array) {
        // Nodes are organised in reverse manner
        node = node->first_child;
        lli pos= 0;
        DARRAY *arrval = ste->value.arrval;
        int depth = arrval->arr_depth;
        lli len = 1;
        // assuming arr[depth] have length of first
        for (int j = depth; j > 0; j -- ) {
            RegPromise* expr_reg = get_expression(node->first_child, symt, instructions_info);
            RegPromise* len_reg = get_free_register_promise(instructions_info);
            addiu(instructions_info, len_reg, zero_reg_promise, len);
            mult(instructions_info, len_reg, expr_reg);
            mflo(instructions_info,len_reg);
            __free_regpromise(expr_reg);
            if (pos_reg == NULL) {
                pos_reg = get_free_register_promise(instructions_info);
                addu(instructions_info, pos_reg, zero_reg_promise, zero_reg_promise); // make its value zero
            }
            addu(instructions_info, pos_reg, pos_reg, len_reg);
            __free_regpromise(len_reg);
            len *= arrval->arr_lengths[j];
            node = node->next;
        }
        int dsize = 0;
        switch (ste->dtype) {
            case DOUBLE:
            dsize = 4;
            break;
            case BOOL:
            dsize = 1;
            break;
            case INT:
            dsize = 4;
            break;
            default:
            return NULL;
        }
        sll(instructions_info, pos_reg, pos_reg, log2(dsize));
        ste->ref_reg->reg->offset = NO_ENTRY;
        addu(instructions_info, pos_reg, pos_reg, ste->ref_reg);
        // got the address in pos_reg
      // printf("%s:%d\n",ste->name, ste->loc_from_ref_reg);
        pos_reg->reg->offset = ste->loc_from_ref_reg;
    } else {
        // then just attach and return the ste
        pos_reg = init_reg_promise(NULL);
        // TODO: to add the type of register that should be given back as promise
        // when handling doubles
        pos_reg->ste = ste;
        // printf("STE: %s\n",ste->name);
        enqueue_queue(ste_register_promises, (lli)pos_reg);
    }
    
    
    return pos_reg;
}


RegPromise* handle_increament(Instructions_info* instructions_info, RegPromise* reg1, int op) {
    RegPromise* final_reg, *temp_reg, * new_reg;
    switch(op) {
        case t_PLUSPLUS_PRE:
            if (reg1 == NULL) {
                yyerror("no operand for evaluation\n");
            }
            // reg1 is ste
            new_reg = get_free_register_promise(instructions_info);
            if (reg1->ste != NULL) {
                STEntry* ste = reg1->ste;
                __free_regpromise(reg1);
                reg1 = ste->ref_reg;
                reg1->reg->offset = ste->loc_from_ref_reg;
            }
            loadw(instructions_info, new_reg, reg1); // ld t = (var)
            addiu(instructions_info, new_reg, new_reg, 1); // addi t ,t , 1
            storew(instructions_info, new_reg, reg1); // st t , (var)
            // return (var)
            // if reg1 was an expression(in case of array) the new_reg will have nul as ste
            new_reg->ste = reg1->ste;
            __free_regpromise(reg1);
            final_reg = new_reg;
            break;
        case t_PLUSPLUS_POST:
            if (reg1 == NULL) {
                yyerror("no operand for evaluation\n");
            }
            // reg1 is ste
            if (reg1->ste != NULL) {
                STEntry* ste = reg1->ste;
                __free_regpromise(reg1);
                reg1 = ste->ref_reg;
                reg1->reg->offset = ste->loc_from_ref_reg;
            }
            temp_reg = get_free_register_promise(instructions_info);
            new_reg = get_free_register_promise(instructions_info);
            loadw(instructions_info, new_reg, reg1); // ld t , (var)
            addiu(instructions_info, temp_reg, new_reg, 1); // add t1 , t, 1
            storew(instructions_info, temp_reg, reg1); // st t1, (var)
            // return t
            __free_regpromise(reg1);
            __free_regpromise(temp_reg);
            final_reg = new_reg;
            break;
        case t_MINUSMINUS_PRE:
            if (reg1 == NULL) {
                yyerror("no operand for evaluation\n");
            }
            // reg1 is ste
            new_reg = get_free_register_promise(instructions_info);
            if (reg1->ste != NULL) {
                STEntry* ste = reg1->ste;
                __free_regpromise(reg1);
                reg1 = ste->ref_reg;
                reg1->reg->offset = ste->loc_from_ref_reg;
            }
            loadw(instructions_info, new_reg, reg1); // ld t , (var)
            subiu(instructions_info, new_reg, new_reg, 1); // sub t, t, 1
            storew(instructions_info, new_reg, reg1); // sd t , (var)
            new_reg->ste = reg1->ste;
            __free_regpromise(reg1);
            final_reg = new_reg;
            break;
        case t_MINUSMINUS_POST:
            if (reg1 == NULL) {
                yyerror("no operand for evaluation\n");
            }
            // reg1 is ste
            if (reg1->ste != NULL) {
                STEntry* ste = reg1->ste;
                __free_regpromise(reg1);
                reg1 = ste->ref_reg;
                reg1->reg->offset = ste->loc_from_ref_reg;
            }
            temp_reg = get_free_register_promise(instructions_info);
            new_reg = get_free_register_promise(instructions_info);
            loadw(instructions_info, new_reg, reg1); // ld t , (var)
            subiu(instructions_info, temp_reg, new_reg, 1); // add t1 , t, 1
            storew(instructions_info, temp_reg, reg1); // st t1, (var)
            // return t
            __free_regpromise(reg1);
            __free_regpromise(temp_reg);
            final_reg = new_reg;
            break;
        default:
        break;
    }
    return final_reg;
}


RegPromise* get_expression(Node* node, SymbolTable* symt, Instructions_info* instructions_info) {
    
    if (node == NULL)
        return NULL;

    RegPromise* reg1 = NULL,* reg2 = NULL;
    Node* left_child = node->first_child;
    Node* right_child = NULL;
    if (left_child) {
        right_child = left_child->next;
    }
    // printf("Expr %p %p %d\n", left_child, right_child, node->n_type);
    int expr = NO_ENTRY;
    if (node->n_type != t_STE && node->n_type != t_FUNC_CALL) {
        if (left_child) {
            reg1 = get_expression(left_child, symt, instructions_info); // left child
            if (right_child) {
                reg2 = get_expression(right_child, symt, instructions_info); // right child
            }
        }
    }

    // printTreeIdent(node);
    // fflush(debug);

    lli* addr;
    int dtype = -1;
    RegPromise* final_reg = NULL;
    RegPromise* new_reg, * temp_reg;
    // printf("Evaluating %d\n", node->n_type);
    switch (node->n_type)
    {
    case t_STR:
        // printf("STR\n");
    final_reg = init_reg_promise(NULL);
    final_reg->label = (String*)node->val;
    break;
    case t_BOOLEAN:
    case t_NUM_I:
    // printf("I_NUM_F\n");
    final_reg = init_reg_promise(NULL);
    final_reg->immediate = (lli*)node->val;
    break;
    case t_NUM_F:
        break;
    case t_STE:
        final_reg = get_ste(node, symt, instructions_info);
        break;
    case t_OP:
        if (reg1 == NULL || (((char)(node->val) != '!') && reg2 == NULL)) {
            yyerror("Operand not recieved..\n" );
        }
        switch ((char)(node->val))
        {
        case '+':
            // both have registers
            reload_reg_withoffset(reg1,instructions_info);
            reload_reg_withoffset(reg2,instructions_info);
            addu(instructions_info, reg2, reg1,reg2);
            __free_regpromise(reg1);
            final_reg = reg2;
            
            clear_offset(final_reg);
            break;
        case '-':
            reload_reg_withoffset(reg1,instructions_info);
            reload_reg_withoffset(reg2,instructions_info);
            subu(instructions_info, reg2, reg1,reg2);
            __free_regpromise(reg1);
            final_reg = reg2;
            clear_offset(final_reg);
            break;
            case '*':
            reload_reg_withoffset(reg1,instructions_info);
            reload_reg_withoffset(reg2,instructions_info);
            mult(instructions_info, reg2, reg1);
            clear_offset(reg2);
            mflo(instructions_info, reg2);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
            case '/':
            reload_reg_withoffset(reg1,instructions_info);
            reload_reg_withoffset(reg2,instructions_info);
            mips_div(instructions_info, reg1, reg2);
            clear_offset(reg2);
            mflo(instructions_info, reg2);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '%':
            if (reg1->immediate && reg2->immediate) {
                *reg2->immediate = (lli)(*(double*)reg1->immediate) % (lli)(*(double*)reg2->immediate);
            } else {
                reload_reg_withoffset(reg1,instructions_info);
                reload_reg_withoffset(reg2,instructions_info);
                mips_div(instructions_info, reg1, reg2);
                clear_offset(reg2);
                // printf("MOD: %p %p\n", reg2->immediate, reg2->reg);
                mfhi(instructions_info, reg2);
            }
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '&':
            reload_reg_withoffset(reg1,instructions_info);
            reload_reg_withoffset(reg2,instructions_info);
            and(instructions_info, reg2, reg2, reg1);
            __free_regpromise(reg1);
            final_reg = reg2;
            clear_offset(final_reg);
            break;
        case '|':
            reload_reg_withoffset(reg1,instructions_info);
            reload_reg_withoffset(reg2,instructions_info);
            or(instructions_info, reg2, reg2, reg1);
            __free_regpromise(reg1);
            final_reg = reg2;
            clear_offset(final_reg);
            break;
        case '<':
            reload_reg_withoffset(reg1,instructions_info);
            reload_reg_withoffset(reg2,instructions_info);
            slt(instructions_info, reg2, reg1,reg2);
            __free_regpromise(reg1);
            final_reg = reg2;
            clear_offset(final_reg);
            break;
        case '>':
            reload_reg_withoffset(reg1,instructions_info);
            reload_reg_withoffset(reg2,instructions_info);
            slt(instructions_info, reg2, reg2,reg1);
            __free_regpromise(reg1);
            final_reg = reg2;
            clear_offset(final_reg);
            break;
        case '!':
            reload_reg_withoffset(reg1,instructions_info);
            reload_reg_withoffset(reg2,instructions_info);
            nor(instructions_info, reg1, reg1, zero_reg_promise);
            __free_regpromise(reg1);
            final_reg = reg2;
            clear_offset(final_reg);
            break;
        default: break;
        }
        break;

    case t_FUNC_CALL:
        handle_function_call(node, symt, instructions_info);
        final_reg = get_specific_register_promise(instructions_info,14);
    break;
    case t_EE:
        reload_reg_withoffset(reg1,instructions_info);
        reload_reg_withoffset(reg2,instructions_info);
        xor(instructions_info, reg2, reg1,reg2);
        sltiu(instructions_info, reg2, reg2, 1ll); // $t0 = 1ll if $t0 == 0 ((unsigned)t0 < 1ll) (equal), 0 otherwise
        __free_regpromise(reg1);
        final_reg = reg2;
        clear_offset(final_reg);
        break;
    case t_NE:
        reload_reg_withoffset(reg1,instructions_info);
        reload_reg_withoffset(reg2,instructions_info);
        xor(instructions_info, reg2, reg1,reg2);
        sltiu(instructions_info, reg2, reg2, 1ll); // $t0 = 1ll if $t0 == 0 ((unsigned)t0 < 1ll) (equal), 0 otherwise
        xori(instructions_info, reg2, reg2, 1ll); // reverse the bit
        __free_regpromise(reg1);
        final_reg = reg2;
        clear_offset(final_reg);
        break;
    case t_GTE:
        reload_reg_withoffset(reg1,instructions_info);
        reload_reg_withoffset(reg2,instructions_info);
        slt(instructions_info, reg2, reg1,reg2);
        xori(instructions_info, reg2, reg2, 1ll); // reverse the result, if t0 == 0 (t1 >= t2) then 1ll, else 0
        __free_regpromise(reg1);
        final_reg = reg2;
        clear_offset(final_reg);
        break;
    case t_LTE:
        reload_reg_withoffset(reg1,instructions_info);
        reload_reg_withoffset(reg2,instructions_info);
        slt(instructions_info, reg2, reg2,reg1);
        xori(instructions_info, reg2, reg2, 1ll); // reverse the result, if t0 == 0 (t2 >= t1) then 1, else 0
        __free_regpromise(reg1);
        final_reg = reg2;
        clear_offset(final_reg);
        break;
    case t_MINUS:
        if (reg1 == NULL) {
            yyerror("no operand for evaluation\n");
        }
        reload_reg_withoffset(reg2,instructions_info);
        subu(instructions_info, reg1, zero_reg_promise, reg1);
        final_reg = reg1;
        clear_offset(final_reg);
        break;
    default: 
        final_reg = handle_increament(instructions_info,reg1, node->n_type);
    break;
    }
    // fprintf(yyout,"EXPR: %f\n", *val);
    // printf("final_reg\n");

    return final_reg;
   
}

int handle_assignment(Node* node, SymbolTable* symt, Instructions_info* instructions_info) {
    if (node == NULL || node->n_type != t_ASSIGN) {
        fprintf(stderr, "Wrong node received for Assignment\n");
        return 1;
    }
    // printf("assign\n");
    // fflush(debug);
    Node *var_node = node->first_child;
    Node *expr_node = var_node->next;
    // printTreeIdent(expr_node);
    RegPromise* expr_promise = get_expression(expr_node->first_child, symt, instructions_info);
    // printf("Expr %p\n", expr_promise->ste);
    // This is important in case it uses the reference register then ste ref offset will be overriden
    if (expr_promise->immediate != NULL) {
        // this is expression not a register
        RegPromise* new_reg = get_free_register_promise(instructions_info);
        // printf("pp %lld\n", (lli)*((double*)(expr_promise->immediate)));
        addiu(instructions_info, new_reg, zero_reg_promise, (lli)*((double*)(expr_promise->immediate)));
        // printf("pp\n");
        __free_regpromise(expr_promise);
        expr_promise = new_reg;
    } else {
        reload_reg_withoffset(expr_promise,instructions_info);
    }
    // this promise definite has a register or immediate
    if (expr_promise == NULL) {
        yyerror("No expression received\n");
    }
    RegPromise* ste_promise = get_ste(var_node, symt, instructions_info);
    int free_ste_promise = 1;
    if (ste_promise->ste != NULL) {
        STEntry* ste = ste_promise->ste;
        free_ste_promise = 0;
        // printf("::%s ", ste->name);
        __free_regpromise(ste_promise);
        ste_promise = ste->ref_reg;
        ste_promise->reg->offset = ste->loc_from_ref_reg;
        // printf("%s %d\n",ste_promise->reg->name,ste_promise->reg->offset);
    }
    // printf("pp\n");
    storew(instructions_info,expr_promise, ste_promise);
    // printf("Finished\n");
    // if not further used
    __free_regpromise(ste_promise);
    __free_regpromise(expr_promise);

    return 0;

}

/**
 * stores an argument on others stack and returns amount of space used
 */
int store_argument(Instructions_info* instr_info, RegPromise* reg, int start, int store_addr, int count) {
    // printf("%p %p %p %p %d %d\n", reg, reg->immediate, reg->label, reg->ste, reg->loc, reg->offset);
    if (reg->immediate == NULL) {
        if (store_addr == 1) {
            // to store addr
            if (reg->ste != NULL) {
                STEntry* ste = reg->ste;
                RegPromise* new_reg = get_free_register_promise(instr_info);
                ste->ref_reg->reg->offset = NO_ENTRY;
                addiu(instr_info, new_reg, ste->ref_reg, ste->loc_from_ref_reg);
                assign_reg_promise(reg, new_reg);
                free(new_reg);
            } else {
                reload_reg(reg, instr_info); // in case this got spilled
                
                if (reg->label || reg->reg == NULL) {
                    printf("%p\n", reg->reg);
                    yyerror("Cannot get address variable/reigister as address\n");
                } else if (reg->reg->offset != NO_ENTRY) {
                    addiu(instr_info, reg, reg, reg->reg->offset);
                }
            } 
            // all good
        } else {
            // offset is not null
            reload_reg_withoffset(reg, instr_info);
        }
    } else {
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info, new_reg, zero_reg_promise, (lli)*((double*)(reg->immediate)));
        assign_reg_promise(reg, new_reg);
        free(new_reg);
    }
    sp_reg_promise->reg->offset = start;
    storew(instr_info, reg, sp_reg_promise);
    if (count > 0) {
        sp_reg_promise->reg->offset = (start);
        // till now all args and temps have been saved
        RegPromise* arg_promise = get_specific_register_promise(instr_info,count);
        loadw(instr_info,arg_promise,sp_reg_promise);
        __free_regpromise(arg_promise);
    }
    return start + 4;
}

/**
 * assumes sp had been extended.
 * store_addr : 1 (in case of read)
 */
int store_arguments(Instructions_info* instr_info, Queue* param_queue, int start, int count, int store_addr) {
    RegPromise* reg;
    int arg = start;
    while ((lli)(reg = (RegPromise*)dequeue_queue(param_queue)) != LLONG_MIN && reg != NULL) {
        arg = store_argument(instr_info, reg, arg, store_addr, count);
        if (count >= 10 && count < 14) {
            count ++;
        } else {
            count = -1;
        }
        __free_regpromise(reg);
    }
    return arg;
}

void create_formatting_string(Instructions_info* instr_info) {
    RegPromise* a0_reg_promise = get_specific_register_promise(instr_info,10);
    addiu(instr_info, a0_reg_promise, zero_reg_promise, 256);
    write_instr(instr_info, "lui	$gp,%%hi(__gnu_local_gp)\n \
	addiu	$gp,$gp,%%lo(__gnu_local_gp)");
    
    write_instr(instr_info, "lw	$v0,%%call16(malloc)($gp)");
    write_instr(instr_info, "move\t$t9,\t$v0");
    write_instr(instr_info, "jalr\t$t9\n\tnop");
    RegPromise* v0_reg_promise = get_specific_register_promise(instr_info,14);
    move(instr_info, a0_reg_promise, v0_reg_promise);
    __free_regpromise(v0_reg_promise);
    __free_regpromise(a0_reg_promise);
}

void free_formatting_string(Instructions_info* instr_info) {
    RegPromise* a0_reg_promise = get_specific_register_promise(instr_info,10);
    sp_reg_promise->reg->offset = 0;
    loadw(instr_info, a0_reg_promise, sp_reg_promise);
    __free_regpromise(a0_reg_promise);
    write_instr(instr_info, "lui	$gp,%%hi(__gnu_local_gp)\n \
	addiu	$gp,$gp,%%lo(__gnu_local_gp)");
    
    write_instr(instr_info, "lw	$v0,%%call16(free)($gp)");
    write_instr(instr_info, "move\t$t9,\t$v0");
    write_instr(instr_info, "jalr\t$t9\n\tnop");
    return;
}


void call_memset(Instructions_info* instr_info, RegPromise* ref, RegPromise* filler, RegPromise* size) {
    int start = store_argument(instr_info,ref, 0, 0, 10);
    start = store_argument(instr_info,filler, start, 0, 11);
    start = store_argument(instr_info,size, start, 0, 12);
    write_instr(instr_info, "lui	$gp,%%hi(__gnu_local_gp)\n \
	addiu	$gp,$gp,%%lo(__gnu_local_gp)");
    
    write_instr(instr_info, "lw	$v0,%%call16(memset)($gp)");
    write_instr(instr_info, "move\t$t9,\t$v0");
    write_instr(instr_info, "jalr\t$t9\n\tnop");
}

void scaffold_for_read_write(Node* node, SymbolTable* symt, Instructions_info* instr_info, int read) {

    Node* params = node->first_child->first_child;
    // printTreeIdent(params);fflush(debug);
    Queue* param_queue = init_queue();
    while (params != NULL) {
        RegPromise* reg = get_expression(params->first_child, symt, instr_info);
        if (read == 0) {
            // printf("%d", (reg->label == NULL));
            reload_reg(reg,instr_info);
        } else {
            
        }
        enqueue_queue(param_queue, (lli)reg);
        params = params->next;
    }
    // save temp and arg registers registers
    store_arg_n_temp_registers(instr_info, read);
    // printf("reg%d\n", instr_info->gen_code);
    // to create the formatting string
    create_formatting_string(instr_info);
    int dist = 0;
    int len = param_queue->len;
    params = node->first_child->first_child;
    while (len -- > 0) {
        write_instr(instr_info, "%-7s\t$a1,\t$0,%d","addiu", '%');
        write_instr(instr_info, "%-7s\t$a1,\t%d($a0)","sb", dist);
        dist ++;
        if (params->first_child->n_type == t_STR) {
            write_instr(instr_info, "%-7s\t$a1,\t$0,%d","addiu", 's');
            write_instr(instr_info, "%-7s\t$a1,\t%d($a0)","sb", dist);
            dist ++;
        } else {

            // TODO: check data type and add formatting appropiately
            // RegPromise* arg_reg = regvals[i];
            // TODO: assume we get some data type here and work with it
            // adding for INT
            
            write_instr(instr_info, "%-7s\t$a1,\t$0,%d","addiu", 'd');
            write_instr(instr_info, "%-7s\t$a1,\t%d($a0)","sb", dist);
            dist ++;
        }
        if (len  != 0) {
            write_instr(instr_info, "%-7s\t$a1,\t$0,%d","addiu", ' ');
            write_instr(instr_info, "%-7s\t$a1,\t%d($a0)","sb", dist);
            dist ++;
        }
        if (dist >= 254) {
            yyerror("Too much varaibles to print in single write");
        }
        params = params->next;
    }
    if (read == 0) {
        write_instr(instr_info, "%-7s\t$a1,\t$0,%d","addu", '\n');
        write_instr(instr_info, "%-7s\t$a1,\t%d($a0)","sb", dist);
        dist ++;
    }
    write_instr(instr_info, "%-7s\t$a1,\t$0,$0","addu");
    write_instr(instr_info, "%-7s\t$a1,\t%d($a0)","sb", dist);
    dist ++;
    // printf("Here\n");
    RegPromise* reg = get_specific_register_promise(instr_info,10);
    int start = store_argument(instr_info,reg, 0, 0, -1);
    __free_regpromise(reg);
    start = store_arguments(instr_info, param_queue, start, 11, read);
    instr_info->call_window_size =  max(start, instr_info->call_window_size);
    free_queue(param_queue);
}

/**
 * returns the argument window size
 */
int handle_write(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL) {
        return 1;
    }
    scaffold_for_read_write(node,symt,instr_info, 0);
    // jump
    write_instr(instr_info, "lui	$gp,%%hi(__gnu_local_gp)\n \
        addiu	$gp,$gp,%%lo(__gnu_local_gp)");
        
        write_instr(instr_info, "lw	$v0,%%call16(printf)($gp)");
        write_instr(instr_info, "move\t$t9,\t$v0");
        write_instr(instr_info, "jalr\t$t9\n\tnop");
        free_formatting_string(instr_info);
}

int handle_read(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL) {
        return 1;
    }

    scaffold_for_read_write(node,symt,instr_info, 1);
    // jump
    write_instr(instr_info, "lui	$gp,%%hi(__gnu_local_gp)\n \
	addiu	$gp,$gp,%%lo(__gnu_local_gp)");
    
    write_instr(instr_info, "lw	$v0,%%call16(__isoc99_scanf)($gp)");
    write_instr(instr_info, "move\t$t9,\t$v0");
    write_instr(instr_info, "jalr\t$t9\n\tnop");
    free_formatting_string(instr_info);
}

/**
 * writes the argument window size in Instructions_info
 */
int handle_function_call(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL || node->n_type != t_FUNC_CALL) {
        yyerror("Wrong node received for a FUNC Call\n");
    }
    char* name = (char*)node->val;
    // printf("FUNC_CALL %s\n", name);
    if (strcmp(name,"write") == 0) {
        handle_write(node, symt, instr_info);
    } else if (strcmp(name,"read") == 0) {
        handle_read(node, symt, instr_info);
    } else {
        Node* params = node->first_child->first_child;
        Queue* param_queue = init_queue();
        while (params != NULL) {
            // printf(":::%p\n", params->first_child);
            RegPromise* reg = get_expression(params->first_child, symt, instr_info);
            if (reg == NULL) {
                yyerror("(NULL) received from expression\n");
            }
            // printf("%p\n", reg);
            reload_reg(reg,instr_info);
            enqueue_queue(param_queue, (lli)reg);
            params = params->next;
        }
        // printf("STORED\n");
        // save temp and arg registers registers
        store_arg_n_temp_registers(instr_info, 0);
        int start = store_arguments(instr_info, param_queue, 0, 10, 0);
        instr_info->call_window_size =  max(start, instr_info->call_window_size);
        free_queue(param_queue);
        RegPromise* t9_reg = get_specific_register_promise(instr_info,9);
        load_label(instr_info, t9_reg, name);
        write_instr(instr_info, "%-7s\t%s\n\tnop","jalr",t9_reg->reg->name);
        __free_regpromise(t9_reg);
    }
    // printf("END\n");
    return 0;
}

int handle_function_return(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL || node->n_type != t_RETURN) {
        yyerror("Wrong Node for return statement\n");
    }

    Node *expr_node = node->first_child;
    // printTreeIdent(expr_node);
    RegPromise* expr_promise = get_expression(expr_node->first_child, symt, instr_info);
    if (expr_promise == NULL) {
        yyerror("No expression received\n");
    }
    if (expr_promise->immediate != NULL) {
        // this is expression not a register
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info, new_reg, zero_reg_promise, (lli)*((double*)(expr_promise->immediate)));
        __free_regpromise(expr_promise);
        expr_promise = new_reg;
    } else {
        reload_reg_withoffset(expr_promise,instr_info);
    }
    if (expr_promise->reg != registers[14]) {
        RegPromise* v0_promise = get_specific_register_promise(instr_info,14); // v0
        if (expr_promise->reg->offset == NO_ENTRY) {
            move(instr_info, v0_promise, expr_promise);
        } else {
            storew(instr_info, v0_promise, expr_promise);
        }
        __free_regpromise(v0_promise);
    }
    __free_regpromise(expr_promise);
    
    write_instr(instr_info, "%-7s\t%s\n\tnop", "j", instr_info->return_label->val);
}

int handle_cond_stmt(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL || node->n_type != t_COND) {
        yyerror("Wrong Node for Condition statement\n");
    }

    Node *expr_node = node->first_child;
    Node* if_block = expr_node->next;
    Node* else_block = if_block->next;

    String* if_label = init_string(instr_info->return_label->val, -1);
    add_str(if_label, "_if", 3);
    char buffer[10];
    buffer[snprintf(buffer, 10, "%d", instr_info->label_count)] = '\0'; // RISK
    add_str(if_label, buffer, -1);

    String* end_label = init_string(instr_info->return_label->val, -1);
    add_str(end_label, "_if_end", 7);
    buffer[snprintf(buffer, 10, "%d", instr_info->label_count)] = '\0'; // RISK
    add_str(end_label, buffer, -1);

    
    RegPromise* expr_promise = get_expression(expr_node->first_child, symt, instr_info);

    if (expr_promise->immediate != NULL) {
        // this is expression not a register
        RegPromise* new_reg = get_free_register_promise(instr_info);
        // printf("pp %lld\n", (lli)*((double*)(expr_promise->immediate)));
        addiu(instr_info, new_reg, zero_reg_promise, (lli)*((double*)(expr_promise->immediate)));
        // printf("pp\n");
        __free_regpromise(expr_promise);
        expr_promise = new_reg;
    } else {
        reload_reg_withoffset(expr_promise,instr_info);
    }


    instr_info->label_count ++;
    // check and jump

    bne(instr_info, expr_promise, zero_reg_promise, if_label->val); // if not equal to zero i.e. true
    __free_regpromise(expr_promise);
    if (else_block) {
        handle_statements(else_block->first_child, symt, instr_info);
        // else label
    }
    // jump to end

    write_instr(instr_info, "%-7s\t%s\n\tnop\n", "j", end_label->val);
    
    
    __write_instr(instr_info, "%s:\n", if_label->val);
    if (if_block) {
        handle_statements(if_block->first_child, symt, instr_info);
    }
    
    // end label
    __write_instr(instr_info, "\n%s:\n", end_label->val);

    freeString(if_label);
    freeString(end_label);
}

int handle_while_stmt(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL || node->n_type != t_WHILE) {
        yyerror("Wrong Node for While statement\n");
    }

    Node *cond_node = node->first_child;
    Node* loop_block = cond_node->next;

    String* prev_loop_label = instr_info->loop_label;
    String* prev_end_label = instr_info->loop_end_label;

    String* loop_label = init_string(instr_info->return_label->val, -1);
    add_str(loop_label, "_while", 3);
    char buffer[10];
    buffer[snprintf(buffer, 10, "%d", instr_info->label_count)] = '\0'; // RISK
    add_str(loop_label, buffer, -1);

    String* end_label = init_string(instr_info->return_label->val, -1);
    add_str(end_label, "_while_end", 10);
    buffer[snprintf(buffer, 10, "%d", instr_info->label_count)] = '\0'; // RISK
    add_str(end_label, buffer, -1);

    instr_info->label_count ++;

    instr_info->loop_label = loop_label;
    instr_info->loop_end_label = end_label;

    __write_instr(instr_info, "%s:\n", loop_label->val);
    
    RegPromise* expr_promise = get_expression(cond_node->first_child, symt, instr_info);
    if (expr_promise != NULL) {
        // code to check for end
        if (expr_promise->immediate != NULL) {
            // this is expression not a register
            RegPromise* new_reg = get_free_register_promise(instr_info);
            // printf("pp %lld\n", (lli)*((double*)(expr_promise->immediate)));
            addiu(instr_info, new_reg, zero_reg_promise, (lli)*((double*)(expr_promise->immediate)));
            // printf("pp\n");
            __free_regpromise(expr_promise);
            expr_promise = new_reg;
        } else {
            reload_reg_withoffset(expr_promise,instr_info);
        }
        // if result was false . i.e. equation == 0
        beq(instr_info, expr_promise, zero_reg_promise, end_label->val);
        __free_regpromise(expr_promise);
    }
    if (loop_block) {
        handle_statements(loop_block->first_child, symt,instr_info);
    }
    
    // jump back
    write_instr(instr_info, "%-7s\t%s\n\tnop\n", "j", loop_label->val);
    
    // end label
    __write_instr(instr_info, "\n%s:\n", end_label->val);

    freeString(loop_label);
    freeString(end_label);

    instr_info->loop_label = prev_loop_label;
    instr_info->loop_end_label = prev_end_label;
}

int handle_do_while_stmt(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL || node->n_type != t_DOWHILE) {
        yyerror("Wrong Node for Do While statement\n");
    }

    String* prev_loop_label = instr_info->loop_label;
    String* prev_end_label = instr_info->loop_end_label;

    Node *cond_node = node->first_child;
    Node* loop_block = cond_node->next;

    String* loop_label = init_string(instr_info->return_label->val, -1);
    add_str(loop_label, "_dowhile", 8);
    char buffer[10];
    buffer[snprintf(buffer, 10, "%d", instr_info->label_count)] = '\0'; // RISK
    add_str(loop_label, buffer, -1);
    
    String* loop_check_label = init_string(instr_info->return_label->val, -1);
    add_str(loop_check_label, "_dowhile_check", 14);
    buffer[snprintf(buffer, 10, "%d", instr_info->label_count)] = '\0'; // RISK
    add_str(loop_check_label, buffer, -1);

    String* end_label = init_string(instr_info->return_label->val, -1);
    add_str(end_label, "_dowhile_end", 12);
    buffer[snprintf(buffer, 10, "%d", instr_info->label_count)] = '\0'; // RISK
    add_str(end_label, buffer, -1);

    instr_info->label_count ++;

    instr_info->loop_label = loop_check_label;
    instr_info->loop_end_label = end_label;

    __write_instr(instr_info, "%s:\n", loop_label->val);

    handle_statements(loop_block->first_child, symt,instr_info);
    
    __write_instr(instr_info, "%s:\n", loop_check_label->val);
    RegPromise* expr_promise = get_expression(cond_node->first_child, symt, instr_info);
    if (expr_promise != NULL) {
        // code to check for end
        if (expr_promise->immediate != NULL) {
            // this is expression not a register
            RegPromise* new_reg = get_free_register_promise(instr_info);
            // printf("pp %lld\n", (lli)*((double*)(expr_promise->immediate)));
            addiu(instr_info, new_reg, zero_reg_promise, (lli)*((double*)(expr_promise->immediate)));
            __free_regpromise(expr_promise);
            expr_promise = new_reg;
        } else {
            reload_reg_withoffset(expr_promise,instr_info);
        }
        // if result was false . i.e. equation == 0
        beq(instr_info, expr_promise, zero_reg_promise, end_label->val);
        __free_regpromise(expr_promise);
    }
    // jump back

    write_instr(instr_info, "%-7s\t%s\n\tnop\n", "j", loop_label->val);
    
    // end label
    __write_instr(instr_info, "\n%s:\n", end_label->val);

    freeString(loop_label);
    freeString(loop_check_label);
    freeString(end_label);

    instr_info->loop_label = prev_loop_label;
    instr_info->loop_end_label = prev_end_label;
}

int handle_for_stmt(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL || node->n_type != t_FOR) {
        yyerror("Wrong Node for For statement\n");
    }
    Node *assign_node = node->first_child;
    Node *cond_node = assign_node->next;
    Node* update_block = cond_node->next;
    Node* loop_block = update_block->next;

    String* loop_label = init_string(instr_info->return_label->val, -1);
    add_str(loop_label, "_for", 4);
    char buffer[10];
    buffer[snprintf(buffer, 10, "%d", instr_info->label_count)] = '\0'; // RISK
    add_str(loop_label, buffer, -1);

    String* loop_update_label = init_string(instr_info->return_label->val, -1);
    add_str(loop_update_label, "_for_update", 10);
    buffer[snprintf(buffer, 10, "%d", instr_info->label_count)] = '\0'; // RISK
    add_str(loop_update_label, buffer, -1);
    
    String* end_label = init_string(instr_info->return_label->val, -1);
    add_str(end_label, "_for_end", 7);
    buffer[snprintf(buffer, 10, "%d", instr_info->label_count)] = '\0'; // RISK
    add_str(end_label, buffer, -1);

    instr_info->label_count ++;
    String* prev_loop_label = instr_info->loop_label;
    String* prev_end_label = instr_info->loop_end_label;
    
    instr_info->loop_label = loop_update_label;
    instr_info->loop_end_label = end_label;
    
    // printf("FOR\n");
    handle_assignment(assign_node, symt,instr_info);

    __write_instr(instr_info, "%s:\n", loop_label->val);
    
    RegPromise* expr_promise = get_expression(cond_node->first_child, symt, instr_info);
    if (expr_promise != NULL) {
        // code to check for end
        if (expr_promise->immediate != NULL) {
            // this is expression not a register
            RegPromise* new_reg = get_free_register_promise(instr_info);
            // printf("pp %lld\n", (lli)*((double*)(expr_promise->immediate)));
            addiu(instr_info, new_reg, zero_reg_promise, (lli)*((double*)(expr_promise->immediate)));
            // printf("pp\n");
            __free_regpromise(expr_promise);
            expr_promise = new_reg;
        } else {
            reload_reg_withoffset(expr_promise,instr_info);
        }
        // if result was false . i.e. equation == 0
        beq(instr_info, expr_promise, zero_reg_promise, end_label->val);
        __free_regpromise(expr_promise);
    }
    // jump back
    handle_statements(loop_block->first_child, symt,instr_info);
    
    __write_instr(instr_info, "%s:\n", loop_update_label->val);
    handle_statements(update_block, symt, instr_info);

    write_instr(instr_info, "%-7s\t%s\n\tnop\n", "j", loop_label->val);

    // end label
    __write_instr(instr_info, "\n%s:\n", end_label->val);

    freeString(loop_label);
    freeString(loop_update_label);
    freeString(end_label);

    instr_info->loop_label = prev_loop_label;
    instr_info->loop_end_label = prev_end_label;
}

void handle_increament_statement(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL || node->n_type != t_INC_STMT) {
        yyerror("Wrong node received for increament Statement\n");
    }
    // this will handle the node
    RegPromise* reg1 = get_expression(node->first_child, symt, instr_info);
    __free_regpromise(reg1);
}

void handle_break(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL || node->n_type != t_BREAK) {
        yyerror("Wrong node received for Break Statement\n");
    }
    // this will handle the node
    if (instr_info->loop_label == NULL) {
        yyerror("Can't use break outside loop\n");
    }
    write_instr(instr_info, "%-7s\t%s\n\tnop\n", "j", instr_info->loop_end_label->val);
}

void handle_continue(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL || node->n_type != t_CONTINUE) {
        yyerror("Wrong node received for Continue Statement\n");
    }
    // this will handle the node
    if (instr_info->loop_label == NULL) {
        yyerror("Can't use continue outside loop\n");
    }
    write_instr(instr_info, "%-7s\t%s\n\tnop\n", "j", instr_info->loop_label->val);
}


int handle_statement(Node* node, SymbolTable* symt, Instructions_info* instructions_info) {
    if (node == NULL) {
        return 1;
    }
    // printf("Stamt %d \n", node->n_type);
    switch (node->n_type)
    {
    case t_ASSIGN:
        handle_assignment(node, symt, instructions_info);
        break;
    case t_COND:
        handle_cond_stmt(node, symt, instructions_info);
        break;
    case t_INC_STMT:
        handle_increament_statement(node,symt, instructions_info);
        break;
    case t_FOR:
        handle_for_stmt(node,symt, instructions_info);
        break;
    case t_WHILE:
        handle_while_stmt(node,symt, instructions_info);
        break;
    case t_DOWHILE:
        handle_do_while_stmt(node,symt, instructions_info);
        break;
    case t_CONTINUE:
        handle_continue(node,symt, instructions_info);
        break;
    case t_BREAK:
        handle_break(node,symt, instructions_info);
        break;
    case t_RETURN:
        handle_function_return(node,symt, instructions_info);
        break;
    case t_FUNC_CALL:
        handle_function_call(node, symt, instructions_info);
        break;
    default:
        break;
    }
}

int handle_statements(Node* node, SymbolTable* symt, Instructions_info* instructions_info) {
    if (node == NULL) {
        return 1;
    }
    while (node != NULL) {
        handle_statement(node, symt, instructions_info);
        node = node ->next;
    }
}

/**
 * gen_code : if 0, does not write anything to instructions_info and also clears the symbol table reg field
 */
int handle_func_body(Node* node, SymbolTable* symt, Instructions_info* instructions_info) {
    if (node == NULL || node->n_type != t_FUNC_BODY) {
        fprintf(stderr, "Wrong node received for function body\n");
        return 1;
    }

    // regs[(*extra_reg_counts)++] = registers[11];
    // regs[(*extra_reg_counts)++] = registers[12];
    Node* child = node->first_child;
    //
    handle_statements(child, symt, instructions_info);

    //

    // if (instructions_info->gen_code == 0) {
    //     free_reg_symt(symt);
    // }
}


char* code_prologue = "\t.file	1 \"s0.c\" \n \
\t.section .mdebug.abi32\n \
\t.previous\n \
\t.nan\tlegacy \n\
\t.module\tfp=xx \n\
\t.module\tnooddspreg \n \
\t.abicalls\n";

char* code_epilogue = "\t.ident\t\"GCC: (Ubuntu 10.5.0-4ubuntu2) 10.5.0\" \n \
\t.section\t.note.GNU-stack,\"\",@progbits\n";


int save_extra_regs(Instructions_info* instr_info, int start) {
    Register** regs= instr_info->extra_regs; int count = instr_info->extra_reg_count;
    for (int i = 0; i < count;i ++) {
        Register* reg = regs[i];
        start -= 4;
        write_instr(instr_info,"%-7s\t%s,\t%d($sp)", "sw",reg->name, start);
    }
    return 0;
}

int load_extra_regs(Instructions_info* instr_info, int start) {
    Register** regs= instr_info->extra_regs; int count = instr_info->extra_reg_count;
    for (int i = 0; i < count;i ++) {
        Register* reg = regs[i];
        start -= 4;
        write_instr(instr_info,"%-7s\t%s,\t%d($sp)", "lw" ,reg->name, start);
    }
    return 0;
}


/**
 * adds specific amount of offset to each location of given symbol table
 */
int add_to_locals(SymbolTable* symt, int count) {
    HashMap *table = symt->local_table;
    lli *table_keys = keys(table);

    if (count % 4 != 0) {
        count = (count / 4 + 1) * 4;
    }

    for (int i = 0; i < table->len; i++)
    {
        STEntry *ste = (STEntry *)get(table, table_keys[i]);
        if (ste->var_type == ARG) {
            continue;
        }
        ste->loc_from_ref_reg += count;
    }
    
    free(table_keys);
    return 0;
}

/**
 * assign the arguments with their distance from sp. 
 * curr_sp_size
 */
int declare_arguments(Node* node,SymbolTable* symt, int curr_sp_size) {
    
    if (node == NULL || node->n_type != t_ARG_LIST) {
        yyerror("WRONG node received for argument list");
    }

    Node* child = node->first_child;
    while (child != NULL) {
        Node* vars = child->first_child;
        while(vars != NULL) {
            STEntry *ste = (STEntry*) (vars->val);
            int dsize = 0;
            switch (ste->dtype) {
                case DOUBLE:
                dsize = 4;
                break;
                case BOOL:
                dsize = 1;
                break;
                case INT:
                dsize = 4;
                break;
                default:
                return -1;
            }

            lli len = 1;
            if (ste->is_array != 0) {
                // not allowed
                fprintf(stderr,"arrays cannot be passed inside function args\n");
                exit(1);
            }
            len *= dsize;
            ste->ref_reg = sp_reg_promise;
            ste->loc_from_ref_reg = curr_sp_size;
            if (curr_sp_size % 4 != 0) {
                int next_block = (curr_sp_size / 4 + 1) * 4;
                if (curr_sp_size + len > next_block) {
                    curr_sp_size = next_block;
                }
            }
            curr_sp_size += len;
            vars = vars->next;
        }
        child = child ->next;
    }
}

/**
 * start : how much below have we come from fp.
 */
int declare_variables(Node* node,SymbolTable* symt, RegPromise* reference_reg,int start) {
    if (reference_reg != gp_reg_promise && reference_reg != sp_reg_promise && reference_reg != fp_reg_promise && reference_reg != s0_reg_promise) {
        yyerror("Invalid reference register");
    }

    if (node == NULL || node->n_type != t_DECL) {
        yyerror("WRONG node received for Declaration");
    }
    int size = start;

    Node* child = node->first_child;
    while (child != NULL) {
        Node* vars = child->first_child;
        while(vars != NULL) {
            STEntry *ste = (STEntry*) (vars->val);
            int dsize = 0;
            switch (ste->dtype) {
                case DOUBLE:
                dsize = 4;
                break;
                case BOOL:
                dsize = 1;
                break;
                case INT:
                dsize = 4;
                break;
                default:
                return -1;
            }

            lli len = 1;
            if (ste->is_array != 0) {
                lli pos= 0;
                DARRAY *arrval = ste->value.arrval;
                int depth = arrval->arr_depth;
                for (int j = depth; j > 0; j--) {
                    len *= arrval->arr_lengths[j];
                }
            }
            len *= dsize;
            ste->ref_reg = reference_reg;
            ste->loc_from_ref_reg = (size);
            if (size % 4 != 0) {
                int next_block = (size / 4 + 1) * 4;
                if (size + len > next_block) {
                    size = next_block;
                }
            }
            size += len;
            vars = vars->next;
        }
        child = child ->next;
    }
    if (size % 4 != 0) {
        size = (size / 4 + 1) * 4;
    }
    return size;
}

void print_symbol_table_out(SymbolTable* symt) {
    HashMap *table = symt->local_table;
    lli *table_keys = keys(table);

    for (int i = 0; i < table->len; i++)
    {
        STEntry *ste = (STEntry *)get(table, table_keys[i]);
        instr_out("#\t%s:\t%d(%s)", ste->name, ste->loc_from_ref_reg, ste->ref_reg->reg->name);
    }
    
    free(table_keys);
}


int handle_function_definitions(Node* node) {
    if (node == NULL || node->n_type != t_FUNC_DEF) {
        fprintf(stderr, "Wrong node received for function declaration\n");
        return 1;
    }
    SymbolTable* symt = (SymbolTable*) (node->val);

    RegPromise* ref_reg_promise = fp_reg_promise;

    if (strcmp(symt->name, "main") == 0) {
        ref_reg_promise = s0_reg_promise;
    } 

    String* return_label = init_string("$", 1);
    add_str(return_label, symt->name, -1);
    insert(labels, (lli)return_label->val, (lli)return_label->val);

    
    int sp_size = 0;
    Node* func_decl = node->first_child->next->next;
    int var_space = declare_variables(func_decl, symt, ref_reg_promise, 0);
    
    Node* func_body = node -> first_child -> next -> next -> next;
    Instructions_info instructions_info;
    instructions_info.instructions_stack = init_stack();
    push_stack(instructions_info.instructions_stack,(lli)init_string("", 0));
    
    // first pass
    instructions_info.gen_code = 0;
    instructions_info.extra_reg_count = 0;
    instructions_info.label_count = 0;
    instructions_info.call_window_size = 0;
    instructions_info.return_label = return_label;
    instructions_info.curr_size = var_space;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = ra_reg;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = fp_reg;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = s0_reg;

    Node* func_arg_list = node->first_child->next;
    declare_arguments(func_arg_list, symt, 0);
    
    handle_func_body(func_body, symt, &instructions_info);

    int extra_fn_argument_space= instructions_info.call_window_size; // max(arguments to be passed to any function)
    if (extra_fn_argument_space != 0) {
        extra_fn_argument_space = max(16, extra_fn_argument_space); // atleast 4 arg space must be available
    }
    int extra_space = instructions_info.curr_size - var_space; // extra_space for register spill fill
    
    int extra_reg_count = instructions_info.extra_reg_count;
    int extra_reg_space = (4 * extra_reg_count);
    // in decreasing order of address -- >
    // | sp/fp |  extra_fn_argument_space  | local_registers(var_space) | extra_space | extra_reg_space
    add_to_locals(symt,extra_fn_argument_space); // extra_reg(callee saved one are stored first)
    
  // printf("%d %d %d %d\n", extra_fn_argument_space, var_space, extra_space,extra_reg_space);
    // prologue
    sp_size = extra_fn_argument_space + var_space + extra_space + extra_reg_space; // full sp needed (utilized and saved also in same order)
    
    instructions_info.gen_code = 1;
    declare_arguments(func_arg_list, symt, sp_size);

    // prologue compiler directive
    instr_out_no_tab("#############%s##############\n", symt->name);
    instr_out(".align\t2\n\t.globl\t%s",symt->name);
    instr_out(".ent\t%s\n\t.type\t%s,\t@function",symt->name, symt->name);
    print_symbol_table_out(symt);
    instr_out_no_tab("%s:", symt->name);

    sp_reg_promise->reg->offset = NO_ENTRY;
    addiu(&instructions_info, sp_reg_promise, sp_reg_promise, -sp_size);

    save_extra_regs(&instructions_info, sp_size);
    
    // according to calling convention arguments are already saved above sp

    move(&instructions_info, fp_reg_promise, sp_reg_promise);
    if (strcmp(symt->name, "main") == 0) {
        move(&instructions_info,s0_reg_promise,fp_reg_promise);
    }
    write_instr(&instructions_info,"\n");

    if (strcmp(symt->name, "main") == 0) {
        RegPromise* ref_reg = get_free_register_promise(&instructions_info);
        addiu(&instructions_info, ref_reg, fp_reg_promise, extra_fn_argument_space);
        RegPromise* filler_reg = init_reg_promise(NULL);
        double zero = 0.0;
        filler_reg->immediate = (lli*)&zero;
        RegPromise* size_reg = init_reg_promise(NULL);
        double size = sp_size - extra_space - extra_reg_space - extra_fn_argument_space;
        size_reg->immediate = (lli*)&size;
        call_memset(&instructions_info, ref_reg, filler_reg, size_reg);
        // printf("%p\n",filler_reg->reg);
        __free_regpromise(size_reg);
        __free_regpromise(filler_reg);
        __free_regpromise(ref_reg);
    } 
    
    // body
    // writes the instructions_info this time
    instructions_info.extra_reg_count = 0;
    instructions_info.label_count = 0;
    instructions_info.call_window_size = 0;
    instructions_info.curr_size = extra_fn_argument_space + var_space;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = ra_reg;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = fp_reg;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = s0_reg;
  // printf("This is all we got\n");
    handle_func_body(func_body, symt, &instructions_info);

  // printf("%d %d %d %d\n", extra_fn_argument_space, var_space, extra_space,extra_reg_space);

    // epilogue
    String* instrs = (String*)top_stack(instructions_info.instructions_stack);
    write_instr(&instructions_info,"\n");
    write_instr_val(instrs,0, "%s:", return_label->val);
    move(&instructions_info, sp_reg_promise, fp_reg_promise);
    load_extra_regs(&instructions_info, sp_size);
    addiu(&instructions_info, sp_reg_promise, sp_reg_promise, sp_size);
    
    instrs = (String*)pop_stack(instructions_info.instructions_stack);
    instr_out_no_tab("%s", instrs->val);

    freeString(return_label);
    freeString(instrs);
    free_stack(instructions_info.instructions_stack);

    instr_out("%-7s\t$31", "jr");
    instr_out("nop\n");
    
    // epilogue compiler directive
    instr_out(".end\t%s\n\t.size\t%s,\t.-%s\n\n",symt->name, symt->name, symt->name);
    instr_out_no_tab("##########################\n");

    return 0;
}

int compile(Node *node)
{
    if (node == NULL || node->n_type != t_PROG)
    {
        return 0;
    }
    initialize_registers();
    
    fprintf(yyout, "%s",code_prologue);
    fprintf(yyout,"\t.text\n\n\n");
    Node* child = node->first_child;
    while (child != NULL) {
        handle_function_definitions(child);
        fflush(debug);
        child = child->next;
    }

    free_registers_struct();
    fprintf(yyout, "%s",code_epilogue);
    fflush(yyout);
}   