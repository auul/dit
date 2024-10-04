#include <dit.h>

int main(int argc, char **args)
{
	if (argc < 2) {
		return 0;
	}

	Dit_State *P = dit_state_create();
	dit_parse(P, args[1]);
	dit_state_debug(P);
	dit_state_destroy(P);

	return 0;
}
