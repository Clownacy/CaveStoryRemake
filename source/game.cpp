#include "game.h"
#include "weapons.h"
#include "level.h"
#include "hud.h"
#include "script.h"
#include "fade.h"
#include "input.h"
#include "filesystem.h"
#include "caret.h"
#include "valueview.h"
#include "render.h"
#include "sound.h"
#include "player.h"
#include "bullet.h"
#include "mathUtils.h"
#include "flags.h"

#include <cstring>
#include <SDL_scancode.h>
#include <SDL_timer.h>
#include <SDL_render.h>
#include <SDL_events.h>

using std::memset;

int gameMode = 1;

VIEW viewport;

//Init game function
void initGame()
{
	//Clear flags
	memset(tscFlags, 0, sizeof(tscFlags));
	memset(mapFlags, 0, sizeof(mapFlags));

	//Clear other stuff
	initWeapons();
	memset(permitStage, 0, sizeof(permitStage));

	//Set up fade
	initFade();
	fade.bMask = true;

	//Load stage
	currentPlayer.init();
	currentPlayer.setPos(10 << 13, 8 << 13);
	loadLevel(13);
	startTscEvent(200);
}

void viewBounds()
{
	//Keep view in level
	if ((levelWidth - 1) << 4 > screenWidth)
		viewport.x = clamp(viewport.x, 0, ((levelWidth - 1) << 13) - (screenWidth << 9));
	else
		viewport.x = ((levelWidth - 1) << 12) - (screenWidth << 8);

	if ((levelHeight - 1) << 4 > screenHeight)
		viewport.y = clamp(viewport.y, 0, ((levelHeight - 1) << 13) - (screenHeight << 9));
	else
		viewport.y = ((levelHeight - 1) << 12) - (screenHeight << 8);
}

void handleView()
{
	//Move view
	viewport.x += ((*viewport.lookX - (screenWidth << 8)) - viewport.x) / viewport.speed;
	viewport.y += ((*viewport.lookY - (screenHeight << 8)) - viewport.y) / viewport.speed;

	viewBounds();

	//Quake
	if (viewport.quake2)
	{
		viewport.x += (random(-5, 5) << 9);
		viewport.y += (random(-3, 3) << 9);
		--viewport.quake2;
	}
	else if (viewport.quake)
	{
		viewport.x += (random(-1, 1) << 9);
		viewport.y += (random(-1, 1) << 9);
		--viewport.quake;
	}
}

void debugLevels()
{
	if (isKeyPressed(SDL_SCANCODE_1))
	{
		loadLevel(13);
		currentPlayer.setPos(10 << 13, 8 << 13);

		//sound::playOrg(8);
	}
	else if (isKeyPressed(SDL_SCANCODE_2))
	{
		loadLevel(12);
		currentPlayer.setPos(37 << 13, 11 << 13);

		//sound::playOrg(8);
	}
	else if (isKeyPressed(SDL_SCANCODE_3))
	{
		loadLevel(2);
		currentPlayer.setPos(5 << 13, 6 << 13);

		//sound::playOrg(1);
	}
	else if (isKeyPressed(SDL_SCANCODE_4))
	{
		loadLevel(11);
		currentPlayer.setPos(13 << 13, 34 << 13);

		//sound::playOrg(9);
	}
	else if (isKeyPressed(SDL_SCANCODE_5))
	{
		loadLevel(49);
		currentPlayer.setPos(7 << 13, 6 << 13);

		//sound::playOrg(27);
	}
	else if (isKeyPressed(SDL_SCANCODE_6))
	{
		loadLevel(67);
		currentPlayer.setPos(7 << 13, 52 << 13);

		//sound::playOrg(29);
	}
	else if (isKeyPressed(SDL_SCANCODE_7))
	{
		loadLevel(48);
		currentPlayer.setPos(155 << 13, 1 << 13);

		//sound::playOrg(38);
	}
	else if (isKeyPressed(SDL_SCANCODE_8))
	{
		loadLevel(53);
		currentPlayer.setPos(4 << 13, 165 << 13);

		//sound::playOrg(38);
	}
	else if (isKeyPressed(SDL_SCANCODE_9))
	{
		loadLevel(6);
		currentPlayer.setPos(4 << 13, 18 << 13);

		//sound::playOrg(38);
	}
}

void delimitFramerate()
{
	while (true)
	{
		//Framerate limiter
		const Uint32 timeNow = SDL_GetTicks();
		const Uint32 timeNext = framerateTicks + framerate;

		if (timeNow >= timeNext)
			framerateTicks = SDL_GetTicks();
		else
		{
			SDL_Delay(timeNext - timeNow);
			continue;
		}
		return;
	}
}

//Escape menu
RECT rcEscape = { 0, 128, 208, 144 };
int escapeMenu()
{
	while (true)
	{
		delimitFramerate();

		//Handle events
		getKeys(&events);
		if (events.type == SDL_QUIT || exitGame)
			return 0;

		if (isKeyPressed(SDL_SCANCODE_ESCAPE))
		{
			return 0;
		}
		if (isKeyPressed(SDL_SCANCODE_F1))
		{
			return 1;
		}
		if (isKeyPressed(SDL_SCANCODE_F2))
		{
			return 2;
		}

		if ((isKeyDown(SDL_SCANCODE_LALT) && isKeyPressed(SDL_SCANCODE_RETURN)) ||
		        (isKeyPressed(SDL_SCANCODE_LALT) && isKeyDown(SDL_SCANCODE_RETURN)))
		{
			switchScreenMode();
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		drawTexture(sprites[0x1A], &rcEscape,
		            (screenWidth >> 1) - 104,
		            (screenHeight >> 1) - 8);

		SDL_RenderPresent(renderer);
	}

	return 0;
}

//Teleporter Menu
PERMIT_STAGE permitStage[8];
int selectedStage;
int stageSelectTitleY;
int stageSelectFlash;

void moveStageSelectCursor()
{
	int stageNo; // [esp+18h] [ebp-10h]

	for (stageNo = 0; permitStage[stageNo].index != 0; ++stageNo);

	if (stageNo)
	{
		//Left and right move
		if (isKeyPressed(keyLeft))
			--selectedStage;
		if (isKeyPressed(keyRight))
			++selectedStage;

		//Wrap around
		if (selectedStage < 0)
			selectedStage = stageNo - 1;
		if (selectedStage >= stageNo)
			selectedStage = 0;

		//Run event and play sound
		if (isKeyPressed(keyLeft) || isKeyPressed(keyRight))
			startTscEvent(permitStage[selectedStage].index + 1000);
		if (isKeyPressed(keyLeft) || isKeyPressed(keyRight))
			playSound(SFX_YNChangeChoice);
	}
}

void drawStageSelect()
{
	RECT rcCur[2];
	RECT rcTitle1;
	RECT rcStage;

	int stageNo;

	rcCur[0] = { 80, 88, 112, 104 };
	rcCur[1] = { 80, 104, 112, 120 };
	rcTitle1 = { 80, 64, 144, 72 };

	if (stageSelectTitleY > 46)
		--stageSelectTitleY;

	drawTexture(sprites[TEX_TEXTBOX], &rcTitle1, (screenWidth / 2) - 32, stageSelectTitleY);
	for (stageNo = 0; permitStage[stageNo].index != 0; stageNo++);

	++stageSelectFlash;

	if (stageNo)
	{
		const int stageX = (-40 * stageNo + screenWidth) / 2;

		//Draw everything now
		drawTexture(sprites[TEX_TEXTBOX], &rcCur[(stageSelectFlash >> 1) & 1], stageX + 40 * selectedStage, 64);
		for (int i = 0; i < 7 && permitStage[i].index; ++i)
		{
			rcStage.left = 32 * (permitStage[i].index % 8);
			rcStage.right = rcStage.left + 32;
			rcStage.top = 16 * (permitStage[i].index / 8);
			rcStage.bottom = rcStage.top + 16;

			drawTexture(sprites[0xE], &rcStage, stageX + 40 * i, 64);
		}
	}
}

int stageSelect(int *runEvent)
{
	//Keep track of old one
	char oldScript[260];
	strcpy(oldScript, tsc.path);

	//Init some stuff
	selectedStage = 0;
	stageSelectTitleY = 54;

	//Load stage select tsc
	loadTsc2("data/StageSelect.tsc");
	startTscEvent(permitStage[selectedStage].index + 1000);

	while (true)
	{
		delimitFramerate();

		//Handle events
		getKeys(&events);
		if (events.type == SDL_QUIT || exitGame)
			return 0;

		if (isKeyDown(SDL_SCANCODE_ESCAPE))
		{
			const int escape = escapeMenu();

			if (!escape)
				return 0;
			if (escape == 2)
				return 1;
		}

		//Update menu
		moveStageSelectCursor();

		const int tscResult = updateTsc();
		if (!tscResult)
			return 0;
		if (tscResult == 2)
			return 2;

		//Draw background
		SDL_SetRenderDrawColor(renderer, 0, 0, 32, 255);
		SDL_RenderClear(renderer);

		drawLevel(false);
		drawNPC();
		currentPlayer.draw();
		drawLevel(true);
		drawCarets();
		drawValueView();

		if (gameFlags & 2)
			drawHud(false);

		//Draw menu
		drawStageSelect();
		drawTsc();

		//Select
		if (isKeyPressed(keyJump))
		{
			stopTsc();
			loadStageTsc(oldScript);
			*runEvent = permitStage[selectedStage].event;
			return 1;
		}

		//Cancel
		if (isKeyPressed(keyShoot))
		{
			stopTsc();
			loadStageTsc(oldScript);
			*runEvent = 0;
			return 1;
		}

		//Present
		SDL_RenderPresent(renderer);
	}
}

//Main States
int gameUpdatePlay()
{
	int tscResult = 0;

	while (true)
	{
		//Framerate limiter
		const uint32_t timeNow = SDL_GetTicks();
		const uint32_t timeNext = framerateTicks + framerate;

		if (timeNow >= timeNext)
		{
			framerateTicks = SDL_GetTicks();
		}
		else
		{
			SDL_Delay(timeNext - timeNow);
			continue;
		}

		//Handle events
		getKeys(&events);

		if (events.type == SDL_QUIT || exitGame)
			return 0;

		if (isKeyDown(SDL_SCANCODE_ESCAPE))
		{
			const int escape = escapeMenu();

			if (!escape)
				return 0;
			if (escape == 2)
				return 1;
		}

		if ((isKeyDown(SDL_SCANCODE_LALT) && isKeyPressed(SDL_SCANCODE_RETURN)) || (isKeyPressed(SDL_SCANCODE_LALT) && isKeyDown(SDL_SCANCODE_RETURN)))
			switchScreenMode();

		debugLevels();

		//Update stuff
		if (gameFlags & 1)
		{
			if (gameFlags & 2)
				currentPlayer.update(true);
			else
				currentPlayer.update(false);
			updateNPC();
			playerHitMap();
			playerHitNpcs();
			if (gameFlags & 2)
				actWeapon();
			updateBullets();
			updateCarets();
			updateValueView();
			if (gameFlags & 2)
				currentPlayer.animate(true);
			else
				currentPlayer.animate(false);

			handleView();
		}

		updateFade();

		// -- DRAW -- //
		SDL_SetRenderDrawColor(renderer, 0, 0, 32, 255);
		SDL_RenderClear(renderer);

		drawLevel(false);
		drawNPC();
		drawBullets();
		currentPlayer.draw();
		drawLevel(true);
		drawCarets();
		drawValueView();
		drawFade();

		//Rotate weapons
		if (gameFlags & 2)
		{
			if (isKeyPressed(keyRotRight))
				rotateWeaponRight();
			else if (isKeyPressed(keyRotLeft))
				rotateWeaponLeft();
		}

		//Do TSC stuff
		tscResult = updateTsc();

		if (!tscResult)
			return 0;
		if (tscResult == 2)
			return 1;

		drawMapName(false);

		if (gameFlags & 2)
			drawHud(false);

		drawTsc();

		SDL_RenderPresent(renderer);
	}

	return 0;
}

int gameUpdateMenu()
{
	int select = 0;
	int anime = 0;

	const int version[4] = { 1, 0, 0, 6 };

	RECT rcVersion = { 152, 80, 208, 88 };
	RECT rcPeriod = { 152, 88, 208, 96 };

	RECT rcTitle = { 0, 0, 144, 32 };
	RECT rcNew = { 144, 0, 176, 16 };
	RECT rcLoad = { 144, 16, 176, 32 };
	RECT rcPixel = { 0, 0, 160, 16 };

	uint32_t frame = 0;
	const uint8_t frameOrder[] = { 0, 1, 0, 2 };

	if (fileExists("Profile.dat"))
		select = 1;
	else
		select = 0;

	while (true)
	{
		//Framerate limiter
		const Uint32 timeNow = SDL_GetTicks();
		const Uint32 timeNext = framerateTicks + framerate;

		if (timeNow >= timeNext)
		{
			framerateTicks = SDL_GetTicks();
		}
		else
		{
			SDL_Delay(timeNext - timeNow);
			continue;
		}

		//Handle events
		getKeys(&events);

		if (events.type == SDL_QUIT || exitGame)
			return 0;

		if (isKeyDown(SDL_SCANCODE_ESCAPE))
		{
			const int escape = escapeMenu();

			if (!escape)
				return 0;
			if (escape == 2)
				return 1;
		}

		if ((isKeyDown(SDL_SCANCODE_LALT) && isKeyPressed(SDL_SCANCODE_RETURN)) || (isKeyPressed(SDL_SCANCODE_LALT) && isKeyDown(SDL_SCANCODE_RETURN)))
			switchScreenMode();

		if (isKeyPressed(keyJump))
		{
			playSound(SFX_YNConfirm);

			if (select == 0)
			{
				initGame();
				break;
			}
			loadProfile();
			break;
		}

		if (isKeyPressed(keyUp) || isKeyPressed(keyDown))
		{
			playSound(SFX_YNChangeChoice);
			select ^= 1;
		}

		if (++anime >= 40)
			anime = 0;

		// -- DRAW -- //
		SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
		SDL_RenderClear(renderer);

		//Draw version
		drawTexture(sprites[0x1A], &rcVersion, (screenWidth >> 1) - 60, 216);
		drawTexture(sprites[0x1A], &rcPeriod, (screenWidth >> 1) - 4, 216);
		drawNumber(version[0], (screenWidth >> 1) - 20, 216, false);
		drawNumber(version[1], (screenWidth >> 1) - 4, 216, false);
		drawNumber(version[2], (screenWidth >> 1) + 12, 216, false);
		drawNumber(version[3], (screenWidth >> 1) + 28, 216, false);

		//Draw title, new, load, and pixel 12.2004 thing
		drawTexture(sprites[0x00], &rcTitle, (screenWidth >> 1) - 72, 40);
		drawTexture(sprites[0x00], &rcNew, (screenWidth >> 1) - 24, 128);
		drawTexture(sprites[0x00], &rcLoad, (screenWidth >> 1) - 24, 148);
		drawTexture(sprites[0x01], &rcPixel, (screenWidth >> 1) - 80, 192);

		//Draw the character cursor
		RECT rcChar = { 0 + (frameOrder[(anime / 10) % 4] << 4), 16, 16 + (frameOrder[(anime / 10) % 4] << 4), 32 };
		drawTexture(sprites[0x10], &rcChar, (screenWidth >> 1) - 44, 127 + (20 * select));

		SDL_RenderPresent(renderer);
	}

	frame = SDL_GetTicks();
	while (SDL_GetTicks() < frame + 1000)
	{
		getKeys(&events);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
	}

	return PLAY;
}

int gameUpdateIntro()
{
	uint32_t frame = 0;
	loadLevel(72);
	startTscEvent(100);

	//Set up fade
	initFade();
	fade.bMask = true;

	while (frame < 500)
	{
		//Framerate limiter
		const Uint32 timeNow = SDL_GetTicks();
		const Uint32 timeNext = framerateTicks + framerate;

		if (timeNow >= timeNext)
		{
			framerateTicks = SDL_GetTicks();
		}
		else
		{
			SDL_Delay(timeNext - timeNow);
			continue;
		}
		frame++;

		//Handle events
		getKeys(&events);

		if (events.type == SDL_QUIT || exitGame)
			return 0;

		if (isKeyDown(SDL_SCANCODE_ESCAPE))
		{
			const int escape = escapeMenu();

			if (!escape)
				return 0;
			if (escape == 2)
				return 1;
		}

		if ((isKeyDown(SDL_SCANCODE_LALT) && isKeyPressed(SDL_SCANCODE_RETURN)) || (isKeyPressed(SDL_SCANCODE_LALT) && isKeyDown(SDL_SCANCODE_RETURN)))
			switchScreenMode();

		if (isKeyPressed(keyJump) || isKeyDown(keyShoot))
		{
			break;
		}

		updateNPC();
		updateCarets();

		handleView();

		updateFade();

		// -- DRAW -- //
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		drawLevel(false);
		drawNPC();
		drawLevel(true);
		drawCarets();
		drawValueView();

		drawFade();

		//Do TSC stuff
		updateTsc();
		drawTsc();

		drawMapName(false);

		SDL_RenderPresent(renderer);
	}

	//wait 500 ms
	frame = SDL_GetTicks();
	while (SDL_GetTicks() < frame + 500)
	{
		getKeys(&events);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
	}

	return MENU;
}

int mainGameLoop()
{
	while (gameMode != 0)
	{
		//////UPDATE//////
		switch (gameMode)
		{
		case(INTRO):
			gameMode = gameUpdateIntro();
			break;
		case(MENU):
			gameMode = gameUpdateMenu();
			break;
		case(PLAY):
			gameMode = gameUpdatePlay();
			break;
		default:
			doCustomError("Invalid gameMode");
			break;
		}
	}

	return 0;
}
