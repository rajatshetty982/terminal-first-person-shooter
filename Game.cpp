#include "Game.h"

void Game::InitWindow(const std::string &title, int windowWidth,
					  int windowHeight)
{
	m_WinWidth = windowWidth;
	m_WinHeight = windowHeight;
}

int Game::GetWinWidth() const
{
	return m_WinWidth;
}
int Game::GetWinHeight() const
{
	return m_WinHeight;
}
void Game::Run()
{
	while (m_Running)
	{
		// run bitchh runn
	}
}
