#include "GameRender.h"

#include <allegro5/allegro_primitives.h>

#include "../../common/Common.h"

void GameRender::drawMap(WorldMap* gameMap)
{
   POSITION mapPos;
   mapPos.x = 0;
   mapPos.y = 0;
   mapPos = mapToScreen(mapPos);

   for (int x=0; x<gameMap->width; x++)
   {
      for (int y=0; y<gameMap->height; y++)
      {
         WorldMap::TerrainType el = gameMap->getElement(x, y);
         WorldMap::StructureType structure = gameMap->getStructure(x, y);

         if (el == WorldMap::TERRAIN_GRASS)
            al_draw_filled_rectangle(x*25+mapPos.x, y*25+mapPos.y, x*25+25+mapPos.x, y*25+25+mapPos.y, al_map_rgb(0, 255, 0));
         else if (el == WorldMap::TERRAIN_OCEAN)
            al_draw_filled_rectangle(x*25+mapPos.x, y*25+mapPos.y, x*25+25+mapPos.x, y*25+25+mapPos.y, al_map_rgb(0, 0, 255));
         else if (el == WorldMap::TERRAIN_ROCK)
            al_draw_filled_rectangle(x*25+mapPos.x, y*25+mapPos.y, x*25+25+mapPos.x, y*25+25+mapPos.y, al_map_rgb(100, 100, 0));

         if (structure == WorldMap::STRUCTURE_BLUE_FLAG) {
            al_draw_circle(x*25+12+mapPos.x, y*25+12+mapPos.y, 12, al_map_rgb(0, 0, 0), 3);
            //al_draw_filled_rectangle(x*25+5+mapPos.x, y*25+5+mapPos.y, x*25+20+mapPos.x, y*25+20+mapPos.y, al_map_rgb(0, 0, 255));
         }else if (structure == WorldMap::STRUCTURE_RED_FLAG) {
            al_draw_circle(x*25+12+mapPos.x, y*25+12+mapPos.y, 12, al_map_rgb(0, 0, 0), 3);
            //al_draw_filled_rectangle(x*25+5+mapPos.x, y*25+5+mapPos.y, x*25+20+mapPos.x, y*25+20+mapPos.y, al_map_rgb(255, 0, 0));
         }
      }
   }

   for (int x=0; x<gameMap->width; x++)
   {
      for (int y=0; y<gameMap->height; y++)
      {
         vector<WorldMap::Object> vctObjects = gameMap->getObjects(x, y);

         vector<WorldMap::Object>::iterator it;
         for(it = vctObjects.begin(); it != vctObjects.end(); it++) {
            switch(it->type) {
               case WorldMap::OBJECT_BLUE_FLAG:
                  al_draw_filled_rectangle(it->pos.x-8+mapPos.x, it->pos.y-8+mapPos.y, it->pos.x+8+mapPos.x, it->pos.y+8+mapPos.y, al_map_rgb(0, 0, 255));
                  break;
               case WorldMap::OBJECT_RED_FLAG:
                  al_draw_filled_rectangle(it->pos.x-8+mapPos.x, it->pos.y-8+mapPos.y, it->pos.x+8+mapPos.x, it->pos.y+8+mapPos.y, al_map_rgb(255, 0, 0));
                  break;
            }
         }
      }
   }
}

void GameRender::drawPlayers(map<unsigned int, Player*>& mapPlayers, ALLEGRO_FONT* font, unsigned int curPlayerId)
{
   map<unsigned int, Player*>::iterator it;

   Player* p;
   POSITION pos;
   ALLEGRO_COLOR color;

   for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
   {
      p = it->second;

      if (p->isDead)
         continue;

      pos = mapToScreen(p->pos.toInt());

      if (p->id == curPlayerId)
         al_draw_filled_circle(pos.x, pos.y, 14, al_map_rgb(0, 0, 0));
      
      if (p->team == 0)
         color = al_map_rgb(0, 0, 255);
      else if (p->team == 1)
         color = al_map_rgb(255, 0, 0);
      
      al_draw_filled_circle(pos.x, pos.y, 12, color);

      // draw player class
      int fontHeight = al_get_font_line_height(font);

      string strClass;
      switch (p->playerClass) {
      case Player::CLASS_WARRIOR:
         strClass = "W";
         break;
      case Player::CLASS_RANGER:
         strClass = "R";
         break;
      case Player::CLASS_NONE:
         strClass = "";
         break;
      default:
         strClass = "";
         break;
      }
      al_draw_text(font, al_map_rgb(0, 0, 0), pos.x, pos.y-fontHeight/2, ALLEGRO_ALIGN_CENTRE, strClass.c_str());

      // draw player health
      al_draw_filled_rectangle(pos.x-12, pos.y-24, pos.x+12, pos.y-16, al_map_rgb(0, 0, 0));
      if (p->maxHealth != 0)
         al_draw_filled_rectangle(pos.x-11, pos.y-23, pos.x-11+(22*p->health)/p->maxHealth, pos.y-17, al_map_rgb(255, 0, 0));

      if (p->hasBlueFlag)
         al_draw_filled_rectangle(pos.x+4, pos.y-18, pos.x+18, pos.y-4, al_map_rgb(0, 0, 255));
      else if (p->hasRedFlag)
         al_draw_filled_rectangle(pos.x+4, pos.y-18, pos.x+18, pos.y-4, al_map_rgb(255, 0, 0));
   }
}

void GameRender::drawProjectiles(map<unsigned int, Projectile>& mapProjectiles, map<unsigned int, Player*>& mapPlayers)
{
   map<unsigned int, Projectile>::iterator it;
   for (it = mapProjectiles.begin(); it != mapProjectiles.end(); it++)
   {
      Projectile proj = it->second;

      FLOAT_POSITION target = mapPlayers[proj.target]->pos;
      float angle =  atan2(target.y-proj.pos.toFloat().y, target.x-proj.pos.toFloat().x);

      POSITION start, end;
      start.x = cos(angle)*15+proj.pos.x;
      start.y = sin(angle)*15+proj.pos.y;
      end.x = proj.pos.x;
      end.y = proj.pos.y;

      start = mapToScreen(start);
      end = mapToScreen(end);

      al_draw_line(start.x, start.y, end.x, end.y, al_map_rgb(0, 0, 0), 4);
   }
}