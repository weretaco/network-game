#include "Map.h"

using namespace std;

Map::Map(int width, int height)
{
   this->width = width;
   this->height = height;

   vctMap = new vector<vector<TerrainType>*>(width);

   for (int x=0; x<width; x++) {
      vector<TerrainType>* newVector = new vector<TerrainType>(height);

      for (int y=0; y<height; y++)
         (*newVector)[y] = TERRAIN_NONE;

      (*vctMap)[x] = newVector;
   }
}

Map::~Map()
{
   for (int x=0; x<width; x++)
      delete (*vctMap)[x];

   delete vctMap;
}

void Map::setElement(int x, int y, TerrainType t)
{
   (*(*vctMap)[x])[y] = t;
}

Map* Map::createDefaultMap()
{
   Map* m = new Map(20l, 20);

   for(int x=0; x<20; x++)
   {   
      for(int y=0; y<20; y++)
      {
         if (x ==0 || y == 0 || x == 19 || y == 19)
            m->setElement(x, y, TERRAIN_OCEAN);
         else
            m->setElement(x, y, TERRAIN_GRASS);
      }
   }

   return m;
}
