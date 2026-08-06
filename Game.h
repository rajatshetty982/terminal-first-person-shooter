#pragma once

#include <string>

class Game
{
  public:
	void InitWindow(const std::string &title = "A terminal game",
					int windowWidth = 240, int windowHeight = 60);
	void Run();
	int GetWinWidth() const;
	int GetWinHeight() const;

  private:
	int m_WinWidth;
	int m_WinHeight;
	bool m_Running = true;
};
