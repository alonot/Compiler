#include "../include/includes.h"

String* init_string(char* val, int len) {
    if (len < 0) {
        len = strlen(val);  
    }
    String* str = (String*)(calloc(1,sizeof(String)));
    str->size = (len + 1) * 2;
    str->length = len;
    str->val = (char*)(malloc(sizeof(char) * str->size));
    str->val = strncpy(str->val, val, len);
    str->val[str->length ++] = '\0';
    return str;
}

void add_str(String* str, char* val, int len) {
    if (len < 0) {
        len = strlen(val);
        if (len != 0 && val[len - 1] == '\0') {
            len --;
        }
    }
    // fprintf(stdout,"%sAdd %d: %s\n", str->val, str->length,val);
    if (str->length + len >= str->size) {
        str->size = (str->length + len) * 2;
        str->val = (char*) (realloc(str->val, sizeof(char) * str->size));
    }
    strncpy(str->val + str->length - 1, val, len); // assumed that str.val[str.length - 1] == '\0'
    str->length += len - 1; 
    str->val[str->length ++] = '\0';
    // fprintf(stdout,"%s %d %d \n", str->val, str->length, len);
}

inline int length(String* str) {
    return str->length - 1;
}

void repeat_n_add(String* str, char val, int times) {
    if (str->length + times >= str->size) {
        str->size = (str->length + times) * 2;
        str->val = (char*) (realloc(str->val, sizeof(char) * str->size));
    }
    // fprintf(debug,"%s %d %d %c\n", str->val, str->length, times, val);
    memset(str->val + str->length - 1, val, times);
    str->length += times - 1;
    str->val[str->length ++] = '\0';
}


void freeString(String* str) {
    free(str->val);
    free(str);
}