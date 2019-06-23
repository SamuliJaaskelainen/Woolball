// Rock that player can smash by running into it

#pragma bank 2
#include "main.h"
UINT8 bank_SPRITE_ROCK = 2;

#include "SpriteManager.h"

void Start_SPRITE_ROCK()
{
	THIS->coll_x = 0;
	THIS->coll_y = 0;
	THIS->coll_w = 16;
	THIS->coll_h = 16;
}

void Update_SPRITE_ROCK()
{
}

void Destroy_SPRITE_ROCK()
{
}