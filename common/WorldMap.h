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

   enum ObjectType {
      OBJECT_NONE,
      OBJECT_RED_FLAG,
      OBJECT_BLUE_FLAG
   };

   int width, height;
   vector<vector<TerrainType>*>* vctMap;
   vector<vector<ObjectType>*>* vctObjects;

   WorldMap(int width, int height);

   ~WorldMap();

   TerrainType getElement(int x, int y);
   void setElement(int x, int y, TerrainType type);

   ObjectType getObject(int x, int y);
   void setObject(int x, int y, ObjectType type);

   static WorldMap* createDefaultMap();
   static WorldMap* loadMapFromFile(string filename);
};

#endif
