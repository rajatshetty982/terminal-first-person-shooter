#include "Helper.h"
#include "LinuxTerminal.h"
#include <cctype>
#include <fcntl.h>
#include <unistd.h>

// default screen dimensions, must be adjust accoring to the terminal size
int g_ScreenWidth = 120;
int g_ScreenHeight = 40;
volatile bool g_ScreenResized = false;

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

bool IsKeyPressed(char targetKey)
{
	char ch;
	bool pressed = false;
	while (read(STDIN_FILENO, &ch, 1) > 0)
	{
		if (ch == targetKey || ch == ::tolower(targetKey))
		{
			pressed = true;
		}
	}

	return pressed;
}

void ClearScreen()
{
	std::cout << "\x1b[2J";
}
