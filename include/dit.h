#ifndef DIT_H
#define DIT_H

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/history.h>
#include <readline/readline.h>

#define DIT_INIT_ALLOC_PAGE_CAP 1
#define DIT_INIT_STACK_CAP 1
#define DIT_INIT_OP_CHAIN_CAP 1

// Forward Declarations

typedef struct dit_state Dit_State;
typedef struct dit_alloc_page Dit_AllocPage;
typedef struct dit_stack Dit_Stack;
typedef struct dit_data Dit_Data;
typedef enum dit_data_type Dit_DataType;
typedef struct dit_error Dit_Error;
typedef int Dit_ErrorType;
typedef struct dit_chunk Dit_Chunk;

typedef struct dit_parse_token Dit_ParseToken;
typedef enum dit_parse_type Dit_ParseType;
typedef struct dit_parse_op Dit_ParseOp;

// State Handling (state.c)

struct dit_state {
	Dit_AllocPage *mem;

	Dit_Stack *parse;
	Dit_Stack *stack;
	Dit_Stack *scope;
};

void dit_state_debug(const Dit_State *P);
Dit_State *dit_state_create(void);
Dit_State *dit_state_destroy(Dit_State *P);
bool dit_state_push_scope(Dit_State *P);
bool dit_state_pop_scope(Dit_State *P);
bool dit_state_return_scope(Dit_State *P, void *src, size_t size);
bool dit_state_scope_to_chunk(Dit_State *P);

// Memory Management (alloc.c)

struct dit_alloc_page {
	Dit_AllocPage *prev;
	size_t cap;
	size_t at;
	char raw[];
};

bool dit_alloc_page(Dit_State *P, size_t cap);
void dit_alloc_exit(Dit_State *P);
void *dit_alloc(Dit_State *P, size_t size);
void *dit_alloc_resize(Dit_State *P, void *ptr, size_t from, size_t to);

// Generic Stack Type (stack.c)

struct dit_stack {
	size_t base;
	size_t cap;
	size_t at;
	char raw[];
};

Dit_Stack *dit_stack_create(size_t cap);
Dit_Stack *dit_stack_destroy(Dit_Stack *stack);
bool dit_stack_push(Dit_Stack **stack_p, void *src, size_t size);
bool dit_stack_pop(Dit_Stack **stack_p, void *dest, size_t size);
bool dit_stack_drop(Dit_Stack **stack_p, size_t size);
void *dit_stack_peek(Dit_Stack *stack, size_t index, size_t size);

// Data Primitives (data.c)

enum dit_data_type {
	DIT_DATA_NIL,
	DIT_DATA_SM_ERROR,
	DIT_DATA_ERROR,
	DIT_DATA_NUM,
	DIT_DATA_STR,
	DIT_DATA_WORD,
	DIT_DATA_CHUNK,
};

struct dit_data {
	Dit_DataType type;
	union {
		void *ptr;
		Dit_ErrorType sm_error;
		Dit_Error *error;
		double num;
		char *str;
		char *word;
		Dit_Chunk *chunk;
	};
};

void dit_data_print(const Dit_Data value);

// Error Type (error.c)

enum dit_error_type {
	DIT_ERROR_ALLOC,
	DIT_ERROR_PARSE,
};

struct dit_error {
	Dit_ErrorType type;
	char ptr[];
};

void dit_error_type_print(Dit_ErrorType type);
void dit_error_print(const Dit_Error *error);

// String Type (str.c)

void dit_str_print(const char *str);

// Chunk Type (chunk.c)

struct dit_chunk {
	size_t len;
	Dit_Data value[];
};

Dit_Chunk *dit_chunk_create(Dit_State *P, Dit_Data *src, size_t len);
void dit_chunk_print(const Dit_Chunk *chunk);

// Parser (parse.c)

enum dit_parse_type {
	DIT_PARSE_ERROR_ALLOC,
	DIT_PARSE_ERROR_BAD_NUM,
	DIT_PARSE_ERROR_BAD_WORD,
	DIT_PARSE_ERROR_UNCLOSED_QUOTE,
	DIT_PARSE_ERROR_UNKNOWN_CHAR,
	DIT_PARSE_ERROR_OP_EXPECTED,
	DIT_PARSE_ERROR_EXPR_EXPECTED,
	DIT_PARSE_ERROR_OP_MISMATCH,

	DIT_PARSE_NIL,
	DIT_PARSE_NUM,
	DIT_PARSE_QUOTE,
	DIT_PARSE_WORD,
	DIT_PARSE_OP,
};

struct dit_parse_op {
	char *str;
	char *left;
	char *right;
};

struct dit_parse_token {
	const char *src;
	const char *end;
	Dit_ParseType type;
	union {
		void *ptr;
		double num;
		char *quote;
		char *word;
		Dit_ParseOp *op;
	};
};

#define dit_parse_error(SRC, END, TYPE) \
	((Dit_ParseToken){.src = SRC, .end = END, .type = TYPE})
#define dit_parse_num(SRC, END, NUM) \
	((Dit_ParseToken){           \
		.src = SRC, .end = END, .type = DIT_PARSE_NUM, .num = NUM})
#define dit_parse_value(SRC, END, TYPE, PTR) \
	((Dit_ParseToken){.src = SRC, .end = END, .type = TYPE, .ptr = PTR})

void dit_parse_error_print(const Dit_ParseToken token);
bool dit_parse(Dit_State *P, const char *src);

#endif
