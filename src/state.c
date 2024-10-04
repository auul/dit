#include <dit.h>

void dit_state_debug(const Dit_State *P)
{
	printf("\nStack: ");
	for (size_t i = P->stack->base; i < P->stack->at;
	     i += sizeof(Dit_Data)) {
		dit_data_print(*((Dit_Data *)(P->stack->raw + i)));
		if (i < P->stack->at - sizeof(Dit_Data)) {
			printf(" ");
		}
	}
	printf("\n\n");
}

Dit_State *dit_state_create(void)
{
	Dit_State *P = calloc(1, sizeof(Dit_State));
	if (!P) {
		return NULL;
	}

	dit_alloc_page(P, DIT_INIT_ALLOC_PAGE_CAP);
	P->parse = NULL;
	P->stack = dit_stack_create(DIT_INIT_STACK_CAP * sizeof(Dit_Data));
	P->scope = dit_stack_create(DIT_INIT_STACK_CAP * sizeof(size_t));
	if (!P->mem || !P->stack || !P->scope) {
		return dit_state_destroy(P);
	}

	return P;
}

Dit_State *dit_state_destroy(Dit_State *P)
{
	if (!P) {
		return NULL;
	}

	dit_alloc_exit(P);
	P->parse = dit_stack_destroy(P->parse);
	P->stack = dit_stack_destroy(P->stack);
	P->scope = dit_stack_destroy(P->scope);
	free(P);

	return NULL;
}

bool dit_state_push_scope(Dit_State *P)
{
	if (!dit_stack_push(&P->scope, &P->stack->base, sizeof(size_t))) {
		return false;
	}

	P->stack->base = P->stack->at;
	return true;
}

bool dit_state_pop_scope(Dit_State *P)
{
	size_t re_base;
	if (!dit_stack_pop(&P->scope, &re_base, sizeof(size_t))) {
		return false;
	}

	P->stack->at   = P->stack->base;
	P->stack->base = re_base;
	return true;
}

bool dit_state_return_scope(Dit_State *P, void *src, size_t size)
{
	return dit_state_pop_scope(P) && dit_stack_push(&P->stack, src, size);
}

bool dit_state_scope_to_chunk(Dit_State *P)
{
	Dit_Data retval = {
		.type  = DIT_DATA_CHUNK,
		.chunk = dit_chunk_create(
			P,
			(Dit_Data *)(P->stack->raw + P->stack->base),
			(P->stack->at - P->stack->base) / sizeof(Dit_Data))};
	return retval.chunk &&
	       dit_state_return_scope(P, &retval, sizeof(Dit_Data));
}
