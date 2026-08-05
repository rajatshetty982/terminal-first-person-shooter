#pragma once

#include <asm-generic/ioctls.h>
#include <csignal>
#include <sys/ioctl.h>
#include <unistd.h>

#include "LinuxTerminal.h"

static termios_t g_OrigTermios;

// Screen dimensions and state
extern int g_ScreenWidth;
extern int g_ScreenHeight;
extern volatile bool
	g_ScreenResized;
// World size
constexpr int c_MapWidth = 20;
constexpr int c_MapHeight = 20;

// Structures
struct Vec2
{
	float x;
	float y;
};

struct Player
{
	Vec2 pos = {13.7f, 5.09f};
	float currentRotaion = 0.0f;
	float fov = 3.14159f / 4.0f;
	float depth = 16.0f;
	float speed = 5.0f;
};

void HandleResize(int sig);
void SetScreenToTerminalSize(int *width, int *height);
void CrashHandler(int signum);
bool IsKeyPressed(char targetKey);
void ClearScreen();
