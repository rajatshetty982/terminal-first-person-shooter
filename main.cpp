#include <asm-generic/ioctls.h>
#include <chrono>
#include <csignal>
#include <iostream>
#include <sys/ioctl.h>

#include "Helper.h"
#include "LinuxTerminal.h"

using namespace std;
int main()
{
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
	const std::wstring map = L"##############.....#"
							 L"#..................#"
							 L"#..................#"
							 L"#..............#####"
							 L"#....#.............#"
							 L"#....#.............#"
							 L"#....#.............#"
							 L"#....#.............#"
							 L"#....####...#####..#"
							 L"#...........#......#"
							 L"#...........#......#"
							 L"#######.....#......#"
							 L"#..................#"
							 L"#..........#####...#"
							 L"#..................#"
							 L"#..................#"
							 L"#...#######........#"
							 L"#..................#"
							 L"#..................#"
							 L"####################";

	// make a screen buffer of char type
	// get a handle for screen buffer from linux
	// set the console buffer to the handle we have
	// when screen is resized we set the flag to true
	signal(SIGWINCH, HandleResize);

	// fill buffer values for intial screen size
	SetScreenToTerminalSize(&g_ScreenWidth, &g_ScreenHeight);
	auto timep1 = chrono::system_clock::now();
	auto timep2 = chrono::system_clock::now();

	// screen buffer
	std::vector<char> screen(g_ScreenWidth * g_ScreenHeight);

	while (1)
	{

		timep2 = chrono::system_clock::now();
		chrono::duration<float> elaspedT = timep2 - timep1;
		timep1 = timep2;
		float elaspedT_f = elaspedT.count();

		if (g_ScreenResized)
		{
			SetScreenToTerminalSize(&g_ScreenWidth, &g_ScreenHeight);
			screen.resize(g_ScreenWidth * g_ScreenHeight);
			ClearScreen();
			g_ScreenResized = false;
		}

		// get keyboard key presses
		// rotion movement
		// no mouse rn

		// A for rotating left (- x)
		// D for right (+ x)
		//
		// forward movement
		// collision
		//
		// backward movement
		// collision
		//  testing it out
		//  Fill background
		std::fill(screen.begin(), screen.end(), '.');

		// Draw border frame to visualize screen bounds
		for (int x = 0; x < g_ScreenWidth; ++x)
		{

			for (int y = 0; y < g_ScreenHeight; ++y)
			{
				if (x == 0 || x == g_ScreenWidth - 1)
				{
					screen[x + (y * g_ScreenWidth)] = '+';
				}
				else if (y == 0 || y == g_ScreenHeight - 1)
				{
					screen[x + (y * g_ScreenWidth)] = '-';
				}
				else
				{

					screen[x + (y * g_ScreenWidth)] = ' ';
				}
			}
		}

		// display stats

		// display map
		//
		// display frame
		DrawFrame(screen, g_ScreenWidth, g_ScreenHeight);
		//
	}
	linuxTerminalClose(true, g_OrigTermios);
	return 0;
}
