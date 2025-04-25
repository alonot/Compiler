#include "../include/includes.h"

/**
 * assigns the value in addr to the val according to dtype
 */
void assign_val(lli *addr, int dtype, double *val)
{
    switch (dtype)
    {
    case DOUBLE:
        *(double *)val = (double)*(double *)addr;
        break;
    case INT:
        *val = *addr;
        break;
    case BOOL:
        *val = *(short *)addr != 0 ;
        break;
    default: break;
        break;
    }
    // fprintf(yyout,"got %f %p %lld %f %d\n", *addr, val, *val, *(double*)val, dtype);
}

/**
 * assigns the val to the addr according to dtype
 */
void assign_addr(lli *addr, int dtype, double val)
{
    // fprintf(yyout,"got %lld %f %f\n", *addr, *(double*)addr,  val);
    switch (dtype)
    {
    case DOUBLE:
        *(double *)addr = val;
        // fprintf(yyout,"%f\n", *(double*)addr);
        break;
    case INT:
        *addr = (lli)val;
        // fprintf(yyout,"%lld\n", *(lli*)addr);
        break;
    case BOOL:
        *(short *)addr = !check_eq_to_int(val,0, dtype);
        // fprintf(yyout,"%d\n", *(short*)addr);
        break;
    default: break;
        break;
    }
}

void evaluate_function(Node *node, double *val)
{
    if (node == NULL)
        return;
    if (node->n_type != t_FUNC_CALL)
        return;

    char *fn_name = (char *)node->val;
    if (strcmp(fn_name, "READ") == 0)
    {
        Node* child = node->first_child -> first_child; // param_list -> expr
        lli* addr;
        lli val;
        int dtype = resolve_ste(child, &addr);
        // fprintf(yyout,"Reading data %p\n", addr);
        freopen("/dev/tty", "r", stdin);
        switch(dtype) {
            case INT:
            fscanf(stdin,"%lld", addr);
            break;
            case BOOL:
            fscanf(stdin,"%hd", (short*)&val);
            *(short*)addr = val != 0;
            break;
            case DOUBLE:
            fscanf(stdin,"%lf", (double*)addr);
            break;

        }
    }
    else if (strcmp(fn_name, "WRITE") == 0)
    {

        // currently assuming all functions are write functions
        double ret_val;
        // fprintf(yyout,"Evaluating FN\n");
        Node *child = node->first_child -> first_child; // param_list -> (expr/ ste)
        while (child != NULL)
        {
            if (child->n_type == t_STR)
            {
                String *str = evaluate_string(child);
                fprintf(yyout,"%s ", str->val);
            }
            else
            {

                int dtype = evalute_expr(child->first_child, &ret_val);
                switch (dtype)
                {
                case BOOL:
                    fprintf(yyout,"%s ", !check_eq_to_int(ret_val, 0, dtype) ? "True" : "False");
                    break;
                case INT:
                    fprintf(yyout,"%lld ", (lli)(ret_val));
                    break;
                case DOUBLE:
                    fprintf(yyout,"%f ", ret_val);
                    break;
                default: break;
                break;
                }
            }
            child = child->next;
        }
        fprintf(yyout,"\n");
        *val = 0;
    }
}

/**
 * requires the node to be t_ASSIGN type
 */
void evaluate_assign(Node *node)
{
    if (node == NULL || node->n_type != t_ASSIGN)
    {
        return;
    }
    Node *var_node = node->first_child;
    Node *expr_node = var_node->next;
    lli *val_addr;
    int dtype = -1;
    // fprintf(yyout,"Assinging..\n");
    if ((dtype = resolve_ste(var_node, &val_addr)) == -1)
    {
        yyerror("Unable to load a variable\n");
    }
    double val;
    evalute_expr(expr_node->first_child, &val);
    // fprintf(yyout,"With %p %lf\n" ,val_addr,val);
    assign_addr(val_addr, dtype, val);
    // fprintf(yyout,"---\n");
    // fprintf(yyout,"%f %p %lld %f\n", val, val_addr, *val_addr, *(double*)val_addr);
}

/**
 * requires the node to be t_STE type
 * returns type of the variable
 */
int resolve_ste(Node *node, lli **addr)
{
    if (node == NULL || node->n_type != t_STE)
    {
        return -1;
    }
    int dtype = -1;
    lli *to_retaddr = NULL;
    STEntry *ste = (STEntry *)node->val;
    // fprintf(yyout,"%s ", ste->name);
    switch (ste->dtype)
    {
        case DOUBLE:
        dtype = DOUBLE;
        to_retaddr = (lli *)&(ste->value.dval);
        break;
        case INT:
        dtype = INT;
        to_retaddr = (lli *)&(ste->value.lval);
        break;
        case BOOL:
        dtype = BOOL;
        to_retaddr = (lli *)&(ste->value.lval);
        break;
        default: break;
    }
    // fprintf(yyout,"%s %d\n", ste->name, dtype);
    // update the addr value
    if (ste->is_array != 0)
    {
        lli pos= 0;
        lli len = 1;
        DARRAY *arrval = ste->value.arrval;
        node = node->first_child; // first EXPR
        // NOTE: The nodes are organized in reverse manner
        // NOTE: This traverse thorugh array in reverse order of their declaration: <-- [4][9][2] --
        int depth = arrval->arr_depth;
        while (depth != 0)
        {
            double index;
            evalute_expr(node->first_child, &index);
            pos += len * (lli)index;
            len *= arrval->arr_lengths[depth];
            // fprintf(yyout,"%f %d %d\n", index, depth, arrval->arr_lengths[depth]);
            if (pos >= arrval->arr_max_pos)
            {
                fprintf(yyout,"Array index %d out of bounds(%d) for array %s\n", (int)index, arrval->arr_lengths[depth + 1], ste->name);
                yyerror("");
            }
            node = node->next; // goes to next EXPR
            depth--;
        }
        switch (ste->dtype)
        {
        case BOOL:
            to_retaddr = (lli *)((short *)(arrval->arr) + pos);
            break;
        case DOUBLE:
            to_retaddr = (lli *)((double *)(arrval->arr) + pos);
            break;
        case INT:
            to_retaddr = (lli *)(arrval->arr) + pos;
            break;
        default: break;
        }
    }
    // fprintf(yyout,"%s %d %p\n",ste->name, dtype, to_retaddr);
    *addr = to_retaddr;
    return dtype;
}

/**
 * expected child of the t_EXPR node
 * * returns the type of expression
 */
int evalute_expr(Node *node, double *val)
{
    if (node == NULL)
        return -1;
    int ret_dtype = BOOL;
    double operand1 = 0, operand2 = 0;
    if (node->n_type != t_STE) {

        if (node->first_child)
        {
            ret_dtype = max(ret_dtype, evalute_expr(node->first_child, &operand1));
            if (node->first_child->next)
            {
                ret_dtype = max(ret_dtype, evalute_expr(node->first_child->next, &operand2));
            }
        }
    }
    lli* addr;
    int dtype = -1;
    // fprintf(yyout,"Evaluating %d\n", node->n_type);
    switch (node->n_type)
    {
    case t_BOOLEAN:
        *val = node->val;
        break;
    case t_NUM_I:
    case t_NUM_F:
        *val = *(double*)node->val;
        break;
    case t_STE:
        if ((dtype = resolve_ste(node, &addr)) == -1)
        {
            yyerror("unable to resolve a variable\n");
        }
        assign_val(addr, dtype, val);
        return dtype;
        break;
    case t_OP:
        switch ((char)(node->val))
        {
        case '+':
            *val = operand1 + operand2;
            break;
        case '-':
            *val = operand1 - operand2;
            break;
        case '*':
            *val = operand1 * operand2;
            break;
        case '/':
            *val = operand1 / operand2;
            break;
        case '%':
            *val = (lli)operand1 % (lli)operand2;
            ret_dtype = (ret_dtype == DOUBLE) ? INT : ret_dtype;
            break;
        case '&':
            *val = (lli)operand1 & (lli)operand2;
            ret_dtype = (ret_dtype == DOUBLE) ? INT : ret_dtype;
            break;
        case '|':
            *val = (lli)operand1 | (lli)operand2;
            ret_dtype = (ret_dtype == DOUBLE) ? INT : ret_dtype;
            break;
        case '<':
            *val = (double)(operand1 < operand2);
            ret_dtype = BOOL;
            break;
        case '>':
            *val = ((double)(operand1 > operand2));
            ret_dtype = BOOL;
            break;
        case '!':
            *val = !(lli)operand1;
            ret_dtype = (ret_dtype == DOUBLE) ? INT : ret_dtype;
            break;
        default: break;
        }
        break;
    case t_EE:
        *val = (double)(operand1 == operand2);
        ret_dtype = BOOL;
        break;
    case t_NE:
        *val = (double)(operand1 != operand2);
        ret_dtype = BOOL;
        break;
    case t_GTE:
        *val = (double) (operand1 >= operand2);
        ret_dtype = BOOL;
        break;
    case t_LTE:
        *val = (double)(operand1 <= operand2);
        ret_dtype = BOOL;
        break;
    case t_MINUS:
        *val = -operand1;
        break;
    default: break;
    }
    // fprintf(yyout,"EXPR: %f\n", *val);
    return ret_dtype;
}

int check_eq_to_int(double val, int to, int dtype)
{
    switch (dtype)
    {
    case BOOL:
        return ((short)val != 0) == to;
        break;
    case DOUBLE:
        return (val) == to;
        break;
    case INT:
        return ((lli)val) == to;
        break;
    }
}

int evaluate_for(Node *node)
{
    if (node == NULL || node->n_type != t_FOR)
        return -1;
    Node *initialization_node = node->first_child;
    Node *cond_node = initialization_node->next;
    Node *update_node = cond_node->next;
    Node *loop_block = update_node->next;
    evaluate_assign(initialization_node); // is nope then nothing gets evaluated
    double val;
    int dtype;
    cond_node = cond_node->first_child;
    while (1)
    {
        dtype = evalute_expr(cond_node, &val);
        if (check_eq_to_int(val, 0, dtype))
        {
            break;
        }
        int res = run(loop_block->first_child);

        if (res == 1)
        { // break statement found
            break;
        }
        evaluate_assign(update_node);
    }
}

int evaluate_while(Node *node)
{
    if (node == NULL || node->n_type != t_WHILE)
        return -1;
    Node *cond_node = node->first_child;
    Node *loop_block = cond_node->next;
    cond_node = cond_node->first_child;
    double val;
    int dtype;
    while (1)
    {
        dtype = evalute_expr(cond_node, &val);
        if (check_eq_to_int(val, 0, dtype))
        {
            break;
        }
        int res = run(loop_block->first_child);
        if (res == 1)
        { // break statement found
            break;
        }
    }
}

int evaluate_dowhile(Node *node)
{
    if (node == NULL || node->n_type != t_DOWHILE)
        return -1;
    Node *cond_node = node->first_child;
    Node *loop_block = cond_node->next;
    cond_node = cond_node->first_child;
    double val;
    int dtype;
    while (1)
    {
        int res = run(loop_block->first_child);
        if (res == 1)
        { // break statement found
            break;
        }
        dtype = evalute_expr(cond_node, &val);
        if (check_eq_to_int(val, 0, dtype))
        {
            break;
        }
    }
}

int evaluate_cond(Node *node)
{
    if (node == NULL || node->n_type != t_COND)
        return -1;
    Node *cond_node = node->first_child;
    Node *if_block = cond_node->next;
    Node *else_block = if_block->next; // maybe null
    cond_node = cond_node->first_child;
    double val;
    int dtype;
    dtype = evalute_expr(cond_node, &val);
    if (!check_eq_to_int(val, 0, dtype))
    {
        return run(if_block->first_child);
    }
    else if (else_block)
    {
        return run(else_block->first_child);
    }
    return 0;
}

String *evaluate_string(Node *node)
{
    if (node == NULL || node->n_type != t_STR || node->val != 0)
    {
        return NULL;
    }
    String *str = init_string("", 0);
    node = node->first_child;
    while (node != NULL)
    {
        add_str(str, (char *)node->val, -1);
        repeat_n_add(str, ' ', 1);
        node = node->next;
    }
    return str;
}

int run(Node *node)
{
    if (node == NULL)
    {
        return 0;
    }
    int res;
    double val;
    switch (node->n_type)
    {
    case t_ASSIGN:
        // fprintf(yyout,"Evaluating ASsing\n");
        evaluate_assign(node);
        break;
        case t_FUNC_CALL:
        // fprintf(yyout,"Evaluating Func\n");
        evaluate_function(node, &val);
        break;
        case t_COND:
        // fprintf(yyout,"Evaluating Cond\n");
        res = evaluate_cond(node);
        if (res != 0)
        {
            return res; // break or continue is encountered
        }
        break;
    case t_WHILE:
        evaluate_while(node);
        break;
    case t_DOWHILE:
        evaluate_dowhile(node);
        break;
    case t_FOR:
        evaluate_for(node);
        break;
    case t_BREAK:
        return 1;
        break;
    case t_CONTINUE:
        return 2;
        break;
    case t_KEYWORD:
        // tbd
        break;
    case t_OTHER:
        res = run(node->first_child);
        if (res != 0)
            return res;
        break;
    default: break;
    }
    return run(node->next);
}