#include <stdlib.h>
#include <stdbool.h>

#include "stack.h"

struct stack {
    void **items;
    int top_index; // -1 if empty
};


bool stack_is_empty(struct stack *s) {
    return s->top_index == -1;
}


// returned result is free'able
struct stack *stack_create() {
    struct stack *s = malloc(sizeof(struct stack));
    if(s == NULL) { return NULL; }
    s->top_index = -1;

    return s;
}

void stack_destroy(struct stack *s) {
    if(!stack_is_empty(s)) {
        for(int i = 0; i <= s->top_index; i++) {
            free(s->items[i]);
        }
    }

    free(s);
}


// Memory control is handed over to stack, what should be free'able
void stack_push(struct stack *s, void *what) {
    s->top_index++;
    s->items[s->top_index] = what;
}


// Memory control is handed over to caller, returned item will be free'able
void *stack_pop(struct stack *s) {
    if(stack_is_empty(s)) { return NULL; }

    void *item = s->items[s->top_index];
    s->top_index--;
    return item;
}


// Memory control is NOT handed over to caller.
void *stack_peek(struct stack *s) {
    if(stack_is_empty(s)) { return NULL; }

    return s->items[s->top_index];
}
