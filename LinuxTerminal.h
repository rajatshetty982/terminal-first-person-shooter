#pragma once

#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include "Helper.h"

typedef struct termios termios_t;

class LinuxTerminal
{
  public:
	LinuxTerminal();
	~LinuxTerminal();
	void EnableRawMode();
	void DisableRawMode();
	void DrawFrame(const std::vector<Cell> &screen, int width, int height);
	void DrawText(std::vector<Cell> &screen, int screenWidth, int screenHeight,
				  int startX, int startY, const std::string &text, RGBA color);
	static void CrashHandler(int signum);
	static void LinuxTerminalClose(bool rawModeEnabled);
	void ClearScreen();
	termios_t &GetTermiosRef();
	bool HasScreenResized() const
	{
		return v_ScreenResized;
	}
	void SetScreenResized(bool has)
	{
		v_ScreenResized = has;
	}

  private:
	inline static termios_t s_OrigTermios;
	bool m_RawModeEnabled = true;
	volatile bool v_ScreenResized = false;
};
