#include "WorldMap.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>

using namespace std;

WorldMap::WorldMap(int width, int height)
{
   this->width = width;
   this->height = height;

   vctMap = new vector<vector<TerrainType>*>(width);
   vctStructures = new vector<vector<StructureType>*>(width);
   vctObjects = new vector<Object>();

   for (int x=0; x<width; x++) {
      vector<TerrainType>* newMapVector = new vector<TerrainType>(height);
      vector<StructureType>* newStructureVector = new vector<StructureType>(height);

      for (int y=0; y<height; y++) {
         (*newMapVector)[y] = TERRAIN_NONE;
         (*newStructureVector)[y] = STRUCTURE_NONE;
      }

      (*vctMap)[x] = newMapVector;
      (*vctStructures)[x] = newStructureVector;
   }
}

WorldMap::~WorldMap()
{
   for (int x=0; x<width; x++) {
      delete (*vctMap)[x];
      delete (*vctStructures)[x];
   }

   delete vctMap;
   delete vctStructures;
   delete vctObjects;
}

WorldMap::TerrainType WorldMap::getElement(int x, int y)
{
   return (*(*vctMap)[x])[y];
}

void WorldMap::setElement(int x, int y, TerrainType t)
{
   (*(*vctMap)[x])[y] = t;
}

WorldMap::StructureType WorldMap::getStructure(int x, int y)
{
   return (*(*vctStructures)[x])[y];
}

void WorldMap::setStructure(int x, int y, StructureType t)
{
   (*(*vctStructures)[x])[y] = t;
}

POSITION WorldMap::getStructureLocation(StructureType t)
{
   POSITION pos;
   pos.x = 0;
   pos.y = 0;

   for (int x=0; x<vctStructures->size(); x++) {
      for (int y=0; y<(*vctStructures)[x]->size(); y++) {
        if ((*(*vctStructures)[x])[y] == t) {
           pos.x = x;
           pos.y = y;
           return pos;
        } 
      }
   }

   return pos;
}

vector<WorldMap::Object>* WorldMap::getObjects() {
   return vctObjects;
}

vector<WorldMap::Object> WorldMap::getObjects(int x, int y) {
   vector<WorldMap::Object> vctObjectsInRegion;

   vector<WorldMap::Object>::iterator it;
   for (it = vctObjects->begin(); it != vctObjects->end(); it++) {
      if (it->pos.x/25 == x && it->pos.y/25 == y)
         vctObjectsInRegion.push_back(*it);
   }

   return vctObjectsInRegion;
}

// used by the server to create new objects
void WorldMap::addObject(WorldMap::ObjectType t, int x, int y) {
   int id;
   vector<WorldMap::Object>::iterator it;

   for (id = 0; id < vctObjects->size(); id++) {
      for (it = vctObjects->begin(); it != vctObjects->end(); it++) {
         if (id == it->id)
            break;
      }

      if (it == vctObjects->end())  // if no objects with this id exists
         break;
   }

   WorldMap::Object o(id, t, x, y);
   vctObjects->push_back(o);
}

// used by the client to update object positions or create objects it has not seen before
void WorldMap::updateObject(int id, WorldMap::ObjectType t, int x, int y) {
   vector<WorldMap::Object>::iterator it;
   bool foundObject = false;

   cout << "Searching for obbject to update" << endl;
   switch (t) {
   case WorldMap::OBJECT_BLUE_FLAG:
      cout << "BLUE_FLAG" << endl;
      break;
   case WorldMap::OBJECT_RED_FLAG:
      cout << "RED_FLAG" << endl;
      break;
   }

   for (it = vctObjects->begin(); it != vctObjects->end(); it++) {
      if (it->id == id) {
         foundObject = true;
         cout << "Found object with id " << id << endl;
         switch (it->type) {
         case WorldMap::OBJECT_BLUE_FLAG:
            cout << "BLUE_FLAG" << endl;
            break;
         case WorldMap::OBJECT_RED_FLAG:
            cout << "RED_FLAG" << endl;
            break;
         }
         it->pos.x = x;
         it->pos.y = y;
      }
   }

   if (!foundObject) {
      WorldMap::Object o(id, t, x, y);
      vctObjects->push_back(o);
   }
}

bool WorldMap::removeObject(int id) {
   vector<WorldMap::Object>::iterator it;

   for (it = vctObjects->begin(); it != vctObjects->end(); it++) {
      if (it->id == id) {
         vctObjects->erase(it);
         return true;
      }
   }

   return false;  // no object with that id was found
}

WorldMap* WorldMap::createDefaultMap()
{
   WorldMap* m = new WorldMap(12l, 12);

   for(int x=0; x<12; x++)
   {   
      for(int y=0; y<12; y++)
      {
         if (x ==0 || y == 0 || x == 11 || y == 11)
            m->setElement(x, y, TERRAIN_OCEAN);
         else
            m->setElement(x, y, TERRAIN_GRASS);

         m->setStructure(x, y, STRUCTURE_NONE);
      }
   }

   m->setElement(5, 5, TERRAIN_ROCK);

   return m;
}

WorldMap* WorldMap::loadMapFromFile(string filename)
{
   WorldMap* m = NULL;

   ifstream file(filename.c_str());

   if (file.is_open())
   {
      string line;
      int width, height;

      // read the map dimensions
      getline(file, line);
      if (line.size() > 0)
      {
         istringstream iss(line);
         string token;
         getline(iss, token, 'x');
         width = atoi(token.c_str());
         getline(iss, token, 'x');
         height = atoi(token.c_str());
      }

      cout << "width: " << width << endl;
      cout << "height: " << height << endl;

      m = new WorldMap(width, height);

      // read the map contents
      int row = 0;
      while ( file.good() )
      {
         getline(file, line);
         if (line.size() > 0)
         {
            //cout << "row: " << row << endl;
            //cout << "line: " << line << endl;

            istringstream iss(line);
            string token;

            if (row < height) {
               // load terrain

               int type;
               TerrainType terrain;

               for(int x=0; x<width; x++)
               {
                  getline(iss, token, ',');
                  type = atoi(token.c_str());

                  //cout << "x: " << x << endl;
                  //cout << "token: " << token << endl;
                  //cout << "type: " << type << endl;

                  switch(type) {
                  case 1:
                     terrain = TERRAIN_GRASS;
                     break;
                  case 2:
                     terrain = TERRAIN_OCEAN;
                     break;
                  case 3:
                     terrain = TERRAIN_ROCK;
                     break;
                  }

                  m->setElement(x, row, terrain);
               }
            }else {
               // load structure

               int x, y, type;
               StructureType structure;

               getline(iss, token, ',');
               //cout << "token(x): " << token << endl;
               x = atoi(token.c_str());

               getline(iss, token, ',');
               //cout << "token(y): " << token << endl;
               y = atoi(token.c_str());

               getline(iss, token, ',');
               //cout << "token(type): " << token << endl;
               type = atoi(token.c_str());

               switch(type) {
               case 0:
                  structure = STRUCTURE_NONE;
                  break;
               case 1:
                  structure = STRUCTURE_BLUE_FLAG;
                  break;
               case 2:
                  structure = STRUCTURE_RED_FLAG;
                  break;
               }

               m->setStructure(x, y, structure);
            }
         }

         row++;
      }
      file.close();
   }
   else
      cout << "Could not open the file" << endl;

   return m;
}


/*** Functions for Object ***/

WorldMap::Object::Object(int id, ObjectType type, int x, int y) {
   this->type = type;
   this->id = id;
   this->pos.x = x;
   this->pos.y = y;
}

WorldMap::Object::Object(int id, ObjectType type, POSITION pos) {
   this->type = type;
   this->id = id;
   this->pos = pos;
}

WorldMap::Object::~Object() {
}

void WorldMap::Object::serialize(char* buffer) {
   memcpy(buffer, &this->type, 4);
   memcpy(buffer+4, &this->id, 4);
   memcpy(buffer+8, &this->pos.x, 4);
   memcpy(buffer+12, &this->pos.y, 4);
}

void WorldMap::Object::deserialize(char* buffer) {
   memcpy(&this->type, buffer, 4);
   memcpy(&this->id, buffer+4, 4);
   memcpy(&this->pos.x, buffer+8, 4);
   memcpy(&this->pos.y, buffer+12, 4);
}
