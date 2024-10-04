#include <dit.h>

void dit_data_print(const Dit_Data value)
{
	switch (value.type) {
	case DIT_DATA_NIL:
		printf("nil");
		break;
	case DIT_DATA_SM_ERROR:
		dit_error_type_print(value.sm_error);
		break;
	case DIT_DATA_ERROR:
		dit_error_print(value.error);
		break;
	case DIT_DATA_NUM:
		printf("%g", value.num);
		break;
	case DIT_DATA_STR:
		dit_str_print(value.str);
		break;
	case DIT_DATA_WORD:
		printf("%s", value.word);
		break;
	case DIT_DATA_CHUNK:
		dit_chunk_print(value.chunk);
		break;
	default:
		printf("%u:%p", value.type, value.ptr);
		break;
	}
}
