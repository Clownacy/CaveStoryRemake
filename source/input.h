#pragma once

#include "common.h"

extern bool useGamepad;

extern int keyLeft;
extern int keyRight;
extern int keyUp;
extern int keyDown;
extern int keyJump;
extern int keyShoot;
extern int keyMenu;
extern int keyMap;
extern int keyRotLeft;
extern int keyRotRight;

void initGamepad();

void getKeys();

attrPure bool isKeyDown(int keynum);
attrPure bool isKeyPressed(int keynum);
bool handleEvents();
