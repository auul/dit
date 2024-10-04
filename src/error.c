#include <dit.h>

void dit_error_type_print(Dit_ErrorType type)
{
	switch (type) {
	case DIT_ERROR_ALLOC:
		printf("Error allocating memory");
		break;
	case DIT_ERROR_PARSE:
		printf("Error parsing input");
		break;
	default:
		printf("Unknown error %u", type);
		break;
	}
}

void dit_error_print(const Dit_Error *error)
{
	dit_error_type_print(error->type);
	printf(": ");

	switch (error->type) {
	case DIT_ERROR_PARSE:
		dit_parse_error_print(*((Dit_ParseToken *)(error->ptr)));
		break;
	default:
		break;
	}
	printf("\n");
}
