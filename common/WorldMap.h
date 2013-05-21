#ifndef _WORLDMAP_H
#define _WORLDMAP_H

#include <string>

#include <vector>

#include "Common.h"

using namespace std;

class WorldMap {
public:
   enum TerrainType {
      TERRAIN_NONE,
      TERRAIN_GRASS,
      TERRAIN_OCEAN,
      TERRAIN_ROCK
   };

   enum StructureType {
      STRUCTURE_NONE,
      STRUCTURE_BLUE_FLAG,
      STRUCTURE_RED_FLAG
   };

   enum ObjectType {
      OBJECT_NONE,
      OBJECT_BLUE_FLAG,
      OBJECT_RED_FLAG
   };

   class Object {
   public:
      int id;
      ObjectType type;
      POSITION pos;

      Object(ObjectType type, int id, int x, int y);
      Object(ObjectType type, int id, POSITION pos);

      ~Object();
   };

   int width, height;
   vector<vector<TerrainType>*>* vctMap;
   vector<vector<StructureType>*>* vctStructures;
   vector<Object>* vctObjects; 

   WorldMap(int width, int height);

   ~WorldMap();

   TerrainType getElement(int x, int y);
   void setElement(int x, int y, TerrainType type);

   StructureType getStructure(int x, int y);
   void setStructure(int x, int y, StructureType type);

   vector<Object> getObjects(int x, int y);
   void addObject(ObjectType type, int x, int y);
   void updateObject(int id, WorldMap::ObjectType t, int x, int y);

   static WorldMap* createDefaultMap();
   static WorldMap* loadMapFromFile(string filename);
};

#endif
