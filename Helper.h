#pragma once

#include <asm-generic/ioctls.h>
#include <csignal>
#include <sys/ioctl.h>
#include <unistd.h>

#include "LinuxTerminal.h"

extern termios_t g_OrigTermios;

// Screen dimensions and state
extern int g_ScreenWidth;
extern int g_ScreenHeight;
extern volatile bool g_ScreenResized;
// World size
constexpr int c_MapWidth = 20;
constexpr int c_MapHeight = 20;
constexpr float c_AspectRatio = 2.0f;

// Structures
struct Vec2
{
	float x;
	float y;
};

struct Player
{
	Vec2 pos = {1.7f, 1.7f};
	float currentAngle = 0.0f;
	float fov = 3.14159f / 4.0f;
	float maxDepth = 20.0f;
	float speed = 19.0f;
	float turnSpeed = 8.0f;
};

void HandleResize(int sig);
void SetScreenToTerminalSize(int *width, int *height);
void CrashHandler(int signum);
// bool IsKeyPressed(char targetKey);
void ClearScreen();
void MovePlayer(bool moveAhead, Vec2 &pos, float currentAngle, float speed,
				float elaspedT);
void DrawText(std::vector<std::string> &screen, int screenWidth,
			  int screenHeight, int startX, int startY,
			  const std::string &text);
