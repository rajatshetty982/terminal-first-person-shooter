#include "LinuxTerminal.h"
#include <asm-generic/ioctls.h>
#include <cstdio>
#include <fcntl.h>
#include <iostream>

void EnableRawMode(termios &orig_termios)
{
	tcgetattr(STDIN_FILENO, &orig_termios);
	struct termios raw = orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

	// Set stdin to NON-BLOCKING mode once here
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	const char *hide_cursor = "\x1b[?25l";
	write(STDOUT_FILENO, hide_cursor, 6);
}

void DisableRawMode(termios &orig_termios)
{
	std::cout << "\x1b[?25h"; // Show cursor
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
void DrawFrame(const std::vector<std::string> &screen, int width, int height)
{
	// 	static std::string frame;
	// 	frame.clear();
	// 	frame.reserve(width * height * 3 + 32);
	//
	// 	frame.append("\x1b[?2026h"); // begin synchronized update
	// 	frame.append("\x1b[H");
	// 	for (const auto &cell : screen) frame += cell;
	// 	frame.append("\x1b[?2026l"); // end synchronized update
	//
	// 	fwrite(frame.data(), 1, frame.size(), stdout);
	// 	fflush(stdout);

	// const char *cursor_at_start = "\x1b[H";
	std::cout << "\x1b[H";

	for (const auto &cell : screen)
	{
		std::cout << cell;
	}

	std::cout.flush();
}

void linuxTerminalClose(bool rawModeEnabled, termios &orig_termios)
{
	if (rawModeEnabled)
	{
		// Safe alternative to std::cout << "\x1b[?25h" << std::flush, says
		// gemini;
		const char *show_cursor = "\x1b[?25h";
		write(STDOUT_FILENO, show_cursor, 6);

		// Restore the original terminal flags
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
	}
}
