#include "Helper.h"

#include <cctype>
#include <cmath>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>


// default screen dimensions, must be adjust accoring to the terminal size

// not used
bool IsKeyPressed(char targetKey)
{
	char ch;
	bool pressed = false;

	// Read all buffered input bytes without blocking
	while (read(STDIN_FILENO, &ch, 1) > 0)
	{
		if (ch == targetKey || ch == ::tolower(targetKey) ||
			ch == ::toupper(targetKey))
		{
			pressed = true;
		}
	}

	return pressed;
}

void MovePlayer(bool moveAhead, Vec2f &pos, float currentAngle, float speed,
				float elaspedT)
{
	float dir = moveAhead ? 1.0f : -1.0f;
	pos.x += sinf(currentAngle) * speed * elaspedT * dir;
	pos.y += cosf(currentAngle) * speed * elaspedT * dir;
}
float Clamp(float v, float lo, float hi)
{
	if (v < lo)
		return lo;
	if (v > hi)
		return hi;
	return v;
}

RGBA LerpRGB(const RGBA &a, const RGBA &b, float t)
{
	t = Clamp(t, 0.1f, 1.0f);

	return RGBA{
		(uint8_t)(a.r + (b.r - a.r) * t), (uint8_t)(a.g + (b.g - a.g) * t),
		(uint8_t)(a.b + (b.b - a.b) * t), (uint8_t)(a.a + (b.a - a.a) * t)};
}

RGBA WallColor(wchar_t wallType, bool darkSide)
{
	RGBA base;

	switch (wallType)
	{
	case L'1':
		base = {115, 12, 12};
		break; // Congealed / Dried Blood Red
	case L'2':
		base = {35, 18, 55};
		break; // Bruised Shadow / Void Purple
	case L'3':
		base = {45, 60, 15};
		break; // Toxic Waste Sludge Green
	default:
		base = {55, 48, 45};
		break; // Scorched Iron / Dried Rust	}

		if (darkSide)
		{
			base.r = (uint8_t)(base.r * 0.65f); // FIXME: why is this done here
			base.g = (uint8_t)(base.g * 0.65f);
			base.b = (uint8_t)(base.b * 0.65f);
		}
		return base;
	}
	return base;
}

bool IsWallTile(wchar_t tile)
{
	// floor is of '.' everything else is a wall
	return !L'.';
}

static inline bool collided(Vec2f pos, std::wstring &map, int mapWidth,
							int mapHeight)
{
	// check is the buddy is trying to escape the matrix
	int x = (int)pos.x;
	int y = (int)pos.y;
	if (x < 0 || x > mapWidth || y < 0 || y > mapHeight)
		return true;
	return IsWallTile(map[y * mapWidth + x]);
}

inline void TryMovePlayer(Vec2f &pos, float dx, float dy, std::wstring &map,
						  int mapWidth, int mapHeight)
{
	Vec2f afterX = Vec2f{pos.x + dx, pos.y};
	if (!collided(afterX, map, mapWidth, mapHeight))
	{
		pos.x = afterX.x;
	}

	Vec2f afterY = Vec2f{pos.x, pos.y + dy};
	if (!collided(afterY, map, mapWidth, mapHeight))
	{
		pos.y = afterY.y;
	}
}

void DoMovement(bool &running, Player &player, bool &miniMapToggle,
				bool &minimapKeyHeld, float dt, std::wstring &map,
				const int mapwidth, const int mapheight)
{
	// ---- input: drain every buffered byte this frame (non-blocking) ----
	bool aPressed = false, dPressed = false, wPressed = false, sPressed = false,
		 qPressed = false, ePressed = false, spacePressed = false,
		 mPressed = false, escPressed = false;
	char ch;
	while (read(STDIN_FILENO, &ch, 1) > 0)
	{
		switch (ch)
		{
		case 'a':
		case 'A':
			aPressed = true;
			break;
		case 'd':
		case 'D':
			dPressed = true;
			break;
		case 'w':
		case 'W':
			wPressed = true;
			break;
		case 's':
		case 'S':
			sPressed = true;
			break;
		case 'q':
		case 'Q':
			qPressed = true;
			break; // strafe left
		case 'e':
		case 'E':
			ePressed = true;
			break; // strafe right
		case ' ':
			spacePressed = true;
			break; // sprint
		case 'm':
		case 'M':
			mPressed = true;
			break; // toggle minimap
		case 27:
			escPressed = true;
			break; // ESC -> quit
		}
	}
	if (escPressed)
		running = false;

	if (mPressed && !minimapKeyHeld)
		miniMapToggle = !miniMapToggle;
	miniMapToggle = mPressed;

	if (aPressed)
		player.currentAngle -= player.turnSpeed * dt;
	if (dPressed)
		player.currentAngle += player.turnSpeed * dt;

	// Build a combined movement vector from all held movement keys so
	// e.g. forward+strafe moves diagonally at normal speed rather than
	// the old model of testing/reverting one intent at a time.
	float speed =
		player.moveSpeed * (spacePressed ? player.sprintMultiplier : 1.0f);
	float moveX = 0.0f, moveY = 0.0f;
	if (wPressed)
	{
		moveX += sinf(player.currentAngle);
		moveY += cosf(player.currentAngle);
	}
	if (sPressed)
	{
		moveX -= sinf(player.currentAngle);
		moveY -= cosf(player.currentAngle);
	}
	if (ePressed) // strafe right: forward direction rotated +90 deg
	{
		moveX += sinf(player.currentAngle + (float)M_PI_2);
		moveY += cosf(player.currentAngle + (float)M_PI_2);
	}
	if (qPressed) // strafe left
	{
		moveX += sinf(player.currentAngle - (float)M_PI_2);
		moveY += cosf(player.currentAngle - (float)M_PI_2);
	}
	if (moveX != 0.0f || moveY != 0.0f)
	{
		// Normalize so diagonal movement isn't faster than axis-aligned.
		float len = sqrtf(moveX * moveX + moveY * moveY);
		moveX = moveX / len * speed * dt;
		moveY = moveY / len * speed * dt;
		TryMovePlayer(player.pos, moveX, moveY, map, mapwidth, mapheight);
	}
}

void DrawMiniMap(std::vector<Cell> &screen, int screenWidth, int screenHeight,
				 const std::wstring &map, int mapWidth, int mapHeight,
				 const Vec2f &playerPos, float playerAngle)
{
	const RGBA wallCol{200, 200, 60, 255};
	const RGBA floorCol{60, 60, 60, 255};
	const RGBA playerCol{255, 60, 60, 255};

	for (int y = 0; y < mapHeight; ++y)
	{
		if (y >= screenHeight)
			break;
		for (int x = 0; x < mapWidth; ++x)
		{
			if (x >= screenWidth)
				break;

			wchar_t tile = map[y * mapWidth + x];
			int i = y * screenWidth + x;
			screen[i] =
				IsWallTile(tile) ? Cell{'#', wallCol} : Cell{'.', floorCol};
		}
	}

	int playerX = (int)playerPos.x;
	int playerY = (int)playerPos.y;
	if (playerX < 0 || playerX >= mapWidth || playerY < 0 ||
		playerY >= mapHeight || playerY >= screenHeight ||
		playerX >= screenWidth)
		return;

	// Angle 0 points along +Y (north/"down" in array terms), matching how
	// the raycaster below defines forward. Pick one of 4 glyphs by octant.
	float angle = fmodf(playerAngle, 2.0f * (float)M_PI);
	if (angle < 0)
		angle += 2.0f * (float)M_PI;

	char arrow;
	if (angle >= 0.25f * M_PI && angle < 0.75f * M_PI)
		arrow = 'v';
	else if (angle >= 0.75f * M_PI && angle < 1.25f * M_PI)
		arrow = '<';
	else if (angle >= 1.25f * M_PI && angle < 1.75f * M_PI)
		arrow = '^';
	else
		arrow = '>';

	screen[playerY * screenWidth + playerX] = Cell{arrow, playerCol};
}
