#include <dit.h>

Dit_Chunk *dit_chunk_create(Dit_State *P, Dit_Data *src, size_t len)
{
	Dit_Chunk *chunk =
		dit_alloc(P, sizeof(Dit_Chunk) + len * sizeof(Dit_Data));
	if (!chunk) {
		return NULL;
	}

	chunk->len = len;
	memcpy(chunk->value, src, len * sizeof(Dit_Data));

	return chunk;
}

void dit_chunk_print(const Dit_Chunk *chunk)
{
	printf("{");
	for (size_t i = 0; i < chunk->len; i++) {
		dit_data_print(chunk->value[i]);
		if (i < chunk->len - 1) {
			printf(" ");
		}
	}
	printf("}");
}
