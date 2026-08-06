#include "Helper.h"
#include "LinuxTerminal.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>

// default screen dimensions, must be adjust accoring to the terminal size
int g_ScreenWidth = 1080;
int g_ScreenHeight = 720;
volatile bool g_ScreenResized = false;
termios_t g_OrigTermios{};

void HandleResize(int sig)
{
	g_ScreenResized = true;
}

void SetScreenToTerminalSize(int *width, int *height)
{
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
	{
		*width = w.ws_col;
		*height = w.ws_row;

		// TODO: remove globals
		g_ScreenWidth = w.ws_col;
		g_ScreenHeight = w.ws_row;
	}
}

void CrashHandler(int signum)
{
	linuxTerminalClose(true, g_OrigTermios);

	if (signum == SIGSEGV)
	{
		const char *msg = "\n[CRASH] Segmentation Fault detected!\n";
		write(STDERR_FILENO, msg, 37);
	}

	_Exit(signum);
}

// not used
bool IsKeyPressed(char targetKey)
{
	char ch;
	bool pressed = false;

	// Read all buffered input bytes without blocking
	while (read(STDIN_FILENO, &ch, 1) > 0)
	{
		if (ch == targetKey || ch == ::tolower(targetKey) ||
			ch == ::toupper(targetKey))
		{
			pressed = true;
		}
	}

	return pressed;
}

void ClearScreen()
{
	const char *clear = "\x1b[2J";
	// write(STDOUT_FILENO, clear, 4);
	std::cout << clear;
}

void MovePlayer(bool moveAhead, Vec2 &pos, float currentAngle, float speed,
				float elaspedT)
{
	float dir = moveAhead ? 1.0f : -1.0f;
	pos.x += sinf(currentAngle) * speed * elaspedT * dir;
	pos.y += cosf(currentAngle) * speed * elaspedT * dir;
}

void DrawMiniMap(std::vector<std::string> &screen, int screenWidth,
				 int screenHeight, const std::wstring &map, int mapWidth,
				 int mapHeight, const Vec2 &playerPos, float playerAngle)
{
	// Draw the 2D Map grid
	for (int y = 0; y < mapHeight; ++y)
	{

		for (int x = 0; x < mapWidth; ++x)
		{
			// Boundary safety check so map doesn't overflow screen buffer
			if (x >= screenWidth || y >= screenHeight)
				break;

			wchar_t tile = map[y * mapWidth + x];
			int bufferIndex = y * screenWidth + x;

			if (tile == L'#')
			{
				screen[bufferIndex] = "█"; // Wall
			}
			else
			{
				screen[bufferIndex] = "."; // Empty space
			}
		}
	}

	// Overlay the player position on top of the map
	int playerX = (int)playerPos.x;
	int playerY = (int)playerPos.y;

	if (playerX >= 0 && playerX < mapWidth && playerY >= 0 &&
		playerY < mapHeight)
	{
		int playerIndex = playerY * screenWidth + playerX;

		// Show directional arrow based on player angle
		// Angle normalized to [0, 2π)
		// although its crappy rn
		float angle = fmodf(playerAngle, 2.0f * M_PI);
		if (angle < 0)
			angle += 2.0f * M_PI;

		if (angle >= 0.25f * M_PI && angle < 0.75f * M_PI)
			screen[playerIndex] = "v"; // Down
		else if (angle >= 0.75f * M_PI && angle < 1.25f * M_PI)
			screen[playerIndex] = "<"; // Left
		else if (angle >= 1.25f * M_PI && angle < 1.75f * M_PI)
			screen[playerIndex] = "^"; // Up
		else
			screen[playerIndex] = ">"; // Right
	}
}

void DrawText(std::vector<std::string> &screen, int screenWidth,
			  int screenHeight, int startX, int startY, const std::string &text)
{
	if (startY < 0 || startY >= screenHeight)
		return;

	for (size_t i = 0; i < text.length(); ++i)
	{
		int targetX = startX + i;
		if (targetX >= 0 && targetX < screenWidth)
		{
			screen[startY * screenWidth + targetX] = std::string(1, text[i]);
		}
	}
}
