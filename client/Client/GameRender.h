#ifndef _GAMERENDER_H
#define _GAMERENDER_H

/*
 * We don't want to place allegro drawing routines in any classes shared by the client and server
 * because the server shouldn't require Allegro
 */

#include "../../common/Compiler.h"

#include <map>

#ifdef WINDOWS
   #define WIN32_LEAN_AND_MEAN
#endif

#include <allegro5/allegro_font.h>

#include "../../common/Player.h"
#include "../../common/Projectile.h"
#include "../../common/WorldMap.h"

class GameRender
{
public:
   static void drawMap(WorldMap* gameMap);
   static void drawPlayers(map<unsigned int, Player*>& mapPlayers, ALLEGRO_FONT* font, unsigned int curPlayerId);
   static void drawProjectiles(map<unsigned int, Projectile>& mapProjectiles, map<unsigned int, Player*>& mapPLayers);
};

#endif

