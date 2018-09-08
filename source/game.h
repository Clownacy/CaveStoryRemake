#pragma once
#include "common.h"

enum gameStates
{
	INTRO = 1,
	MENU = 2,
	PLAY = 3,
};

enum debugFlagsEnum
{
	showSlots = 0x01,
	showNPCId = 0x02,
	showBULId = 0x04,
	showCARId = 0x08,
	notifyOnNotImplemented = 0x10,
	showNPCHealth = 0x20,
};

struct PERMIT_STAGE
{
	int index;
	int event;
};

struct ITEM
{
	int code;
};

struct VIEW
{
	int x;
	int y;
	int *lookX;
	int *lookY;
	int speed;

	int quake;
	int quake2;
};

struct BOSSLIFE
{
	int flag;
	int *pLife;
	int max;
	int br;
	int count;
};

//Game variables
constexpr auto ITEMS = 32;
constexpr auto PERMITSTAGES = 8;

extern PERMIT_STAGE permitStage[PERMITSTAGES];
extern ITEM items[ITEMS];
extern int selectedItem;

extern BOSSLIFE bossLife;

extern VIEW viewport;

//Debug flags
extern int debugFlags;

//Functions
void viewBounds();
void initGame();
void init2();

int escapeMenu();

int openMapSystem();
int openInventory();
int stageSelect(int *runEvent);
int mainGameLoop();
