#include "Game.h"

#include "Helper.h"
#include "LinuxTerminal.h"

#include <asm-generic/ioctls.h>
#include <chrono>
#include <clocale>
#include <cmath>
#include <csignal>
#include <cwchar>
#include <stdio.h>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

Game::Game(const std::string &title, int windowWidth, int windowHeight)
	: m_WinWidth(windowWidth), m_WinHeight(windowHeight), m_Title(title),
	  m_Screen((size_t)windowWidth * windowHeight)
{
	s_ScreenResized = false;
}

void Game::HandleResize(int sig)
{
	// s_ScreenResized = true;
}

void Game::SetScreenToTerminalSize(int *width, int *height)
{
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
	{
		*width = w.ws_col;
		*height = w.ws_row;
	}
}
void Game::Init()
{
	// std::ios::sync_with_stdio(false);
	// Syncs the program's runtime encoding to your Linux system's native UTF-8
	// locale
	std::setlocale(LC_ALL, "");

	// ---- REGISTER THE CRASH INTERCEPTOR ----
	struct sigaction sa;
	sa.sa_handler = terminal.CrashHandler; // Point to our function
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	// Intercept crashes
	sigaction(SIGSEGV, &sa, nullptr);
	sigaction(SIGABRT, &sa, nullptr);
	// Intercept manual close commands
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
	// ----------------------------------------

	terminal.EnableRawMode();

	// setup the map

	m_Map = L"####################"
			L"#..................#"
			L"#............2.....#"
			L"#.11111......2.....#"
			L"#............2.....#"
			L"#............2.....#"
			L"#.....3333.........#"
			L"#........#.........#"
			L"#........#.........#"
			L"#........#.........#"
			L"#..................#"
			L"#..................#"
			L"#...........1111...#"
			L"#...........2......#"
			L"#...........2......#"
			L"#......333333......#"
			L"#..................#"
			L"#..................#"
			L"#..................#"
			L"####################";

	signal(SIGWINCH, HandleResize);

	// fill buffer values for intial screen size
	SetScreenToTerminalSize(&m_WinWidth, &m_WinHeight);
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
	auto prev = std::chrono::steady_clock::now();

	while (m_Running)
	{
		auto now = std::chrono::steady_clock::now();
		float dt = std::chrono::duration<float>(now - prev).count();
		prev = now;
		if (dt <= 0.0f)
			dt = 1 / 1000.0f; // so that we dont divide by 0

		if (terminal.HasScreenResized())
		{
			SetScreenToTerminalSize(&m_WinWidth, &m_WinHeight);
			m_Screen.resize(m_WinWidth * m_WinHeight);
			terminal.ClearScreen();
			terminal.SetScreenResized(false);
		}

		ProcessInputs(m_Running, m_Player, m_ShowMiniMap, m_MinimapKeyHeld, dt,
				   m_Map, c_MapWidth, c_MapHeight);

		// clear the screen before drawing
		// like how we do glClear in opengl
		// std::fill(screen.begin(), screen.end(), " "); // no need ig, looks
		// clean to me, maybe will test properly later

		// now the main part
		// we cast our rays at every column (along the x axis)
		// and check if it hits a wall (i.e. the char '#' from our map)
		// when it does hit, get the distance from the plyaer to the wall and
		// render the specific text for the wall/sky/floor according to the
		// distance
		// seems simple!!!

		// ---- render ----
		auto tRayStart = std::chrono::steady_clock::now();

		for (int x = 0; x < m_WinWidth; ++x)
			RenderColumn(x);

		float raycastMs = std::chrono::duration<float, std::milli>(
							  std::chrono::steady_clock::now() - tRayStart)
							  .count();

		static float drawMs = 0.0f;
		char buf[161];
		snprintf(buf, sizeof(buf),
				 "FPS: %4.1f | Pos: (%4.1f, %4.1f) | Angle: %5.1f deg | "
				 "ray: %4.1fms | draw: %4.1fms",
				 1.0f / dt, m_Player.pos.x, m_Player.pos.y,
				 m_Player.currentAngle * (180.0f / (float)M_PI), raycastMs,
				 drawMs);

		const RGBA hudColor{120, 230, 160, 255};
		int statsY = m_WinHeight - 2;
		terminal.DrawText(m_Screen, m_WinWidth, m_WinHeight, 0, statsY, buf, hudColor);
		terminal.DrawText(m_Screen, m_WinWidth, m_WinHeight, 0, statsY + 1,
				 "W/S move  A/D turn  Q/E strafe  Space sprint  M map  "
				 "ESC quit",
				 hudColor);
		if (m_ShowMiniMap)
			DrawMiniMap(m_Screen, m_WinWidth, m_WinHeight, m_Map, c_MapWidth,
						c_MapHeight, m_Player.pos, m_Player.currentAngle);

		auto tDrawStart = std::chrono::steady_clock::now();
		terminal.DrawFrame(m_Screen, m_WinWidth, m_WinHeight);

		drawMs = std::chrono::duration<float, std::milli>(
					 std::chrono::steady_clock::now() - tDrawStart)
					 .count();

		// Cap the frame rate so we don't peg a CPU core spinning on a
		// render that, with the batched writer, is now cheap enough to
		// run far faster than the eye (or the terminal) needs.
		constexpr auto targetFrameTime =
			std::chrono::milliseconds(6); // ~166fps
		auto elapsedThisFrame = std::chrono::steady_clock::now() - now;
		if (elapsedThisFrame < targetFrameTime)
			std::this_thread::sleep_for(targetFrameTime - elapsedThisFrame);
	}

	terminal.LinuxTerminalClose(true);
}

struct RayHit
{
	float perpendiculardist;
	bool sideHit;
	bool isEdge;
	wchar_t wallType;
};

RayHit castRay(float rayAngle, Vec2f pos, float maxDepth, std::wstring &map, Vec2i mapSize)
{
	float rayDirX = sinf(rayAngle);
	float rayDirY = cosf(rayAngle);

	int mapX = (int)pos.x;
	int mapY = (int)pos.y;

	float deltaDistX = (rayDirX == 0.0f) ? INFINITY : fabsf(1.0f / rayDirX);
	float deltaDistY = (rayDirY == 0.0f) ? INFINITY : fabsf(1.0f / rayDirY);

	int stepX, stepY;
	float sideDistX, sideDistY;

	if (rayDirX < 0)
	{
		stepX = -1;
		sideDistX = (pos.x - mapX) * deltaDistX;
	}
	else
	{
		stepX = 1;
		sideDistX = (mapX + 1.0f - pos.x) * deltaDistX;
	}

	if (rayDirY < 0)
	{
		stepY = -1;
		sideDistY = (pos.y - mapY) * deltaDistY;
	}
	else
	{
		stepY = 1;
		sideDistY = (mapY + 1.0f - pos.y) * deltaDistY;
	}

	bool sideHit = false;
	bool outOfBounds = false;
	wchar_t wallType = L'.';

	while (wallType == L'.')
	{

		if (sideDistX < sideDistY)
		{
			sideDistX += deltaDistX;
			mapX += stepX;
			sideHit = false;
		}
		else
		{
			sideDistY += deltaDistY;
			mapY += stepY;
			sideHit = true;
		}

		if (mapX < 0 || mapX >= mapSize.x || mapY < 0 || mapY >= mapSize.y)
		{
			outOfBounds = true;
			break;
		}
		wallType = map[mapY * mapSize.x + mapX];
	}

	if (outOfBounds)
		return RayHit{maxDepth, sideHit, false, L'.'};

	float pDist = sideHit ? (sideDistY - deltaDistY) : (sideDistX - deltaDistX);
	if (pDist > maxDepth)
		pDist = maxDepth;

	// Exact fractional position where the ray crosses the wall face; near
	// 0 or 1 means we're near the seam between two adjacent tiles.
	float wallX =
		sideHit ? (pos.x + pDist * rayDirX) : (pos.y + pDist * rayDirY);
	wallX -= floorf(wallX);
	bool isEdge = (wallX < 0.03f || wallX > 0.97f);

	return RayHit{pDist, sideHit, isEdge, wallType};
}

static float RowDistance(int y, int height, float aspectRatio)
{
	float diff = fabsf((float)y - height / 2.0f);
	if (diff < 0.5f) // avoid dividing by ~0 right at the horizon line
		return 1000.0f;
	return (aspectRatio * height) / diff;
}

void Game::RenderColumn(int x)
{
	float rayAngle = (m_Player.currentAngle - m_Player.fov / 2) +
					 (x / (float)m_WinWidth) * m_Player.fov;

	RayHit rayHitPoint =
		castRay(rayAngle, m_Player.pos, m_Player.maxDepth, m_Map, Vec2i{c_MapWidth, c_MapHeight});

	int ceiling = (int)(m_WinHeight / 2.0f - (c_AspectRatio * m_WinHeight) /
												 rayHitPoint.perpendiculardist);

	int floorEdge = m_WinHeight - ceiling;

	if (ceiling < 0)
		ceiling = 0;
	if (floorEdge >= m_WinHeight)
		floorEdge = m_WinHeight - 1;

	const RGBA fogColor{10, 10, 22, 255};

	float fog = Clamp(1.0f - rayHitPoint.perpendiculardist / m_Player.maxDepth,
					  0.12f, 1.0f);
	RGBA wallColor = LerpRGB(
		fogColor, WallColor(rayHitPoint.wallType, rayHitPoint.sideHit), fog);
	if (rayHitPoint.isEdge)
		wallColor = LerpRGB(RGBA{0, 0, 0, 255}, wallColor, 0.45f);

	const RGBA skyNear{40, 50, 90, 255};
	const RGBA skyFar{8, 8, 20, 255};
	const RGBA floorNear{90, 80, 60, 255};
	const RGBA floorFar{15, 14, 12, 255};

	for (int y = 0; y < m_WinHeight; ++y)
	{
		Cell cell;
		if (y < ceiling)
		{
			float dist = RowDistance(y, m_WinHeight, c_AspectRatio);
			float f = Clamp(1.0f - dist / m_Player.maxDepth, 0.05f, 1.0f);
			cell.color = LerpRGB(skyFar, skyNear, f);
		}
		else if (y <= floorEdge)
		{
			cell.color = wallColor;
		}
		else
		{
			float dist = RowDistance(y, m_WinHeight, c_AspectRatio);
			float f = Clamp(1.0f - dist / m_Player.maxDepth, 0.05f, 1.0f);
			RGBA c = LerpRGB(floorFar, floorNear, f);
			// Cheap checkerboard: darken alternating distance bands so the
			// floor reads as a receding grid instead of a flat gradient.
			if (((int)(dist * 2.0f)) % 2 == 0)
				c = LerpRGB(RGBA{0, 0, 0, 255}, c, 0.85f);
			cell.color = c;
		}
		m_Screen[y * m_WinWidth + x] = cell;
	}
}
