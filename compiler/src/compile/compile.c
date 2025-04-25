
#include "../../include/includes.h"

Register* zero_reg, * sp_reg, * fp_reg, *ra_reg, *gp_reg;
RegPromise* zero_reg_promise, * sp_reg_promise, * fp_reg_promise, * ra_reg_promise, * gp_reg_promise;
Queue* register_usage_queue;

Register* registers[32];
Register* fregisters[32]; // floating point registers

void loadw(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1);
void __free_regpromise(RegPromise* reg_promise) ;


/**
 * assigns p2 to p1 . i.e. p1 = p2
 */
void assign_reg_promise(RegPromise* p1, RegPromise* p2) {
    p1->immediate = p2->immediate;
    p1->reg = p2->reg;
    p1->reg->reg_promise = p1;
    p1->loc = p2->loc;
    p1->ste = p2->ste;
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

void reload_reg(RegPromise* reg_promise, Instructions_info* instr_info) {
    if (reg_promise->immediate != NULL) {
        // immediate
        yyerror("Got immediate for reloading\n");
        return;
    }
    if (reg_promise->reg == NULL) {
        RegPromise* new_reg_promise = get_free_register_promise(instr_info);
        if (reg_promise->ste != NULL) {
            // this was an ste
            // fprintf(debug, "Trying to reload a Register Promise that holds ste\n");
            // exit(1);
            // this means this register had an ste value but 
            // printf("STE\n");
            RegPromise* ref_reg_promise = reg_promise->ste->ref_reg;
            ref_reg_promise->reg->offset = reg_promise->ste->loc_from_ref_reg;
            __loadw(instr_info, new_reg_promise->reg, fp_reg_promise->reg); // to prevent infinite rec, using __load
        } else {
            write_instr(instr_info, "#\tFILL");
            // it was an expression
            sp_reg->offset = reg_promise->loc; // load from sp
            __loadw(instr_info,new_reg_promise->reg,sp_reg);
            new_reg_promise->reg->offset = reg_promise->offset;
        }
        assign_reg_promise(reg_promise, new_reg_promise);
        free(new_reg_promise);
    } else {
        // printf("RELOAD%s %d\n", reg_promise->reg->name, reg_promise->reg->offset);
    }
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
    for (int i = 0;i < 8; i ++ ) {
        registers[pos ++] = init_reg('s', i);
    }
    zero_reg = init_reg('0', -'0');
    sp_reg = init_reg('s', 'p' - '0');
    fp_reg = init_reg('f', 'p' - '0');
    gp_reg = init_reg('g', 'p' - '0');
    ra_reg = init_reg('r', 'a' - '0');

    register_usage_queue = init_queue();

    zero_reg_promise = init_reg_promise(zero_reg);
    sp_reg_promise = init_reg_promise(sp_reg);
    fp_reg_promise = init_reg_promise(fp_reg);
    gp_reg_promise = init_reg_promise(gp_reg);
    ra_reg_promise = init_reg_promise(ra_reg);

    pos = 0;
    for (int i = 0;i < 32; i ++ ) {
        fregisters[pos ++] = init_reg('f', i);
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
    if (reg_promise == NULL || reg_promise == zero_reg_promise || reg_promise == gp_reg_promise || reg_promise == sp_reg_promise || reg_promise == fp_reg_promise) {
        return;
    }
    __free_reg(reg_promise->reg);
    free(reg_promise);
}

void store_arg_n_temp_registers(Instructions_info* instr_info) {
    Register* reg;
    while ((lli)(reg = (Register*)dequeue_queue(register_usage_queue)) != LONG_MIN && reg != NULL) {
        // printf("%s\n", reg->name);;
        if (reg->name[1] == 't' || reg->name[1] == 'a' || reg->name[1] == 'v') {
            save_n_free(reg, instr_info);
            // later will be reloaded as needed
        } else {
            enqueue_queue(register_usage_queue,(lli)reg);
        }
    }
}

/**
 * Saves the register and set it // free
 * updates the current sp
 */
void save_n_free(Register* reg, Instructions_info* instructions_info) {
    // code to save the reg
    if (reg->reg_promise == NULL) {
        printf("%s %d\n", reg->name, reg->occupied);
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
 * returns a // free register... 
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
//     return __get_free_register(registers, exempt_reg, check, 16, 24);    
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
 * returns a // free register... 
 * if none is // free,
    * check == 0 ,  returns any but the given exempt register
    * check == 1 ,  returns NULL
    * 
 * first checks for empty registers or one which are linked to an ste(you can always load them back :) )
 * if still none, that is all the registers are occupied and holding an expression value(we cannot discard them :) )
 * then saves any one of them to the stack and returns that register
 * it marks the register occupied so its responsibility of client to // free this.
 */
RegPromise* get_free_register_promise(Instructions_info* instructions_info) {
    // printf("h\n");
    Register* reg = __get_free_register(registers, instructions_info, 0 , 16, 0);
    if (reg == NULL) {
        reg = __get_free_register(registers, instructions_info, 16 , 24, 0);
        check_n_add_extra(instructions_info, reg);
    }
    if (reg == NULL) {
        reg = __get_free_register(registers, instructions_info, 0, 16, 1);
    }
    if (reg == NULL) {
        reg = __get_free_register(registers, instructions_info, 16, 24, 1);
        check_n_add_extra(instructions_info, reg);
    }
    // printf("nn\n");
    enqueue_queue(register_usage_queue, (lli)reg);
    // printf("aa\n");
    reg->occupied = 1;
    RegPromise* reg_promise =  init_reg_promise(reg);
    // printf("q\n");
    return reg_promise;
}

/**
 * returns a // free fregister... 
 * Search is in following order
 * t0 - 9 : callee saved
 * s0 - 7 : caller saved (if caller_regs == 1)
 * if none is // free,
    * check == 0 ,  returns any but the given exempt fregister
    * check == 1 ,  returns NULL
    * 
 * first checks for empty registers or one which are linked to an ste(you can always load them back :) )
 * if still none, that is all the registers are occupied and holding an expression value(we cannot discard them :) )
 * then saves any one of them to the stack and returns that fregister
 */
// Register* get_free_fregister(Register* exempt_reg, Instructions_info* instructions_info) {
//     Register* reg = __get_free_register(fregisters, exempt_reg, instructions_info, 1 , 24, 0);
//     if (reg != NULL) {
//         return reg;
//     }
//     reg = __get_free_register(fregisters, exempt_reg, instructions_info, 24 , 32, 0);
//     if (reg != NULL) {
//         return reg;
//     }
//     reg = __get_free_register(fregisters, exempt_reg, instructions_info, 1, 24, 1);
//     if (reg != NULL) {
//         return reg;
//     }
//     reg = __get_free_register(fregisters, exempt_reg, instructions_info, 24, 32, 1);
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
    STEntry* ste = (STEntry*) (node->val);
    // printf("Here\n");
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
        pos_reg->reg->offset = ste->loc_from_ref_reg;
    } else {
        // then just attach and return the ste
        pos_reg = init_reg_promise(NULL);
        // TODO: to add the type of register that should be given back as promise
        // when handling doubles
        pos_reg->ste = ste;
    }
    
    
    return pos_reg;
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
    if (node->n_type != t_STE) {
        if (left_child) {
            reg1 = get_expression(left_child, symt, instructions_info); // left child
            if (right_child) {
                reg2 = get_expression(right_child, symt, instructions_info); // right child
            }
        }
    }
    
    // printTreeIdent(node);
    // printTreeIdent(left_child);
    // printTreeIdent(right_child);
    // fprintf(debug, "---\n");
    // fflush(debug);

    lli* addr;
    int dtype = -1;
    RegPromise* final_reg = NULL;
    RegPromise* new_reg, * temp_reg;
    // printf("Evaluating %d\n", node->n_type);
    switch (node->n_type)
    {
    case t_BOOLEAN:
    case t_NUM_I:
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
            addu(instructions_info, reg2, reg1,reg2);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '-':
            subu(instructions_info, reg2, reg1,reg2);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '*':
            mult(instructions_info, reg2, reg1);
            mflo(instructions_info, reg2);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '/':
            mips_div(instructions_info, reg1, reg2);
            mflo(instructions_info, reg2);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '%':
            if (reg1->immediate && reg2->immediate) {
                *reg2->immediate = (lli)(*(double*)reg1->immediate) % (lli)(*(double*)reg2->immediate);
            } else {
                mips_div(instructions_info, reg1, reg2);
                mfhi(instructions_info, reg2);
            }
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '&':
            and(instructions_info, reg2, reg2, reg1);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '|':
            or(instructions_info, reg2, reg2, reg1);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '<':
            slt(instructions_info, reg2, reg1,reg2);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '>':
            slt(instructions_info, reg2, reg2,reg1);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        case '!':
            nor(instructions_info, reg1, reg1, zero_reg_promise);
            __free_regpromise(reg1);
            final_reg = reg2;
            break;
        default: break;
        }
        break;
    case t_EE:
        xor(instructions_info, reg2, reg1,reg2);
        sltiu(instructions_info, reg2, reg2, 1ll); // $t0 = 1ll if $t0 == 0 ((unsigned)t0 < 1ll) (equal), 0 otherwise
        __free_regpromise(reg1);
        final_reg = reg2;
        break;
    case t_NE:
        xor(instructions_info, reg2, reg1,reg2);
        sltiu(instructions_info, reg2, reg2, 1ll); // $t0 = 1ll if $t0 == 0 ((unsigned)t0 < 1ll) (equal), 0 otherwise
        xori(instructions_info, reg2, reg2, 1ll); // reverse the bit
        __free_regpromise(reg1);
        final_reg = reg2;
        break;
    case t_GTE:
        slt(instructions_info, reg2, reg1,reg2);
        xori(instructions_info, reg2, reg2, 1ll); // reverse the result, if t0 == 0 (t1 >= t2) then 1ll, else 0
        __free_regpromise(reg1);
        final_reg = reg2;
        break;
    case t_LTE:
        slt(instructions_info, reg2, reg2,reg1);
        xori(instructions_info, reg2, reg2, 1ll); // reverse the result, if t0 == 0 (t2 >= t1) then 1, else 0
        __free_regpromise(reg1);
        final_reg = reg2;
        break;
    case t_MINUS:
        if (reg1 == NULL) {
            yyerror("no operand for evaluation\n");
        }
        subu(instructions_info, reg1, zero_reg_promise, reg1);
        final_reg = reg1;
        break;
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
    default: break;
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
    // printf("Expr\n");
    // this promise definite has a register or immediate
    if (expr_promise == NULL) {
        yyerror("No expression received\n");
    }
    RegPromise* ste_promise = get_ste(var_node, symt, instructions_info);
    // printf("STE\n");
    int free_ste_promise = 1;
    if (ste_promise->ste != NULL) {
        STEntry* ste = ste_promise->ste;
        free_ste_promise = 0;
        // printf("::%s\n", ste->name);
        __free_regpromise(ste_promise);
        ste_promise = ste->ref_reg;
        // printf("%s\n",ste_promise->reg->name);
        ste_promise->reg->offset = ste->loc_from_ref_reg;
    }
    if (expr_promise->immediate != NULL) {
        // this is expression not a register
        RegPromise* new_reg = get_free_register_promise(instructions_info);
        // printf("pp %lld\n", (lli)*((double*)(expr_promise->immediate)));
        addiu(instructions_info, new_reg, zero_reg_promise, (lli)*((double*)(expr_promise->immediate)));
        // printf("pp\n");
        __free_regpromise(expr_promise);
        expr_promise = new_reg;
    }
    
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
int store_argument(Instructions_info* instr_info, RegPromise* reg, int start, int store_addr) {
    if (reg->immediate == NULL) {
        if (reg->ste != NULL && store_addr == 1) {
                STEntry* ste = reg->ste;
                RegPromise* new_reg = get_free_register_promise(instr_info);
                ste->ref_reg->reg->offset = NO_ENTRY;
                addiu(instr_info, new_reg, ste->ref_reg, ste->loc_from_ref_reg);
                assign_reg_promise(reg, new_reg);
                free(new_reg);
        } else {
            // offset is not null
            reload_reg(reg, instr_info);
            if (store_addr == 0 && reg->reg->offset != NO_ENTRY) {
                loadw(instr_info, reg, reg);
            }
        }
    } else {
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info, new_reg, zero_reg_promise, (lli)((double*)(reg->immediate)));
        assign_reg_promise(reg, new_reg);
        free(new_reg);
    }
    sp_reg_promise->reg->offset = start;
    storew(instr_info, reg, sp_reg_promise);
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
        arg = store_argument(instr_info, reg, arg, store_addr);
        if (count < 14) {
            sp_reg_promise->reg->offset = (arg - 4);
            // till now all args and temps have been saved
            RegPromise* arg_promise = init_reg_promise(registers[count++]);
            loadw(instr_info,arg_promise,sp_reg_promise);
            __free_regpromise(arg_promise);
            // free(arg_promise);
        }
        __free_regpromise(reg);
        // free(reg);
    }
    return arg;
}

RegPromise* create_formatting_string(Instructions_info* instr_info) {
    RegPromise* a0_reg_promise = init_reg_promise(registers[10]);
    addiu(instr_info, a0_reg_promise, zero_reg_promise, 256);
    write_instr(instr_info, "lui	$gp,%%hi(__gnu_local_gp)\n \
	addiu	$gp,$gp,%%lo(__gnu_local_gp)");
    
    write_instr(instr_info, "lw	$v0,%%call16(malloc)($gp)");
    write_instr(instr_info, "jalr\t$v0\n\tnop");
    RegPromise* v0_reg_promise = init_reg_promise(registers[14]);
    move(instr_info, a0_reg_promise, v0_reg_promise);
    __free_regpromise(v0_reg_promise);
    return a0_reg_promise;
}

void free_formatting_string(Instructions_info* instr_info, RegPromise* a0_reg_promise) {
    sp_reg_promise->reg->offset = 0;
    loadw(instr_info, a0_reg_promise, sp_reg_promise);
    write_instr(instr_info, "lui	$gp,%%hi(__gnu_local_gp)\n \
	addiu	$gp,$gp,%%lo(__gnu_local_gp)");
    
    write_instr(instr_info, "lw	$v0,%%call16(free)($gp)");
    write_instr(instr_info, "jalr\t$v0\n\tnop");
    return;
}

RegPromise* scaffold_for_read_write(Node* node, SymbolTable* symt, Instructions_info* instr_info, int read) {

    Node* params = node->first_child->first_child;
    // printTreeIdent(params);fflush(debug);
    Queue* param_queue = init_queue();
    while (params != NULL) {
        RegPromise* reg = get_expression(params->first_child, symt, instr_info);
        enqueue_queue(param_queue, (lli)reg);
        params = params->next;
    }
    // save temp and arg registers registers
    store_arg_n_temp_registers(instr_info);
    // printf("reg%d\n", instr_info->gen_code);
    // to create the formatting string
    RegPromise* reg = create_formatting_string(instr_info);
    int dist = 0;
    int len = param_queue->len;
    while (len -- > 0) {
        // TODO: check data type and add formatting appropiately
        // RegPromise* arg_reg = regvals[i];
        // TODO: assume we get some data type here and work with it
        // adding for INT
        write_instr(instr_info, "%-7s\t$a1,\t$0,%d","addiu", '%');
        write_instr(instr_info, "%-7s\t$a1,\t%d($a0)","sb", dist);
        dist ++;
        write_instr(instr_info, "%-7s\t$a1,\t$0,%d","addiu", 'd');
        write_instr(instr_info, "%-7s\t$a1,\t%d($a0)","sb", dist);
        dist ++;
        write_instr(instr_info, "%-7s\t$a1,\t$0,%d","addiu", ' ');
        write_instr(instr_info, "%-7s\t$a1,\t%d($a0)","sb", dist);
        dist ++;
        if (dist >= 255) {
            yyerror("Too much varaibles to print in single write");
        }
    }
    write_instr(instr_info, "%-7s\t$a1,\t$0,$0","addu");
    write_instr(instr_info, "%-7s\t$a1,\t%d($a0)","sb", dist);
    dist ++;
    int start = store_argument(instr_info,reg, 0, 0);
    start = store_arguments(instr_info, param_queue, start, 11, read);
    instr_info->call_window_size =  max(start, instr_info->call_window_size);
    free_queue(param_queue);
    return reg;
}

/**
 * returns the argument window size
 */
int handle_write(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL) {
        return 1;
    }
    RegPromise* a0_prom = scaffold_for_read_write(node,symt,instr_info, 0);
    // jump
    write_instr(instr_info, "lui	$gp,%%hi(__gnu_local_gp)\n \
	addiu	$gp,$gp,%%lo(__gnu_local_gp)");
    
    write_instr(instr_info, "lw	$v0,%%call16(printf)($gp)");
    write_instr(instr_info, "jalr\t$v0\n\tnop");
    free_formatting_string(instr_info, a0_prom);
    __free_regpromise(a0_prom);
}

int handle_read(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL) {
        return 1;
    }
    RegPromise* a0_prom = scaffold_for_read_write(node,symt,instr_info, 1);
    // jump
    write_instr(instr_info, "lui	$gp,%%hi(__gnu_local_gp)\n \
	addiu	$gp,$gp,%%lo(__gnu_local_gp)");
    
    write_instr(instr_info, "lw	$v0,%%call16(__isoc99_scanf)($gp)");
    write_instr(instr_info, "jalr\t$v0\n\tnop");
    free_formatting_string(instr_info, a0_prom);
    // free(a0_prom);
    __free_regpromise(a0_prom);
}

/**
 * writes the argument window size in Instructions_info
 */
int handle_function_call(Node* node, SymbolTable* symt, Instructions_info* instr_info) {
    if (node == NULL || node->n_type != t_FUNC_CALL) {
        yyerror("Wrong node received for a FUNC Call\n");
    }
    char* name = (char*)node->val;
    if (strcmp(name,"write") == 0) {
        handle_write(node, symt, instr_info);
    } else if (strcmp(name,"read") == 0) {
        handle_read(node, symt, instr_info);
    } else {
        Node* params = node->first_child->first_child;
        Queue* param_queue = init_queue();
        while (params != NULL) {
            RegPromise* reg = get_expression(params->first_child, symt, instr_info);
            enqueue_queue(param_queue, (lli)reg);
            params = params->next;
        }
        // save temp and arg registers registers
        store_arg_n_temp_registers(instr_info);
        int start = store_arguments(instr_info, param_queue, 0, 10, 0);
        instr_info->call_window_size =  max(start, instr_info->call_window_size);
        free_queue(param_queue);
    }
}

int handle_statement(Node* node, SymbolTable* symt, Instructions_info* instructions_info) {
    if (node == NULL) {
        fprintf(stderr, "Wrong node received for a Statement\n");
        return 1;
    }
    // printf("Stamt %d \n", node->n_type);
    switch (node->n_type)
    {
    case t_ASSIGN:
        handle_assignment(node, symt, instructions_info);
        break;
    case t_COND:
        break;
    case t_FOR:
        break;
    case t_WHILE:
        break;
    case t_DOWHILE:
        break;
    case t_CONTINUE:
        break;
    case t_BREAK:
        break;
    case t_FUNC_RET:
        break;
    case t_FUNC_CALL:
        handle_function_call(node, symt, instructions_info);
        break;
    default:
        break;
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
    while (child != NULL) {
        handle_statement(child, symt, instructions_info);
        child = child->next;
    }


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
            if (curr_sp_size % 4 != 0) {
                int next_block = (curr_sp_size / 4 + 1) * 4;
                if (curr_sp_size + len > next_block) {
                    curr_sp_size = next_block;
                }
            }
            ste->ref_reg = sp_reg_promise;
            curr_sp_size += len;
            ste->loc_from_ref_reg = (curr_sp_size - dsize);
            vars = vars->next;
        }
        child = child ->next;
    }
}

/**
 * start : how much below have we come from fp.
 */
int declare_variables(Node* node,SymbolTable* symt, RegPromise* reference_reg,int start) {
    if (reference_reg != gp_reg_promise && reference_reg != sp_reg_promise && reference_reg != fp_reg_promise) {
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

    if (strcmp(symt->name, "global") == 0) {
        symt->name = ("main");
    } else if (strcmp(symt->name, "main") == 0) {
        symt->name = ("main_main");
    } 
    
    int sp_size = 0;
    Node* func_decl = node->first_child->next->next;
    int var_space = declare_variables(func_decl, symt, fp_reg_promise, 0);
    Node* func_arg_list = node->first_child->next;
    int arg_space = declare_arguments(func_arg_list, symt, 0);
    
    Node* func_body = node -> first_child -> next -> next -> next;
    Instructions_info instructions_info;
    instructions_info.instructions_stack = init_stack();
    push_stack(instructions_info.instructions_stack,(lli)init_string("", 0));
    
    // first pass
    instructions_info.gen_code = 0;
    instructions_info.extra_reg_count = 0;
    instructions_info.call_window_size = 0;
    instructions_info.curr_size = var_space;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = ra_reg;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = fp_reg;
    
    handle_func_body(func_body, symt, &instructions_info);
    int extra_fn_argument_space= instructions_info.call_window_size; // max(arguments to be passed to any function)
    int extra_space = instructions_info.curr_size - var_space; // extra_space for register spill fill
    
    int extra_reg_count = instructions_info.extra_reg_count;
    int extra_reg_space = (4 * extra_reg_count);
    // in decreasing order of address -- >
    // | sp/fp |  extra_fn_argument_space  | local_registers(var_space) | extra_space | extra_reg_space
    add_to_locals(symt,extra_fn_argument_space); // extra_reg(callee saved one are stored first)
    
    printf("%d %d %d %d\n", extra_fn_argument_space, var_space, extra_space,extra_reg_space);
    // prologue
    sp_size = extra_fn_argument_space + var_space + extra_space + extra_reg_space; // full sp needed (utilized and saved also in same order)
    
    instructions_info.gen_code = 1;


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
        move(&instructions_info,gp_reg_promise,fp_reg_promise);
    }
    write_instr(&instructions_info,"\n");
    
    // body
    // writes the instructions_info this time
    instructions_info.extra_reg_count = 0;
    instructions_info.call_window_size = 0;
    instructions_info.curr_size = extra_fn_argument_space + var_space;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = ra_reg;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = fp_reg;
    // printf("This is all we got\n");
    handle_func_body(func_body, symt, &instructions_info);

    // epilogue
    write_instr(&instructions_info,"\n");
    move(&instructions_info, sp_reg_promise, fp_reg_promise);
    load_extra_regs(&instructions_info, sp_size);
    addiu(&instructions_info, sp_reg_promise, sp_reg_promise, sp_size);
    
    String* instrs = (String*)pop_stack(instructions_info.instructions_stack);
    instr_out_no_tab("%s", instrs->val);

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