#pragma once


#include <asm-generic/ioctls.h>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <string>
#include <vector>
#include <sys/ioctl.h>
#include <unistd.h>

// Structures
struct Vec2f
{
	float x;
	float y;
};
struct Vec2i
{
	int x;
	int y;
};

struct Player
{
	Vec2f pos = {1.7f, 1.7f};
	float currentAngle = 0.0f;
	float fov = 1.0472f; // 60 deg
	float maxDepth = 20.0f;
	float moveSpeed = 19.0f;
	float turnSpeed = 8.0f;
	float sprintMultiplier = 1.8f;
};

struct RGBA
{
	uint8_t r, g, b, a;
};

struct Cell
{
	char character = 0;
	RGBA color{0, 0, 0, 1};
};

void HandleResize(int sig);
void SetScreenToTerminalSize(int *width, int *height);
// bool IsKeyPressed(char targetKey);
inline void TryMovePlayer(Vec2f &pos, float dx, float dy,
						  const std::wstring &map);

float Clamp(float v, float lo, float hi);
RGBA LerpRGB(const RGBA &a, const RGBA &b, float t);

void ProcessInputs(bool &running, Player &player, bool &miniMapToggle,
				bool &minimapKeyHeld, float dt, std::wstring &map,
				const int mapWidth, const int mapHeight);

RGBA WallColor(wchar_t wallType, bool darkSide);

void DrawMiniMap(std::vector<Cell> &screen, int screenWidth, int screenHeight,
				 const std::wstring &map, int mapWidth, int mapHeight,
				 const Vec2f &playerPos, float playerAngle);
