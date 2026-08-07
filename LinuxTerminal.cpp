#include "LinuxTerminal.h"
#include <charconv>
#include <csignal>
#include <cstring>
#include <fcntl.h>

// write() is allowed to send fewer bytes than asked (this is explicit in
// POSIX once you go past PIPE_BUF, and pty writes are not exempt) -- the
// previous version ignored the return value entirely, so any short write
// silently dropped the rest of the frame. This loops until every byte is
// actually sent, retrying on EINTR/EAGAIN, so a frame is either written
// completely or (on a genuine unrecoverable error) not written at all --
// never half-written.
static void WriteAll(int fd, const char *data, size_t len)
{
	size_t written = 0;
	while (written < len)
	{
		ssize_t n = write(fd, data + written, len - written);
		if (n > 0)
		{
			written += (size_t)n;
			continue;
		}
		if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
			continue; // interrupted or momentarily full -- try again
		break;		  // unrecoverable error; give up rather than spin forever
	}
}
void LinuxTerminal::EnableRawMode()
{
	tcgetattr(STDIN_FILENO, &s_OrigTermios);
	struct termios raw = s_OrigTermios;
	raw.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

	// Set stdin to NON-BLOCKING mode once here
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	const char *hide_cursor = "\x1b[?25l";
	write(STDOUT_FILENO, hide_cursor, 6);
}


void LinuxTerminal::DisableRawMode()
{
	// Reverse of EnableRawMode's setup: show cursor, leave alt screen.
	const char *teardown = "\x1b[?25h\x1b[?1049l";
	WriteAll(STDOUT_FILENO, teardown, strlen(teardown));
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &s_OrigTermios);
}


// Appends the decimal digits of a value with no heap allocation. Used for
// both 0-255 color channels and row/column numbers (which can exceed 255
// on a large terminal), hence taking an int rather than uint8_t.
static inline void AppendUInt(std::string &out, int v)
{
	char buf[12];
	auto res = std::to_chars(buf, buf + sizeof(buf), v);
	out.append(buf, res.ptr - buf);
}

static inline bool CellsEqual(const Cell &a, const Cell &b)
{
	return a.character == b.character && a.color.r == b.color.r && a.color.g == b.color.g &&
		   a.color.b == b.color.b;
}

// Appends one cell's glyph to `out`, emitting a new truecolor escape only
// when the color actually changed since the last cell written *anywhere
// in this frame* (color state is a terminal-global SGR, so this is valid
// across a row boundary too, not just within one row).
static inline void AppendCell(std::string &out, const Cell &cell, int &lastR,
							   int &lastG, int &lastB)
{
	if (cell.color.r != lastR || cell.color.g != lastG || cell.color.b != lastB)
	{
		out += "\x1b[38;2;";
		AppendUInt(out, cell.color.r);
		out += ';';
		AppendUInt(out, cell.color.g);
		out += ';';
		AppendUInt(out, cell.color.b);
		out += 'm';
		lastR = cell.color.r;
		lastG = cell.color.g;
		lastB = cell.color.b;
	}

	if (cell.character == 0)
		out += "\xE2\x96\x88"; // U+2588 FULL BLOCK, UTF-8
	else
		out += cell.character;
}

void LinuxTerminal::DrawFrame(const std::vector<Cell> &screen, int width,
							  int height)
{
	// Previous frame's cells, kept around purely so we can skip rewriting
	// rows that haven't changed. Reallocated (and a full redraw forced)
	// whenever the terminal size changes.

	static std::vector<Cell> prevScreen;
	static int prevWidth = -1, prevHeight = -1;

	bool forceFull = (width != prevWidth || height != prevHeight);
	if (forceFull)
	{
		prevScreen.assign((size_t)width * height, Cell{});
		prevWidth = width;
		prevHeight = height;
	}

	static std::string out;
	out.clear();
	out.reserve((size_t)width * height * 4);

	out += "\x1b[?2026h"; // begin synchronized update (no-op if unsupported)

	if (forceFull)
		out += "\x1b[2J"; // clear stray glyphs a shrinking resize might leave

	// -1 can never match a real 0-255 channel, so the first cell written
	// this frame always emits its color code.
	int lastR = -1, lastG = -1, lastB = -1;

	for (int y = 0; y < height; ++y)
	{
		const Cell *rowStart = &screen[(size_t)y * width];
		const Cell *prevRowStart = &prevScreen[(size_t)y * width];

		bool rowChanged = forceFull;
		if (!rowChanged)
		{
			for (int x = 0; x < width; ++x)
			{
				if (!CellsEqual(rowStart[x], prevRowStart[x]))
				{
					rowChanged = true;
					break;
				}
			}
		}
		if (!rowChanged)
			continue;

		// Explicit absolute positioning for every row we rewrite -- never
		// rely on the terminal's auto-wrap to land us at the right
		// column, which is what let one dropped/miscounted byte smear
		// every row after it.
		out += "\x1b[";
		AppendUInt(out, y + 1);
		out += ";1H";

		for (int x = 0; x < width; ++x)
			AppendCell(out, rowStart[x], lastR, lastG, lastB);

		out += "\x1b[K"; // erase to end of line: guards against leftover
						 // glyphs if this row was ever wider before

		for (int x = 0; x < width; ++x)
			prevScreen[(size_t)y * width + x] = rowStart[x];
	}

	out += "\x1b[0m";	  // leave the terminal in a clean SGR state
	out += "\x1b[?2026l"; // end synchronized update

	WriteAll(STDOUT_FILENO, out.data(), out.size());
}

void LinuxTerminal::DrawText(std::vector<Cell> &screen, int screenWidth, int screenHeight,
			  int startX, int startY, const std::string &text, RGBA color)
{
	if (startY < 0 || startY >= screenHeight)
		return;

	for (size_t i = 0; i < text.length(); ++i)
	{
		int targetX = startX + (int)i;
		if (targetX >= 0 && targetX < screenWidth)
			screen[startY * screenWidth + targetX] = Cell{text[i], color};
	}
}


LinuxTerminal::LinuxTerminal()
{
	s_OrigTermios = termios_t{};

}
void LinuxTerminal::LinuxTerminalClose(bool rawModeEnabled)
{
	if (rawModeEnabled)
		DisableRawMode();
}
termios_t &LinuxTerminal::GetTermiosRef()
{
	return s_OrigTermios;
}

static void WriteIgnoreResult(int fd, const char *data, size_t len)
{
	ssize_t ignored = write(fd, data, len);
	(void)ignored;
}

void LinuxTerminal::CrashHandler(int signum)
{
	LinuxTerminalClose(true);

	if (signum == SIGSEGV)
	{
		const char *msg = "\n[CRASH] Segmentation fault detected!\n";
		WriteIgnoreResult(STDERR_FILENO, msg, strlen(msg));
	}

	_Exit(signum);
}
void LinuxTerminal::ClearScreen()
{
	const char *clear = "\x1b[2J";
	// write(STDOUT_FILENO, clear, 4);
	std::cout << clear;
}

LinuxTerminal::~LinuxTerminal()
{
	if (m_RawModeEnabled)
	{
		// Safe alternative to std::cout << "\x1b[?25h" << std::flush, says
		// gemini;
		const char *show_cursor = "\x1b[?25h";
		write(STDOUT_FILENO, show_cursor, 6);

		// Restore the original terminal flags
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &s_OrigTermios);
	}
}
