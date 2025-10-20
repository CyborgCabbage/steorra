#include <SDL3/SDL_main.h>
#include "game.h"


int main(int argc, char* args[]) {
	Game game{};
	game.Run();
	return EXIT_SUCCESS;
}
