#include "Game.h"

#include <allegro5/allegro_primitives.h>

#include "Common.h"

using namespace std;

Game::Game() {
   this->id = 0;
   this->name = "";
   this->blueScore = 0;
   this->redScore = 0;
   this->worldMap = NULL;
}

Game::Game(string name, string filepath) {
   this->id = 0;
   this->name = name;
   this->blueScore = 0;
   this->redScore = 0;
   this->worldMap = WorldMap::loadMapFromFile(filepath);
}

Game::~Game() {
   delete this->worldMap;
}

string Game::getName() {
   return this->name;
}

int Game::getNumPlayers() {
   return this->players.size();
}

map<unsigned int, Player*>& Game::getPlayers() {
   return this->players;
}

int Game::getRedScore() {
   return this->redScore;
}

int Game::getBlueScore() {
   return this->blueScore;
}

WorldMap* Game::getMap() {
   return this->worldMap;
}

void Game::setId(unsigned int id) {
   this->id = id;
}

bool Game::addPlayer(Player* p) {
   if (players.find(p->id) == players.end()) {
      players[p->id] = p;
      return true;
   }
   else
      return false;
}

bool Game::removePlayer(unsigned int id) {
   if (players.erase(id) == 1)
      return true;
   else
      return false;
}

void Game::setRedScore(int score) {
   this->redScore = score;
}

void Game::setBlueScore(int score) {
   this->blueScore = score;
}

void Game::drawPlayers(ALLEGRO_FONT* font, unsigned int curPlayerId) {
   map<unsigned int, Player*>::iterator it;

   Player* p;
   POSITION pos;
   ALLEGRO_COLOR color;

   for(it = players.begin(); it != players.end(); it++)
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
