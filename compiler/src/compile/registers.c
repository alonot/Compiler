#include "../../include/includes.h"


Register* zero_reg, * sp_reg, * fp_reg, *ra_reg, *gp_reg;
RegPromise* zero_reg_promise, * sp_reg_promise, * fp_reg_promise, * ra_reg_promise, * gp_reg_promise;
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
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info,new_reg, zero_reg_promise, (lli)*((double*)(reg_promise->immediate)));
        assign_reg_promise(reg_promise, new_reg);
        free(new_reg);
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
            if (reg_promise->ste->dtype != GLOBAL) {
                RegPromise* ref_reg_promise = reg_promise->ste->ref_reg;
                ref_reg_promise->reg->offset = reg_promise->ste->loc_from_ref_reg;
                __loadw(instr_info, new_reg_promise->reg, ref_reg_promise->reg); // to prevent infinite rec, using __load
            } else {
                fprintf(stderr,"Global Vaariable in reload");
                exit(0);
            }
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
        // this will not get the value from the offset 
        // if want to load then first call reload_withoffset
    }
}

/**
 * Reloads a register's value through its offset if required
 */
void reload_reg_withoffset(RegPromise* reg, Instructions_info* instr_info) {
    if (reg == NULL) return;
    reload_reg(reg,instr_info);
    if (reg->reg->offset != NO_ENTRY) {
        loadw(instr_info,reg, reg);
        // now the actual value have been loaded hence
        reg->reg->offset = NO_ENTRY;
    }
    // printf("%s %d\n", reg->reg->name, reg->reg->offset);
}


Register* init_reg(char type, int pos, int idx) {
    Register* reg = (Register*) (calloc(1, sizeof(Register)));
    char* val = (char*) calloc(4, sizeof(char));
    val[0] = '$';val[1] = type;val[2] = '0' + pos;
    reg->name  = val;
    reg->offset = NO_ENTRY;
    reg->idx = idx;
    return reg;
}

void free_reg_struct(Register* reg) {
    free(reg->name);
    free(reg);
}


void initialize_registers() {
    int pos = 0;
    for (int i = 0;i < 10; i ++ ) {
        registers[pos] = init_reg('t', i, pos);
        pos ++;
    }
    for (int i = 0;i < 4; i ++ ) {
        registers[pos] = init_reg('a', i, pos);
        pos ++;
    }
    for (int i = 0;i < 2; i ++ ) {
        registers[pos] = init_reg('v', i, pos);
        pos ++;
    }
    for (int i = 1;i < 8; i ++ ) {
        registers[pos] = init_reg('s', i, pos);
        pos ++;
    }
    registers[pos] = init_reg('s', 0, pos); // keep s0 at last so that it does not get allocated too often.
    pos ++;

    zero_reg = init_reg('0', -'0', -1);
    sp_reg = init_reg('s', 'p' - '0', -1);
    fp_reg = init_reg('f', 'p' - '0', -1);
    gp_reg = init_reg('g', 'p' - '0', -1);
    ra_reg = init_reg('r', 'a' - '0', -1);

    register_usage_queue = init_queue();
    ste_register_promises = init_queue();

    zero_reg_promise = init_reg_promise(zero_reg);
    sp_reg_promise = init_reg_promise(sp_reg);
    fp_reg_promise = init_reg_promise(fp_reg);
    gp_reg_promise = init_reg_promise(gp_reg);
    ra_reg_promise = init_reg_promise(ra_reg);

    pos = 0;
    for (int i = 0;i < 32; i ++ ) {
        fregisters[pos ++] = init_reg('f', i, i);
    }
}

void free_registers_struct() {
    int pos = 0;
    for (int i = 0;i < 24; i ++ ) {
        free_reg_struct(registers[i]);
    }
    free_reg_struct(zero_reg);
    free_reg_struct(sp_reg);
    free_reg_struct(gp_reg);
    free_reg_struct(fp_reg);
    free_reg_struct(ra_reg);
    free(zero_reg_promise);
    free(sp_reg_promise);
    free(gp_reg_promise);
    free(fp_reg_promise);
    free(ra_reg_promise);

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
    if (reg_promise == NULL || reg_promise == zero_reg_promise || reg_promise == gp_reg_promise || reg_promise == sp_reg_promise || reg_promise == fp_reg_promise ) {
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
        Register* curr = registers[i];
        if (curr->occupied == 0) {
            reg = curr;
            break;
        }
    }
    if (reg != NULL) {
        return reg;
    }
    // all are occupied
    int len = register_usage_queue->len;
    if (occupied == 1) {
        int got_value = 0;
        while (len-- > 0) {
            Register* curr_reg = (Register*)dequeue_queue(register_usage_queue);
            if (got_value == 0 && curr_reg->reg_promise->ste == NULL && // trying to prevent releasing registers which hold ste values
                curr_reg->idx >= start && curr_reg->idx < end // checking if the register is from same limit(start < end)
            ) { 
                reg = curr_reg;
                save_n_free(reg, instructions_info);
                got_value = 1;
                // break; // do not break because the ordering must again become same using enqueue
            } else {
                enqueue_queue(register_usage_queue,(lli)curr_reg);
            }
        }
        if (reg) {
            return reg;
        }
        while (len-- > 0) {
            Register* curr_reg = (Register*)dequeue_queue(register_usage_queue);
            if (got_value == 0 && curr_reg->idx >= start && curr_reg->idx < end) { // checking if the register is from same limit(start < end)
                reg = curr_reg;
                save_n_free(reg, instructions_info);
                got_value = 1;
                // break;
            } else {
                enqueue_queue(register_usage_queue,(lli)curr_reg);
            }
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
        if (reg) {
            check_n_add_extra(instructions_info, reg);
        }
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