#include <dit.h>

Dit_Stack *dit_stack_create(size_t cap)
{
	Dit_Stack *stack = malloc(sizeof(Dit_Stack) + cap);
	if (!stack) {
		return NULL;
	}

	stack->base = 0;
	stack->cap  = cap;
	stack->at   = 0;

	return stack;
}

static Dit_Stack *stack_expand(Dit_Stack *stack, size_t request)
{
	size_t re_cap = 2 * stack->cap;
	if (re_cap < stack->at + request) {
		re_cap = stack->at + request;
	}

	stack = realloc(stack, sizeof(Dit_Stack) + re_cap);
	if (!stack) {
		return NULL;
	}

	stack->cap = re_cap;

	return stack;
}

Dit_Stack *dit_stack_destroy(Dit_Stack *stack)
{
	if (!stack) {
		return NULL;
	}

	free(stack);

	return NULL;
}

bool dit_stack_push(Dit_Stack **stack_p, void *src, size_t size)
{
	Dit_Stack *stack = *stack_p;
	if (stack->cap < stack->at + size) {
		stack = stack_expand(stack, size);
		if (!stack) {
			return false;
		}
		*stack_p = stack;
	}

	memcpy(stack->raw + stack->at, src, size);
	stack->at += size;

	return true;
}

bool dit_stack_pop(Dit_Stack **stack_p, void *dest, size_t size)
{
	Dit_Stack *stack = *stack_p;
	if (size > stack->at - stack->base) {
		stack->at = stack->base;
		memset(dest, 0, size);
		return false;
	}

	stack->at -= size;
	memcpy(dest, stack->raw + stack->at, size);

	return true;
}

bool dit_stack_drop(Dit_Stack **stack_p, size_t size)
{
	Dit_Stack *stack = *stack_p;
	if (size > stack->at - stack->base) {
		stack->at = stack->base;
		return false;
	}

	stack->at -= size;
	return true;
}

void *dit_stack_peek(Dit_Stack *stack, size_t index, size_t size)
{
	if (stack->at - stack->base < (index + 1) * size) {
		return NULL;
	}
	return stack->raw + stack->at - ((index + 1) * size);
}
