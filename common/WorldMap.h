#ifndef _WORLDMAP_H
#define _WORLDMAP_H

#include <string>

#include <vector>

using namespace std;

class WorldMap {
public:
   enum TerrainType {
      TERRAIN_NONE,
      TERRAIN_GRASS,
      TERRAIN_OCEAN,
      TERRAIN_ROCK
   };

   int width, height;
   vector<vector<TerrainType>*>* vctMap;

   WorldMap(int width, int height);

   ~WorldMap();

   TerrainType getElement(int x, int y);
   void setElement(int x, int y, TerrainType type);

   static WorldMap* createDefaultMap();
   static WorldMap* loadMapFromFile(string filename);
};

#endif
