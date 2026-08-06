#include <algorithm>
#include <asm-generic/ioctls.h>
#include <chrono>
#include <clocale>
#include <cmath>
#include <csignal>
#include <cwchar>
#include <stdio.h>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <utility>

#define __STDC_WANT_LIB_EXT1__ 1
#include <cwchar>

#include "Helper.h"
#include "LinuxTerminal.h"
#include "Game.h"

// completely rebuilding this as there are many tricky bugs that is hard to catch in this mess of a codebase.
// its incomplete rn
int main()
{
	Game game;
	game.InitWindow("A cool fp!s", 240, 60);
	game.Run();
	// std::ios::sync_with_stdio(false);
	int width = g_ScreenWidth;
	int height = g_ScreenHeight;
	// Syncs the program's runtime encoding to your Linux system's native UTF-8
	// locale
	std::setlocale(LC_ALL, "");

	// ---- REGISTER THE CRASH INTERCEPTOR ----
	struct sigaction sa;
	sa.sa_handler = CrashHandler; // Point to our function
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	// Intercept crashes
	sigaction(SIGSEGV, &sa, nullptr);
	sigaction(SIGABRT, &sa, nullptr);
	// Intercept manual close commands
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
	// ----------------------------------------

	EnableRawMode(g_OrigTermios);

	// create a map
	const std::wstring map = L"####################"
							 L"#..................#"
							 L"#............#.....#"
							 L"#.#####......#.....#"
							 L"#............#.....#"
							 L"#............#.....#"
							 L"#.....####.........#"
							 L"#........#.........#"
							 L"#........#.........#"
							 L"#........#.........#"
							 L"#..................#"
							 L"#..................#"
							 L"#...........####...#"
							 L"#...........#......#"
							 L"#...........#......#"
							 L"#......######......#"
							 L"#..................#"
							 L"#..................#"
							 L"#..................#"
							 L"####################";

	signal(SIGWINCH, HandleResize);

	// fill buffer values for intial screen size
	SetScreenToTerminalSize(&width, &height);

	// screen buffer
	std::vector<std::string> screen(width * height, " ");
	Player player;

	auto timep1 = std::chrono::steady_clock::now();
	auto timep2 = std::chrono::steady_clock::now();
	bool running = true;

	while (running)
	{
		timep2 = std::chrono::steady_clock::now();
		std::chrono::duration<float> elaspedT = timep2 - timep1;
		timep1 = timep2;
		float elaspedT_f = elaspedT.count();

		if (g_ScreenResized)
		{
			SetScreenToTerminalSize(&width, &height);
			screen.resize(width * height);
			ClearScreen();
			g_ScreenResized = false;
			// continue;
		}

		bool aPressed = false;
		bool dPressed = false;
		bool wPressed = false;
		bool sPressed = false;
		bool qPressed = false;
		char ch;
		// Flush non-blocking input buffer for this frame
		while (read(STDIN_FILENO, &ch, 1) > 0) // FIXME: getting stuck here
		{
			// wsad movement keys
			if (ch == 'a' || ch == 'A')
				aPressed = true;
			if (ch == 'd' || ch == 'D')
				dPressed = true;
			if (ch == 'w' || ch == 'W')
				wPressed = true;
			if (ch == 's' || ch == 'S')
				sPressed = true;
			if (ch == 'q' || ch == 'Q')
				qPressed = true;
		}

		if (aPressed)
			player.currentAngle -= (player.turnSpeed) * elaspedT_f;

		if (dPressed)
			player.currentAngle += (player.turnSpeed) * elaspedT_f;

		if (wPressed)
		{
			MovePlayer(true, player.pos, player.currentAngle, player.speed,
					   elaspedT_f);
			if (map[(int)player.pos.y * c_MapWidth + (int)player.pos.x] ==
				'#') // collision
			{
				MovePlayer(false, player.pos, player.currentAngle, player.speed,
						   elaspedT_f);
			}
		}

		if (sPressed)
		{
			MovePlayer(false, player.pos, player.currentAngle, player.speed,
					   elaspedT_f);
			if (map[(int)player.pos.y * c_MapWidth + (int)player.pos.x] ==
				'#') // collision
			{
				MovePlayer(true, player.pos, player.currentAngle, player.speed,
						   elaspedT_f);
			}
		}
		if (qPressed)
			running = false;

		// clear the screen before drawing
		// like how we do glClear in opengl
		// std::fill(screen.begin(), screen.end(), " "); // no need ig, looks
		// clean to me, maybe will test properly later

		// now the main part
		// we cast our rays at every column (along the x axis)
		// and check if it hits a wall (i.e. the char '#' from our map)
		// when it does hit, get the distance from the plyaer to the wall and
		// render the specific text for the wall/sky/floor according to the
		// distance
		// seems simple!!!

		auto tRaycastStart = std::chrono::steady_clock::now();
		for (int x = 0; x < width; ++x)
		{
			float rayAngle = (player.currentAngle - player.fov / 2.0f) +
							 ((float)x / width) * player.fov;

			float distToWall = 0;
			bool hitTheWall = false;
			bool boundary = false;

			float eyeX = sinf(rayAngle);
			float eyeY = cosf(rayAngle);

			while (!hitTheWall && distToWall < player.maxDepth)
			{
				distToWall += 0.1f;
				int rayX = player.pos.x + eyeX * distToWall;
				int rayY = player.pos.y + eyeY * distToWall;
				// int testX = player.pos.x + distToWall;
				// int testY = player.pos.y + distToWall;

				// if current ray point is out of our map bound set it to max
				// depth allowed max depth tied to the player seems a bit weird
				// tho
				if (rayX < 0 || rayX >= c_MapWidth || rayY < 0 ||
					rayY >= c_MapHeight)
				{
					hitTheWall = true;
					distToWall = player.maxDepth;
				}
				// check if the ray hit the wall
				else
				{
					if (map[rayY * c_MapWidth + rayX] == '#')
					{

						hitTheWall = true;

						std::vector<std::pair<float, float>> pair;

						for (int tx = 0; tx < 2; ++tx)
						{
							for (int ty = 0; ty < 2; ++ty)
							{
								float vy = (float)rayY + ty - player.pos.y;
								float vx = (float)rayX + tx - player.pos.x;
								float distance = sqrt(vx * vx + vy * vy);
								float dot = (eyeX * vx / distance) +
											(eyeY * vy / distance);
								pair.push_back(std::make_pair(distance, dot));
							}
						}
						// sort pairs from closest to farther
						std::sort(pair.begin(), pair.end(),
								  [](const std::pair<float, float> &left,
									 const std::pair<float, float> &right)
								  { return left.first < right.first; });

						float bound = 0.001;
						if (acos(pair.at(0).second) < bound)
							boundary = true;
						if (acos(pair.at(1).second) < bound)
							boundary = true;
						if (acos(pair.at(2).second) < bound)
							boundary = true;
					}
				}
			}

			// calculate the floor and the ceiling height
			float correctedDist =
				distToWall * cosf(rayAngle - player.currentAngle);

			// the farther the wall is, more of the ceiling is visible
			// basically this checks from the middle of screen the ratio of ht
			// to dist and we get a good approximate how big the walls would
			// look at that dist
			// int ceiling =
			// 	(float)(height / 2.0f) - height / (float)correctedDist;
			int ceiling = (float)(height / 2.0f) -
						  (c_AspectRatio * height) /
							  correctedDist; // need a guard for near 0 dist
			// what is not the ceiling is a floor, ofcourse ignoring the wall
			// until we think about it
			int floor = height - ceiling;

			// clamp ceiling and floor
			if (ceiling < 0)
				ceiling = 0;
			if (floor >= height)
				floor = height - 1;

			std::string shade = " ";
			// draw walls
			// closer one's are more clearly visible
			if (distToWall <= player.maxDepth / 4.0f)
				shade = "█"; // Full block
			else if (distToWall < player.maxDepth / 3.0f)
				shade = "▓"; // Dark shade
			else if (distToWall < player.maxDepth / 2.0f)
				shade = "▒"; // Medium shade
			else if (distToWall < player.maxDepth)
				shade = "░"; // Light shade
			else
				shade = " "; // Too far// too far can't see a damn thing

			// shade boundary
			if (boundary)
				shade = " ";

			for (int y = 0; y < height; ++y)
			{
				if (y < ceiling)
					screen[width * y + x] = " ";
				else if (y >= ceiling && y <= floor)
					screen[width * y + x] = shade;
				else
				{
					// float floorPoint = 1.0f - ((float)y - height /
					// player.maxDepth) /
					// ((float)height / 2.0f);
					float floorPoint = (float)y / (float)height;
					if (floorPoint < 0.3f)
						shade = " ";
					else if (floorPoint < 0.5f)
						shade = ".";
					else if (floorPoint < 0.7f)
						shade = "-";
					else if (floorPoint < 0.9f)
						shade = "x";
					else
						shade = "#";

					screen[width * y + x] = shade;
				}
			}
		}
		auto tRaycastEnd = std::chrono::steady_clock::now();

		float raycastMs =
			std::chrono::duration<float, std::milli>(tRaycastEnd - tRaycastStart)
				.count();
		static float drawMs = 0;
		// stats text — uses timings from THIS frame's raycast, and the drawMs
		// from the PREVIOUS frame's draw (that's fine and expected, see note
		// below)
		char buf[161];
		snprintf(buf, sizeof(buf),
				 "FPS: %4.1f | Pos: (%4.1f, %4.1f) | Angle: %4.1f deg | "
				 "raycastMs: %4.1f | drawMs: %4.1f",
				 1.0f / elaspedT_f, player.pos.x, player.pos.y,
				 player.currentAngle * (180.0f / 3.14159265f), raycastMs,
				 drawMs);

		int statsY = c_MapHeight + 1;
		DrawText(screen, width, height, 0, statsY, buf);
		DrawText(screen, width, height, 0, statsY + 1,
				 "Controls: [W/S] Move | [A/D] Turn");

		// NOW draw, once, with everything already baked into screen[]
		auto tDrawStart = std::chrono::steady_clock::now();
		DrawFrame(screen, width, height);
		auto tDrawEnd = std::chrono::steady_clock::now();
		drawMs =
			std::chrono::duration<float, std::milli>(tDrawEnd - tDrawStart).count();

		// pacing
		constexpr auto targetFrameTime =
			std::chrono::milliseconds(7); // ~144fps
		auto frameElapsed = std::chrono::steady_clock::now() - timep2;
		if (frameElapsed < targetFrameTime)
			std::this_thread::sleep_for(targetFrameTime - frameElapsed);
	}
	linuxTerminalClose(true, g_OrigTermios);
	return 0;
}
