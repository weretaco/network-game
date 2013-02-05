#ifndef _MAP_H
#define _MAP_H

#include <vector>

using namespace std;

class Map {
public:
   enum TerrainType {
      TERRAIN_NONE,
      TERRAIN_GRASS,
      TERRAIN_OCEAN
   };

   int width, height;
   vector<vector<TerrainType>*>* vctMap;

   Map(int width, int height);

   ~Map();

   void setElement(int x, int y, TerrainType type);

   static Map* createDefaultMap();
};

#endif
