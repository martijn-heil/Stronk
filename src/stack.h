#ifndef STRONK_STACK_H
#define STRONK_STACK_H

struct stack;


struct stack *stack_create(); // returned result is free'able
void stack_push(struct stack *s, void *what); // Memory control is handed over to stack, what should be free'able
void *stack_pop(struct stack *s); // Memory control is handed over to caller, returned item will be free'able

bool stack_is_empty(struct stack *s);
void *stack_peek(struct stack *s); // Memory control is NOT handed over to caller.


#endif
