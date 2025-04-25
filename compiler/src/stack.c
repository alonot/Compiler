#include "../include/includes.h"

Stack* init_stack() {
    Stack* st = (Stack*) calloc(1, sizeof(Stack));
    st->size = 1;
    st->values = (lli*)malloc(sizeof(lli));
    return st;
}

void push_stack(Stack* st, lli val) {
    if (st->top == st->size) {
        st->size *= 2;
        st->values = (lli*) (realloc(st->values, sizeof(lli) * st->size));
    }
    if (val == LLONG_MIN) {
        return;
    }

    st->values[st->top++] = val;
}

lli pop_stack(Stack* st) {
    if (st->top == 0) {
        return LLONG_MIN;
    }
    // fprintf(debug,"pop %d\n", st->top);
    lli val = st->values[--st->top ];
    if (st->top != 1 && st->top < st->size / 2 ) {
        st->size /= 2;
        st->values = (lli*) (realloc(st->values, sizeof(lli) * st->size));
    }
    return val;
}

lli top_stack(Stack* st) {
    if (st->top == 0) {
        return LLONG_MIN;
    }
    // fprintf(debug,"pop %d\n", st->top);
    lli val = st->values[st->top - 1];
    return val;
}

void free_stack(Stack* st) {
    free(st->values);
    free(st);
}