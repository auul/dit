#include <dit.h>

void dit_str_print(const char *str)
{
	int len;
	printf("\"");
	for (len = 0; str[len]; len++) {
		if (str[len] == '\\' || str[len] == '"') {
			printf("%.*s\\%c", len, str, str[len]);
			str += len;
			len = 0;
		}
	}
	printf("%.*s\"", len, str);
}
