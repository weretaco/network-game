#include "WorldMap.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

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

vector<WorldMap::Object> WorldMap::getObjects(int x, int y) {
   vector<WorldMap::Object> vctObjectsInRegion;

   return vctObjectsInRegion;
}

void WorldMap::addObject(int x, int y, WorldMap::ObjectType t) {
   WorldMap::Object o(t, x, y);

   vctObjects->push_back(o);
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
   WorldMap* m = new WorldMap(12l, 12);

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

      // read the map contents
      int row = 0;
      while ( file.good() )
      {
         getline(file, line);
         if (line.size() > 0)
         {
            cout << "line: " << line << endl;

            istringstream iss(line);
            string token;

            if (row < height) {
               // load terrain

               int type;
               TerrainType terrain;

               for(int x=0; x<width; x++)
               {
                  getline(iss, token, ',');
                  cout << "token: " << token << endl;
                  type = atoi(token.c_str());
                  cout << "type: " << type << endl;

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

                  cout << "About to set element" << endl;
                  cout << "x: " << x << endl;
                  cout << "row: " << row << endl;
                  m->setElement(x, row, terrain);
               }
            }else {
               // load objects

               int x, y, type;
               StructureType structure;

               getline(iss, token, ',');
               cout << "token(x): " << token << endl;
               x = atoi(token.c_str());

               getline(iss, token, ',');
               cout << "token(y): " << token << endl;
               y = atoi(token.c_str());

               getline(iss, token, ',');
               cout << "token(type): " << token << endl;
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

WorldMap::Object::Object(ObjectType type, POSITION pos) {
   this->type = type;
   this->pos = pos;
}

WorldMap::Object::Object(ObjectType type, int x, int y) {
   this->type = type;
   this->pos.x = x;
   this->pos.y = y;
}

WorldMap::Object::~Object() {
}
