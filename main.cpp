
#include "Game.h"

// completely rebuilding this as there are many tricky bugs that is hard to catch in this mess of a codebase.
// its incomplete rn
int main()
{
	Game game("A cool fp!s", 240, 60);
	game.Init();
	game.Run();

	return 0;
}
