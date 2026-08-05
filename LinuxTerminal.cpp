#include "LinuxTerminal.h"

void EnableRawMode(termios &orig_termios)
{
	tcgetattr(STDIN_FILENO, &orig_termios);
	struct termios raw = orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	std::cout << "\x1b[?25l"; // Hide cursor
}

void DisableRawMode(termios &orig_termios)
{
	std::cout << "\x1b[?25h"; // Show cursor
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void DrawFrame(const std::vector<char> &screen, int width, int height)
{
	// Reposition cursor to top-left (0,0) without clear screen
	std::cout << "\x1b[H";

	std::cout.write(screen.data(), screen.size());
	std::cout.flush();
}

void linuxTerminalClose(bool rawModeEnabled, termios &orig_termios)
{
    if (rawModeEnabled) {
        // Safe alternative to std::cout << "\x1b[?25h" << std::flush, says gemini;
        const char* show_cursor = "\x1b[?25h";
        write(STDOUT_FILENO, show_cursor, 6); 
        
        // Restore the original terminal flags
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
}
