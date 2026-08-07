#pragma once

#include <string>
#include <vector>

#include "Helper.h"
#include "LinuxTerminal.h"

class Game
{
  public:
	Game(const std::string &title, int windowWidth, int windowHeight);

	void Init();
	void Run();
	int GetWinWidth() const;
	int GetWinHeight() const;
	void SetScreenToTerminalSize(int *width, int *height);
	void RenderColumn(int index);

	static void HandleResize(int sig);

  private:
	const std::string &m_Title;
	int m_WinWidth;
	int m_WinHeight;
	 bool s_ScreenResized;
	const float c_AspectRatio = 2.0f;

	LinuxTerminal terminal;
	bool m_Running = true;
	std::wstring m_Map;
	const int c_MapWidth = 20;
	const int c_MapHeight = 20;
	std::vector<Cell> m_Screen;

	Player m_Player;

	bool m_ShowMiniMap = true;
	bool m_MinimapKeyHeld = false;
};
