#include "../../include/includes.h"
#include "../../include/compiler.h"

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
        if (ste->var_type != LOCAL) {
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
    if (reference_reg != gp_reg_promise && reference_reg != sp_reg_promise && reference_reg != fp_reg_promise ) {
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
            if (ste->var_type == LOCAL) {
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
            }
            vars = vars->next;
        }
        child = child ->next;
    }
    if (size % 4 != 0) {
        size = (size / 4 + 1) * 4;
    }
    return size;
}

void declare_global_variables(Node* node,SymbolTable* symt) {

    if (node == NULL || node->n_type != t_DECL) {
        yyerror("WRONG node received for Declaration");
    }

    Instructions_info instr_info;
    instr_info.instructions_stack = init_stack();
    push_stack(instr_info.instructions_stack,(lli)init_string("", 0));
    instr_info.gen_code = 1;

    Node* child = node->first_child;
    while (child != NULL) {
        Node* vars = child->first_child;
        while(vars != NULL) {
            STEntry *ste = (STEntry*) (vars->val);
            if (ste->var_type == GLOBAL) {

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
                insert(labels, (lli)ste->name, (lli)ste->name);
                String* instr = (String*)top_stack(instr_info.instructions_stack);
                write_instr(&instr_info, ".global\t%s", ste->name);
                write_instr(&instr_info, ".align\t2");
                write_instr(&instr_info, ".type\t%s,\t@object", ste->name);
                write_instr(&instr_info, ".size\t%s,%d",ste->name, len);
                write_instr_val(instr, 0, "%s:",ste->name);
                write_instr(&instr_info, ".space\t%d", len);
            }
            vars = vars->next;
        }
        child = child ->next;
    }
    String* instrs = (String*)top_stack(instr_info.instructions_stack);
    fprintf(yyout,"\t.section\t.bss,\"aw\",@nobits \n\n\n");
    instr_out_no_tab("%s", instrs->val);
    return;
}

void print_symbol_table_out(SymbolTable* symt) {
    HashMap *table = symt->local_table;
    lli *table_keys = keys(table);

    for (int i = 0; i < table->len; i++)
    {
        STEntry *ste = (STEntry *)get(table, table_keys[i]);
        if (ste->var_type != GLOBAL) {
            instr_out("#\t%s:\t%d(%s)\t%d", ste->name, ste->loc_from_ref_reg, ste->ref_reg->reg->name, (ste->var_type == LOCAL) ? "LOCAL" : "ARG" );
        } else {
            instr_out("#\t%s:\t%s", ste->name, "GLOBAL");
        }
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

    String* return_label = init_string("ret_", 4);
    add_str(return_label, symt->name, -1);
    insert(labels, (lli)return_label->val, (lli)return_label->val);

    
    int sp_size = 0;
    Node* func_decl = node->first_child->next->next;
    Node* func_body = node -> first_child -> next -> next -> next;
    Instructions_info instructions_info;
    instructions_info.instructions_stack = init_stack();
    push_stack(instructions_info.instructions_stack,(lli)init_string("", 0));
    instructions_info.gen_code = 1;


    int var_space = declare_variables(func_decl, symt, ref_reg_promise, 0);
    
    
    // first pass
    instructions_info.gen_code = 0;
    instructions_info.extra_reg_count = 0;
    instructions_info.label_count = 0;
    instructions_info.call_window_size = 0;
    instructions_info.return_label = return_label;
    instructions_info.curr_size = var_space;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = ra_reg;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = fp_reg;

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
    // in Increasing order of address -- >
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

    write_instr(&instructions_info,"\n");
    
    // body
    // writes the instructions_info this time
    instructions_info.extra_reg_count = 0;
    instructions_info.label_count = 0;
    instructions_info.call_window_size = 0;
    instructions_info.curr_size = extra_fn_argument_space + var_space;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = ra_reg;
    instructions_info.extra_regs[instructions_info.extra_reg_count ++] = fp_reg;
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
    if (node == NULL || node->n_type != t_DECL)
    {
        return 0;
    }

    initialize_registers();

    fprintf(yyout, "%s",code_prologue);

    // global definitons
    Node* global_decl = node; 
    Node* prog_decl = node->next;
    Node* child = prog_decl->first_child; // first child main
    SymbolTable* symt = (SymbolTable*) (child->val);

    
    declare_global_variables(global_decl, symt);
    
    printTreeIdent(child);
    fflush(debug);
    
    fprintf(yyout,"\t.text\n\n\n");
    while (child != NULL) {
        handle_function_definitions(child);
        fflush(debug);
        child = child->next;
    }

    free_registers_struct();
    fprintf(yyout, "%s",code_epilogue);
    fflush(yyout);
}   