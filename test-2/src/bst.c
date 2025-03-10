#include "../include/includes.h"

Node* init_node(lli val, NODETYPE n_type,  void (*free_fn)(lli)) {
    Node* n = (Node*)(calloc(1, sizeof(Node))) ;
    n->val = val;
    n->n_type = n_type;
    n->free = free_fn;
    return n;
}

int add_child(Node* node, Node* child) {
    if (node == NULL) {
        return -1;
    }
    if (child == NULL) {
        return 0;
    }
    child->next = NULL;
    if (node->first_child == NULL) {
        node->first_child = child;
        node->last_child = child;
    } else if (node->last_child != NULL) {
        node->last_child->next = child;
        node->last_child = child;
    } else {
        return -1;
    }
    return 0;
}

int add_child_in_front(Node* node, Node* child) {
    if (node == NULL) {
        return -1;
    }
    if (child == NULL) {
        return 0;
    }
    child->next = node->first_child;
    if (node->first_child == NULL) {
        node->last_child = child;
    } 
    node->first_child = child;
    return 0;
}

int update_last_child(Node* node) {
    if (node == NULL) {
        return 0;
    }
    Node* child = node ->first_child;
    Node* prev = NULL;
    while (child != NULL) {
        prev = child;
        child = child -> next;
    }
    node ->last_child = prev;
}

int add_neighbour(Node* node, Node* child) {
    if (node == NULL) {
        return -1;
    }
    if (child == NULL) {
        return 0;
    }
    update_last_child(node);
    node->next = child;
    return 0;
}

int add_all_children(Node* node, Node* child, Node* lastchild) {
    if (node == NULL || child == NULL) {
        return -1;
    }
    if (lastchild != NULL) {
        lastchild->next = NULL;
    }
    if (node->first_child == NULL) {
        node->first_child = child;
        node->last_child = lastchild;
    } else if (node->last_child != NULL) {
        node->last_child->next = child;
        node->last_child = lastchild;
    } else {
        return -1;
    }
}

char* node_to_str(Node* node, int* len) {
    *len = 100;
    char* val = (char*)(malloc(sizeof(char) * (*len)));
    switch (node->n_type) {
        case t_VAR: 
        *len = snprintf(val, *len, "VAR(%f)", (double)(node->val));
        break;    
        case t_ASSIGN: 
        *len = snprintf(val, *len, "ASSIGN(=)");
        break;    
        case t_NUM_I: 
        *len = snprintf(val, *len, "INTEGER(%ld)", (lli)*(double*)(node->val));
        break;    
        case t_NUM_F: 
        *len = snprintf(val, *len, "FLOAT(%lf)", *(double*)(node->val));
        break;
        case t_ARRAY_SIZE: 
        *len = snprintf(val, *len, "ARRAY_SIZE(%f)", (double)(node->val));
        break;    
        case t_STR: 
        *len = snprintf(val, *len, "STR(%s)", (char*)(node->val));
        break;    
        case t_IDENTIFIER: 
        *len = snprintf(val, *len, "IDENTIFIER(%s)", (char*)(node->val));
        break;    
        case t_EXPR: 
        *len = snprintf(val, *len, "EXPR");
        break;    
        case t_FUNC: 
        *len = snprintf(val, *len, "FUNC(%s)", (char*)(node->val));
        break;    
        case t_COND: 
        *len = snprintf(val, *len, "COND");
        break;    
        case t_FOR: 
        *len = snprintf(val, *len, "FOR");
        break;    
        case t_IF_BLOCK: 
        *len = snprintf(val, *len, "IF_BLOCK");
        break;    
        case t_ELSE_BLOCK: 
        *len = snprintf(val, *len, "ELSE_BLOCK");
        break;    
        case t_LOOP_BLOCK: 
        *len = snprintf(val, *len, "LOOP_BLOCK");
        break;    
        case t_NOP: 
        *len = snprintf(val, *len, "NOP");
        break;    
        case t_WHILE: 
        *len = snprintf(val, *len, "WHILE");
        break;    
        case t_DOWHILE: 
        *len = snprintf(val, *len, "DOWHILE");
        break; 
        case t_BREAK: 
        *len = snprintf(val, *len, "BREAK");
        break;    
        case t_CONTINUE: 
        *len = snprintf(val, *len, "CONTINUE");
        break;    
        case t_KEYWORD: 
        *len = snprintf(val, *len, "KEYWORD(%s)", (char*)(node->val));
        break;    
        case t_OTHER:
        *len = snprintf(val, *len, "%s", (char*)(node->val));
        break;    
        case t_BOOLEAN: 
        *len = snprintf(val, *len, "BOOLEAN(%s)", (node->val == 1 ? "true" : "false"));
        break;    
        case t_STE: 
        *len = sprintf_ste((STEntry*)node -> val, val, *len);
        break;    
        case t_OP: 
            switch((char)(node->val)) {
                case '+':
                *len = snprintf(val, *len, "ADD");
                break;
                case '-':
                *len = snprintf(val, *len, "SUB");
                break;
                case '*':
                *len = snprintf(val, *len, "MUL");
                break;
                case '/':
                *len = snprintf(val, *len, "DIV");
                break;
                case '%':
                *len = snprintf(val, *len, "MOD");
                break;
                case '&':
                *len = snprintf(val, *len, "AND");
                break;
                case '|':
                *len = snprintf(val, *len, "OR");
                break;
                case '<':
                *len = snprintf(val, *len, "LT");
                break;
                case '>':
                *len = snprintf(val, *len, "GT");
                break;
                case '!':
                *len = snprintf(val, *len, "NOT");
                break;
                default:
                *len = snprintf(val, *len, "NONE");
            }
        break;  
        case t_EE:
        *len = snprintf(val, *len, "EQ");
        break;  
        case t_NE:
        *len = snprintf(val, *len, "NE");
        break;  
        case t_GTE:
        *len = snprintf(val, *len, "GTEQ");
        break;  
        case t_LTE:
        *len = snprintf(val, *len, "LTEQ");
        break;  
        case t_MINUS:
        *len = snprintf(val, *len, "MINUS");
        break;  
        default:
        *len = snprintf(val, *len, "NONE");
    } 
    (*len) ++;
    if (*len <= 0) {
        printf("Failed%d %d\n",*len, node->n_type);
        return NULL; // failed
    }
    val = (char*) (realloc(val, *len));
    return val;
}

int depth(Node* node) {
    if (node == NULL) {
        return 0;
    }
    int cur_depth = 0;
    Node* child = node->first_child;
    while (child != NULL) {
        int d = depth(child);
        if (d > cur_depth) {
            cur_depth = d;
        }
        child = child->next;
    }
    return cur_depth + 1;
}


int create_print_tree(Node* node, int depth,int parent_offset,String* string_at_depth[]) {
    if (node == NULL) {
        return parent_offset;
    }
    // printf("%d \n", depth);
    int total_children = 0;
    Node* child = node->first_child;
    while (child != NULL) {
        total_children ++;
        child = child->next;
    }
    int mid = ceil(total_children / 2.0);
    int len = parent_offset;
    child = node->first_child;
    for (int i =0; i < mid; i ++) {
        len = create_print_tree(child,depth + 1, len, string_at_depth);
        child = child->next;
    }
    len -= length(string_at_depth[depth]);
    repeat_n_add(string_at_depth[depth], ' ', len < 0 ? 0 : len);
    // printf("after: %s.%d\n", string_at_depth[depth]->val, len);
    int cur_len = 0;
    char* val = node_to_str(node, &cur_len);
    add_str(string_at_depth[depth] ,val, cur_len - 1);
    free(val);
    len = length(string_at_depth[depth]);
    // printf("%sgetting%d %d %d\n", string_at_depth[depth]->val, len, parent_offset, cur_len);
    for (int i =mid; i < total_children; i ++) {
        len = create_print_tree(child, depth + 1, len, string_at_depth);
        child = child->next;
    }
    return len;
}


void printTree(Node* node) {
    int max_depth = depth(node);
    String* string_at_depth[max_depth];
    // printf("depth%d\n", max_depth);
    for (int i =0; i < max_depth; i ++) {
        string_at_depth[i] = init_string("", 0);
    }
    //
    create_print_tree(node, 0, 0,string_at_depth);
    //
    // printf("Hereh\n");
    for (int i =0; i < max_depth; i ++) {
        printf("%s\n", string_at_depth[i]->val);
        freeString(string_at_depth[i]);
    }
}

void printNodeWithIndent(Node* node, int depth)  {
    if (node == NULL)  return;
    for (int i =0; i < depth; i ++) {
        printf("\t");
    }
    int curr_len = 0;
    if (depth > 0) {
        printf("|-");
    } 
    char* val = node_to_str(node, &curr_len);
    printf("%s\n", val);
    free(val);
    printNodeWithIndent(node ->first_child, depth + 1);
    printNodeWithIndent(node ->next, depth);
}

void printTreeIdent(Node* node) {
    printNodeWithIndent(node , 0);
}


void free_tree(Node* node) {
    if (node == NULL) {
        return;
    }
    free_tree(node ->first_child);
    free_tree(node->next);
    if (node->free != NULL ) {
        node->free(node->val);
    }
    free(node);
}