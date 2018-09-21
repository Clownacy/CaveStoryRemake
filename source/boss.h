#pragma once
#include "npc.h"

#define BOSSNPCS 20
extern npc bossObj[BOSSNPCS];

void initBoss(int code);

void setBossAction(int a);

void updateBoss();
void drawBoss();

void actBoss_Omega(npc *boss);
void actBoss_Balfrog(npc *boss);
//void actBoss_MonstX(npc *boss);
//void actBoss_Core(npc *boss);
//void actBoss_Ironhead(npc *boss);
//void actBoss_Twin(npc *boss);
//void actBoss_Undead(npc *boss);
void actBoss_HeavyPress(npc *boss);
//void actBoss_Ballos(npc *boss);
