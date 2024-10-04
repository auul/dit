#include <dit.h>

#define PREC_FLAG (UINTPTR_MAX & ~(UINTPTR_MAX >> 1))
#define op_prec(value) (char *)(PREC_FLAG | (uintptr_t)(value))
#define is_prec(ptr) (PREC_FLAG & (uintptr_t)(ptr))
#define as_prec(ptr) (~PREC_FLAG & (uintptr_t)(ptr))
#define as_fn(ptr) ((char *)(ptr))
#define NONOP ((char *)(UINTPTR_MAX))

static bool paren_close(Dit_State *P)
{
	dit_stack_drop(&P->stack, sizeof(Dit_Data));

	Dit_Data *head   = dit_stack_peek(P->stack, 0, sizeof(Dit_Data));
	Dit_Chunk *chunk = head->chunk;
	if (chunk->len > 0 &&
	    chunk->value[chunk->len - 1].type == DIT_DATA_WORD &&
	    !strcmp(chunk->value[chunk->len - 1].word, ",")) {
		while (chunk->len > 0 &&
		       chunk->value[chunk->len - 1].type == DIT_DATA_WORD &&
		       !strcmp(chunk->value[chunk->len - 1].word, ",")) {
			chunk->len -= 1;
		}
		return true;
	}
	dit_stack_drop(&P->stack, sizeof(Dit_Data));

	return dit_stack_push(
		&P->stack, chunk->value, chunk->len * sizeof(Dit_Data));
}

static bool curly_close(Dit_State *P)
{
	dit_stack_drop(&P->stack, sizeof(Dit_Data));

	Dit_Data *head   = dit_stack_peek(P->stack, 0, sizeof(Dit_Data));
	Dit_Chunk *chunk = head->chunk;
	if (chunk->len > 0 &&
	    chunk->value[chunk->len - 1].type == DIT_DATA_WORD &&
	    !strcmp(chunk->value[chunk->len - 1].word, ",")) {
		while (chunk->len > 0 &&
		       chunk->value[chunk->len - 1].type == DIT_DATA_WORD &&
		       !strcmp(chunk->value[chunk->len - 1].word, ",")) {
			chunk->len -= 1;
		}
	}

	return true;
}

Dit_ParseOp op_list[] = {
	{   ",",         op_prec(0),    op_prec(0)},
	{   "+",         op_prec(5),    op_prec(5)},
	{   "-",         op_prec(5),    op_prec(6)},
	{   "*",         op_prec(7),    op_prec(7)},
	{   "/",         op_prec(7),    op_prec(8)},
	{   "(",	      NONOP,         ")\0"},
	{   ")", as_fn(paren_close),         NONOP},
	{   "{",	      NONOP,         "}\0"},
	{   "}", as_fn(curly_close),         NONOP},
	{  "if",              NONOP,      "then\0"},
	{"then",               NULL, "else\0end\0"},
	{"else",               NULL,       "end\0"},
	{ "end",               NULL,         NONOP},
	{  NULL,	       NULL,          NULL}
};

static inline bool is_error(Dit_ParseToken token)
{
	return token.type < DIT_PARSE_NIL;
}

void dit_parse_error_print(const Dit_ParseToken token)
{
	switch (token.type) {
	case DIT_PARSE_ERROR_ALLOC:
		printf("Could not allocate memory");
		break;
	case DIT_PARSE_ERROR_BAD_NUM:
		printf("Error parsing number: %.*s",
		       (int)(token.end - token.src),
		       token.src);
		break;
	case DIT_PARSE_ERROR_BAD_WORD:
		printf("Error parsing word: %.*s",
		       (int)(token.end - token.src),
		       token.src);
		break;
	case DIT_PARSE_ERROR_UNCLOSED_QUOTE:
		printf("Unclosed quote: %.*s",
		       (int)(token.end - token.src),
		       token.src);
		break;
	case DIT_PARSE_ERROR_UNKNOWN_CHAR:
		printf("Invalid character: %.*s",
		       (int)(token.end - token.src),
		       token.src);
		break;
	case DIT_PARSE_ERROR_OP_EXPECTED:
		printf("Operator expected: %.*s",
		       (int)(token.end - token.src),
		       token.src);
		break;
	default:
		if (token.src) {
			printf("(error %u %.*s)",
			       token.type,
			       (int)(token.end - token.src),
			       token.src);
		} else {
			printf("(error %u)", token.type);
		}
		break;
	}
}

void dit_parse_token_print(const Dit_ParseToken token)
{
	switch (token.type) {
	case DIT_PARSE_NIL:
		printf("(nil)");
		break;
	case DIT_PARSE_NUM:
		printf("(num %g)", token.num);
		break;
	case DIT_PARSE_QUOTE:
		printf("(quote ");
		dit_str_print(token.quote);
		printf(")");
		break;
	case DIT_PARSE_WORD:
		printf("(word %s)", token.word);
		break;
	case DIT_PARSE_OP:
		printf("(op %s)", token.op->str);
		break;
	default:
		if (is_error(token)) {
			dit_parse_error_print(token);
		} else {
			printf("(unknown %u)", token.type);
		}
		break;
	}
}

static bool is_partial_match(const char *target, const char *src)
{
	for (size_t i = 0; target[i]; i++) {
		if (target[i] != src[i]) {
			return false;
		}
	}
	return true;
}

static bool is_punct(const char *str)
{
	for (size_t i = 0; str[i]; i++) {
		if (isalnum(str[i])) {
			return false;
		}
	}
	return true;
}

static const char *get_punct(const char *src)
{
	for (size_t i = 0; op_list[i].str; i++) {
		if (is_partial_match(op_list[i].str, src) &&
		    is_punct(op_list[i].str)) {
			return op_list[i].str;
		}
	}
	return NULL;
}

static bool is_delim(const char *src)
{
	return !*src || isspace(*src) || get_punct(src);
}

static Dit_ParseOp *get_op(const char *src)
{
	for (size_t i = 0; op_list[i].str; i++) {
		if (is_partial_match(op_list[i].str, src) &&
		    (isalnum(src[strlen(op_list[i].str)]) ||
		     src[strlen(op_list[i].str)] == '_' ||
		     is_delim(src + strlen(op_list[i].str)))) {
			return &op_list[i];
		}
	}
	return NULL;
}

static Dit_ParseToken parse_num(const char *src)
{
	double left    = 0.0;
	double right   = 0.0;
	unsigned place = 0;
	bool dot       = false;

	size_t len;
	for (len = 0; src[len] == '.' || !is_delim(src + len); len++) {
		if (!isdigit(src[len])) {
			if (src[len] != '.' || dot) {
				return dit_parse_error(src,
				                       src + len + 1,
				                       DIT_PARSE_ERROR_BAD_NUM);
			}
			dot = true;
		} else if (dot) {
			right = (10.0 * right) + (double)(src[len] - '0');
			place++;
		} else {
			left = (10.0 * left) + (double)(src[len] - '0');
		}
	}

	while (place) {
		right /= 10.0;
		place--;
	}

	return dit_parse_num(src, src + len, left + right);
}

static Dit_ParseToken parse_quote(Dit_State *P, const char *src)
{
	char quote_char  = src[0];
	size_t esc_count = 0;

	size_t len;
	for (len = 1; src[len] != quote_char; len++) {
		if (src[len] == '\\') {
			esc_count++;
			len++;
		}

		if (!src[len]) {
			return dit_parse_error(
				src, src + len, DIT_PARSE_ERROR_UNCLOSED_QUOTE);
		}
	}
	len++;

	size_t dest_len = (len - 2) - esc_count;
	char *dest      = dit_alloc(P, dest_len + 1);
	if (!dest) {
		return dit_parse_error(NULL, NULL, DIT_PARSE_ERROR_ALLOC);
	}
	char *trace = dest;

	for (size_t i = 1; i < len - 1; i++) {
		if (src[i] == '\\') {
			i++;
		}

		*trace = src[i];
		trace++;
	}
	*trace = 0;

	return dit_parse_value(src, src + len, DIT_PARSE_QUOTE, dest);
}

static inline bool is_word_char(char c)
{
	return c == '_' || isalnum(c);
}

static Dit_ParseToken parse_word(Dit_State *P, const char *src)
{
	size_t len;
	for (len = 0; !is_delim(src + len); len++) {
		if (!is_word_char(src[len])) {
			return dit_parse_error(
				src, src + len + 1, DIT_PARSE_ERROR_BAD_WORD);
		}
	}

	char *dest = dit_alloc(P, len + 1);
	if (!dest) {
		return dit_parse_error(NULL, NULL, DIT_PARSE_ERROR_ALLOC);
	}
	memcpy(dest, src, len);
	dest[len] = 0;

	return dit_parse_value(src, src + len, DIT_PARSE_WORD, dest);
}

static inline bool is_num(const char *src)
{
	return isdigit(src[0]) || (src[0] == '.' && isdigit(src[1]));
}

static inline bool is_quote(const char *src)
{
	return src[0] == '\'' || src[0] == '"';
}

static inline bool is_word(const char *src)
{
	return src[0] == '_' || isalpha(src[0]);
}

static Dit_ParseToken parse_token(Dit_State *P, const char *src)
{
	if (is_num(src)) {
		return parse_num(src);
	} else if (is_quote(src)) {
		return parse_quote(P, src);
	}

	Dit_ParseOp *op = get_op(src);
	if (op) {
		return dit_parse_value(
			src, src + strlen(op->str), DIT_PARSE_OP, op);
	} else if (is_word(src)) {
		return parse_word(P, src);
	} else {
		return dit_parse_error(
			src, src + 1, DIT_PARSE_ERROR_UNKNOWN_CHAR);
	}
}

static Dit_ParseOp *
parse_op_join(Dit_State *P, Dit_ParseOp *left, Dit_ParseOp *right)
{
	size_t left_str_len  = strlen(left->str);
	size_t right_str_len = strlen(right->str);

	Dit_ParseOp *op = dit_alloc(P, sizeof(Dit_ParseOp));
	if (!op) {
		return NULL;
	}

	op->str = dit_alloc(P, left_str_len + right_str_len + 2);
	if (!op->str) {
		return NULL;
	}

	memcpy(op->str, left->str, left_str_len);
	op->str[left_str_len] = '#';
	memcpy(op->str + left_str_len + 1, right->str, right_str_len);
	op->str[left_str_len + right_str_len + 1] = 0;

	op->left  = left->left;
	op->right = right->right;

	return op;
}

static char *parse_op_return(Dit_State *P, Dit_ParseOp *op)
{
	size_t len;
	for (len = 0; op->str[len]; len++) {
		if (op->str[len] == '#') {
			return op->str;
		}
	}

	char *dest = dit_alloc(P, len + 1);
	if (!dest) {
		return NULL;
	}

	memcpy(dest, op->str, len);
	dest[len] = 0;

	return dest;
}

static Dit_Data parse_error_return(Dit_State *P, Dit_ParseToken token)
{
	Dit_Data retval;
	switch (token.type) {
	case DIT_PARSE_ERROR_ALLOC:
		retval.type     = DIT_DATA_SM_ERROR;
		retval.sm_error = DIT_ERROR_ALLOC;
		break;
	default:
		retval.error = dit_alloc(
			P, sizeof(Dit_Error) + sizeof(Dit_ParseToken));
		if (retval.error) {
			retval.type        = DIT_DATA_ERROR;
			retval.error->type = DIT_ERROR_PARSE;
			memcpy(retval.error->ptr,
			       &token,
			       sizeof(Dit_ParseToken));
		} else {
			retval.type     = DIT_DATA_SM_ERROR;
			retval.sm_error = DIT_ERROR_ALLOC;
		}
		break;
	}
	return retval;
}

static Dit_Data parse_token_return(Dit_State *P, Dit_ParseToken token)
{
	Dit_Data retval;
	switch (token.type) {
	case DIT_PARSE_NIL:
		retval.type = DIT_DATA_NIL;
		retval.ptr  = NULL;
		break;
	case DIT_PARSE_NUM:
		retval.type = DIT_DATA_NUM;
		retval.num  = token.num;
		break;
	case DIT_PARSE_QUOTE:
		retval.type = DIT_DATA_STR;
		retval.str  = token.quote;
		break;
	case DIT_PARSE_WORD:
		retval.type = DIT_DATA_WORD;
		retval.word = token.word;
		break;
	case DIT_PARSE_OP:
		retval.type = DIT_DATA_WORD;
		retval.word = parse_op_return(P, token.op);
		break;
	default:
		if (is_error(token)) {
			retval = parse_error_return(P, token);
		} else {
			assert(false);
			exit(1);
		}
	}
	return retval;
}

static const char *skip_space(const char *src)
{
	while (isspace(*src)) {
		src++;
	}
	return src;
}

static bool skip_space_passes_newline(const char **src_p)
{
	const char *src = *src_p;
	while (isspace(*src)) {
		if (*src == '\n') {
			*src_p = skip_space(src);
			return true;
		}
		src++;
	}
	*src_p = src;
	return false;
}

static inline bool is_op_expected(const Dit_ParseToken *token,
                                  const Dit_ParseToken *next)
{
	return token->type != DIT_PARSE_OP && next->type != DIT_PARSE_OP;
}

static inline bool is_nonop(const Dit_ParseToken token)
{
	switch (token.type) {
	case DIT_PARSE_OP:
		return token.op->left == NONOP && token.op->right == NONOP;
	default:
		return true;
	}
}

static inline Dit_ParseToken *get_current_token(Dit_State *P)
{
	return *((Dit_ParseToken **)(P->parse->raw));
}

static inline void set_current_token(Dit_State *P, void *ptr)
{
	*((Dit_ParseToken **)(P->parse->raw)) = ptr;
}

static bool get_next_token(Dit_State *P)
{
	Dit_ParseToken *token = get_current_token(P);
	if (!token) {
		return true;
	}

	const char *src = token->end;
	bool newline    = skip_space_passes_newline(&src);
	if (!*src) {
		set_current_token(P, NULL);
		return true;
	}

	Dit_ParseToken next = parse_token(P, src);
	if (is_error(next)) {
		*token = next;
		return false;
	}

	if (is_op_expected(token, &next)) {
		if (newline) {
			token->src  = token->end;
			token->type = DIT_PARSE_OP;
			token->op   = get_op(",");
		} else {
			token->end  = next.end;
			token->type = DIT_PARSE_ERROR_OP_EXPECTED;
			return false;
		}
	} else {
		*token = next;
	}

	return true;
}

static bool do_in_to_out(Dit_State *P)
{
	Dit_ParseToken *token = get_current_token(P);
	Dit_Data retval       = parse_token_return(P, *token);
	if (!dit_stack_push(&P->stack, &retval, sizeof(Dit_Data))) {
		token->type = DIT_PARSE_ERROR_ALLOC;
		return false;
	}
	return get_next_token(P);
}

static bool do_in_to_hold(Dit_State *P)
{
	Dit_ParseToken *token = get_current_token(P);
	bool retval = dit_stack_push(&P->parse, token, sizeof(Dit_ParseToken));
	if (!retval ||
	    (!is_prec(token->op->right) && !dit_state_push_scope(P))) {
		token->type = DIT_PARSE_ERROR_ALLOC;
		return false;
	}
	return get_next_token(P);
}

static bool do_hold_to_out(Dit_State *P)
{
	Dit_ParseToken stack_token;
	dit_stack_pop(&P->parse, &stack_token, sizeof(Dit_ParseToken));
	if (!is_prec(stack_token.op->right)) {
		Dit_ParseToken *token = get_current_token(P);
		token->src            = stack_token.src;
		token->end            = stack_token.end;
		token->type           = DIT_PARSE_ERROR_OP_EXPECTED;
		return false;
	}

	Dit_Data retval = parse_token_return(P, stack_token);
	return dit_stack_push(&P->stack, &retval, sizeof(Dit_Data));
}

static bool do_join_to_hold(Dit_State *P)
{
	Dit_ParseToken *token = get_current_token(P);
	Dit_ParseToken *prev =
		dit_stack_peek(P->parse, 0, sizeof(Dit_ParseToken));

	if (!dit_state_scope_to_chunk(P)) {
		token->type = DIT_PARSE_ERROR_ALLOC;
		return false;
	}

	bool (*close_fn)(Dit_State *) = (void *)(token->op->left);

	prev->op = parse_op_join(P, prev->op, token->op);
	if (!prev->op) {
		token->type = DIT_PARSE_ERROR_ALLOC;
		return false;
	} else if (prev->op->right == NONOP) {
		if (!do_hold_to_out(P)) {
			return false;
		}
	} else if (!is_prec(prev->op->right) && !dit_state_push_scope(P)) {
		token->type = DIT_PARSE_ERROR_ALLOC;
		return false;
	}

	if (close_fn && !close_fn(P)) {
		token->type = DIT_PARSE_ERROR_ALLOC;
		return false;
	}

	return get_next_token(P);
}

static bool is_op_match(const char *left, const char *right)
{
	while (*left) {
		if (!strcmp(left, right)) {
			return true;
		}
		while (*left) {
			left++;
		}
		left++;
	}
	return false;
}

static bool do_parse_action(Dit_State *P)
{
	Dit_ParseToken *token = get_current_token(P);
	Dit_ParseToken *hold  = NULL;
	if (P->parse->at > P->parse->base) {
		hold = dit_stack_peek(P->parse, 0, sizeof(Dit_ParseToken));
	}

	if (is_nonop(*token)) {
		return do_in_to_out(P);
	} else if (!is_prec(token->op->left)) {
		if (!hold) {
			token->type = DIT_PARSE_ERROR_EXPR_EXPECTED;
			return false;
		} else if (!is_prec(hold->op->right)) {
			if (is_op_match(hold->op->right, token->op->str)) {
				return do_join_to_hold(P);
			}
			token->src  = hold->src;
			token->type = DIT_PARSE_ERROR_OP_MISMATCH;
			return false;
		}
		return do_hold_to_out(P);
	} else if (!hold || !is_prec(hold->op->right)) {
		return do_in_to_hold(P);
	} else if (as_prec(hold->op->right) > as_prec(token->op->left)) {
		return do_hold_to_out(P);
	} else {
		return do_in_to_hold(P);
	}
}

static bool parse_fail(Dit_State *P, size_t re_scope)
{
	Dit_ParseToken *token = get_current_token(P);
	P->parse              = dit_stack_destroy(P->parse);
	P->scope->at          = re_scope;
	dit_state_pop_scope(P);

	Dit_Data retval = parse_token_return(P, *token);
	return dit_stack_push(&P->stack, &retval, sizeof(Dit_Data));
}

bool dit_parse(Dit_State *P, const char *src)
{
	if (!dit_state_push_scope(P)) {
		return false;
	}
	size_t re_scope = P->scope->at;

	P->parse =
		dit_stack_create(sizeof(Dit_ParseToken *) +
	                         DIT_INIT_STACK_CAP * sizeof(Dit_ParseToken));
	if (!P->parse) {
		return false;
	}

	Dit_ParseToken token = {.src  = src,
	                        .end  = src,
	                        .type = DIT_PARSE_OP,
	                        .op   = get_op(",")};
	P->parse->base       = sizeof(Dit_ParseToken);
	P->parse->at         = P->parse->base;
	set_current_token(P, &token);
	if (!get_next_token(P)) {
		return parse_fail(P, re_scope);
	}

	while (get_current_token(P)) {
		if (!do_parse_action(P)) {
			return parse_fail(P, re_scope);
		}
	}
	set_current_token(P, &token);

	while (P->parse->at > P->parse->base) {
		if (!do_hold_to_out(P)) {
			return parse_fail(P, re_scope);
		}
	}

	P->parse     = dit_stack_destroy(P->parse);
	P->scope->at = re_scope;
	return dit_state_scope_to_chunk(P);
}
