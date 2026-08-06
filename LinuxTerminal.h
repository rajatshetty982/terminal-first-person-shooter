#pragma once

#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

typedef struct termios termios_t;

void EnableRawMode(termios &orig_termios);

void DisableRawMode(termios &orig_termios);

void DrawFrame(const std::vector<std::string> &screen, int width, int height);


void linuxTerminalClose(bool rawModeEnabled, termios &orig_termios);
