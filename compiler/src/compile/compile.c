
#include "../../include/includes.h"
#include "../../include/compiler.h"

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
            dsize = 4;
            break;
            case INT:
            dsize = 4;
            break;
            default:
            return NULL;
        }
        sll(instructions_info, pos_reg, pos_reg, log2(dsize));

        // got the address in pos_reg
        // now we just need to load the address
        if (ste->var_type != GLOBAL) {
            ste->ref_reg->reg->offset = NO_ENTRY;
            addu(instructions_info, pos_reg, pos_reg, ste->ref_reg);
            pos_reg->reg->offset = ste->loc_from_ref_reg;
            
        } else {
            // now we need to access the value using s0 only
            RegPromise* label_reg = get_free_register_promise(instructions_info);
            load_label(instructions_info, label_reg, ste->name );
            addu(instructions_info, pos_reg, pos_reg, label_reg);
            __free_regpromise(label_reg);
            pos_reg->reg->offset = 0;
        }

    } else if (ste->var_type != GLOBAL) {
        // then just attach and return the ste
        pos_reg = init_reg_promise(NULL);
        // TODO: to add the type of register that should be given back as promise
        // when handling doubles
        pos_reg->ste = ste;
        enqueue_queue(ste_register_promises, (lli)pos_reg);
    } else {
        pos_reg = get_free_register_promise(instructions_info);
        load_label(instructions_info, pos_reg, ste->name );
        pos_reg->reg->offset = 0;
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
    
    Node *var_node = node->first_child;
    Node *expr_node = var_node->next;
    
    RegPromise* expr_promise = get_expression(expr_node->first_child, symt, instructions_info);
    
    // This is important in case it uses the reference register then ste ref offset will be overriden
    reload_reg_withoffset(expr_promise,instructions_info);
    
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
            reload_reg(reg,instr_info);
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
    // printf("reg%d\n", instr_info->gen_code);
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
    reload_reg_withoffset(expr_promise,instr_info);
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

    reload_reg_withoffset(expr_promise,instr_info);


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
        // if (expr_promise->immediate != NULL) {
        //     // this is expression not a register
        //     RegPromise* new_reg = get_free_register_promise(instr_info);
        //     // printf("pp %lld\n", (lli)*((double*)(expr_promise->immediate)));
        //     addiu(instr_info, new_reg, zero_reg_promise, (lli)*((double*)(expr_promise->immediate)));
        //     // printf("pp\n");
        //     __free_regpromise(expr_promise);
        //     expr_promise = new_reg;
        // } else {
        // }
        reload_reg_withoffset(expr_promise,instr_info);
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
        reload_reg_withoffset(expr_promise,instr_info);
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
        reload_reg_withoffset(expr_promise,instr_info);
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


