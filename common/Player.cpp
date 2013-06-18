#include "Player.h"

#include <iostream>
#include <sstream>
#include <cstring>
#include <cmath>

using namespace std;

Player::Player()
{
   this->id = 0;
   this->name = "";
   this->password = "";
   this->pos.x = this->target.x = 0;
   this->pos.y = this->target.y = 0;
   this->timeLastUpdated = 0;
   this->timeAttackStarted = 0;
   this->timeDied = 0;
   this->isChasing = false;
   this->isAttacking = false;
   this->isDead = false;

   this->playerClass = CLASS_NONE;
   this->maxHealth = 0;
   this->health = 0;
   this->attackType = ATTACK_NONE;
   this->damage = 0;
   this->range = 0;
   this->attackCooldown = 0;
   this->team = 0;   // blue team by default
   this->hasBlueFlag = false;
   this->hasRedFlag = false;
}

Player::Player(const Player& p)
{
   this->id = p.id;
   this->name = p.name;
   this->password = p.password;
   this->addr = p.addr;
   this->pos.x = p.pos.x;
   this->pos.y = p.pos.y;
   this->target.x = p.target.x;
   this->target.y = p.target.y;
   this->timeLastUpdated = p.timeLastUpdated;
   this->timeAttackStarted = p.timeAttackStarted;
   this->timeDied = p.timeDied;
   this->isChasing = p.isChasing;
   this->isAttacking = p.isAttacking;
   this->isDead = p.isDead;

   this->playerClass = p.playerClass;
   this->maxHealth = p.maxHealth;
   this->health = p.health;
   this->attackType = p.attackType;
   this->damage = p.damage;
   this->range = p.range;
   this->attackCooldown = p.attackCooldown;
   this->team = p.team;
   this->hasBlueFlag = p.hasBlueFlag;
   this->hasRedFlag = p.hasRedFlag;
}

// eventually make this take a PlayerClass argument as well
Player::Player(string name, string password)
{
   this->id = 0;
   this->name = name;
   this->password = password;
   this->pos.x = this->target.x = 200;
   this->pos.y = this->target.y = 200;
   this->timeLastUpdated = 0;
   this->timeAttackStarted = 0;
   this->timeDied = 0;
   this->isChasing = false;
   this->isAttacking = false;
   this->isDead = false;

   this->playerClass = CLASS_NONE;
   this->maxHealth = 0;
   this->health = 0;
   this->attackType = ATTACK_NONE;
   this->damage = 0;
   this->range = 0;
   this->attackCooldown = 0;
   this->team = 0;   // blue team by default
   this->hasBlueFlag = false;
   this->hasRedFlag = false;
}

Player::~Player()
{
}

void Player::setId(int id)
{
   this->id = id;
}

void Player::setAddr(sockaddr_in addr)
{
   this->addr = addr;
}

void Player::setClass(PlayerClass c)
{
   switch (c) {
      case CLASS_WARRIOR:
         this->playerClass = CLASS_WARRIOR;
         this->maxHealth = this->health = 120;
         this->attackType = ATTACK_MELEE;
         this->damage = 10;
         this->range = 30;
         this->attackCooldown = 800;
         break;
      case CLASS_RANGER:
         this->playerClass = CLASS_RANGER;
         this->maxHealth = this->health = 60;
         this->attackType = ATTACK_RANGED;
         this->damage = 6;
         this->range = 100;
         this->attackCooldown = 1000;
         break;
      case CLASS_NONE:
         cout << "No clas" << endl;
         break;
      dafault:
         cout << "nvalid class" << endl;
         break;
   }
}

void Player::serialize(char* buffer)
{
   memcpy(buffer, &this->id, 4);
   memcpy(buffer+4, &this->pos.x, 4);
   memcpy(buffer+8, &this->pos.y, 4);
   memcpy(buffer+12, &this->target.x, 4);
   memcpy(buffer+16, &this->target.y, 4);

   memcpy(buffer+20, &this->playerClass, 4);
   memcpy(buffer+24, &this->maxHealth, 4);
   memcpy(buffer+28, &this->health, 4);
   memcpy(buffer+32, &this->attackType, 4);
   memcpy(buffer+36, &this->damage, 4);
   memcpy(buffer+40, &this->team, 4);
   memcpy(buffer+44, &this->hasBlueFlag, 1);
   memcpy(buffer+45, &this->hasRedFlag, 1);
   memcpy(buffer+46, &this->range, 4);

   strcpy(buffer+50, this->name.c_str());
}

void Player::deserialize(char* buffer)
{
   memcpy(&this->id, buffer, 4);
   memcpy(&this->pos.x, buffer+4, 4);
   memcpy(&this->pos.y, buffer+8, 4);
   memcpy(&this->target.x, buffer+12, 4);
   memcpy(&this->target.y, buffer+16, 4);

   memcpy(&this->playerClass, buffer+20, 4);
   memcpy(&this->maxHealth, buffer+24, 4);
   memcpy(&this->health, buffer+28, 4);
   memcpy(&this->attackType, buffer+32, 4);
   memcpy(&this->damage, buffer+36, 4);
   memcpy(&this->team, buffer+40, 4);
   memcpy(&this->hasBlueFlag, buffer+44, 1);
   memcpy(&this->hasRedFlag, buffer+45, 1);
   memcpy(&this->range, buffer+46, 4);

   this->name.assign(buffer+50);
}

bool Player::move(WorldMap *map) {
   int speed = 100; // pixels per second. should probably be in the constructor
   unsigned long long curTime = getCurrentMillis();

   // if we're at our target, don't move
   bool moving = (pos.x != target.x || pos.y != target.y);

   if (moving) {
      float pixels = speed * (curTime-timeLastUpdated) / 1000.0;
      double angle = atan2(target.y-pos.y, target.x-pos.x);
      float dist = sqrt(pow(target.x-pos.x, 2) + pow(target.y-pos.y, 2));

      if (dist <= pixels) {
         pos.x = target.x;
         pos.y = target.y;
      }else {
         pos.x = pos.x + cos(angle)*pixels;
         pos.y = pos.y + sin(angle)*pixels;
      }
   }

   timeLastUpdated = curTime;

   return moving;
}

bool Player::updateTarget(map<unsigned int, Player>& mapPlayers) {
   if (this->isChasing) {
      this->target.x = mapPlayers[this->targetPlayer].pos.x;
      this->target.y = mapPlayers[this->targetPlayer].pos.y;

      if (posDistance(this->pos, this->target.toFloat()) <= this->range) {
         this->target.x = this->pos.x;
         this->target.y = this->pos.y;

         this->isChasing = false;
         this->isAttacking = true;
         this->timeAttackStarted = getCurrentMillis();

         return true;
      }
   }

   return false;
}
