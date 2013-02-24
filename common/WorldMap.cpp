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

   for (int x=0; x<width; x++) {
      vector<TerrainType>* newVector = new vector<TerrainType>(height);

      for (int y=0; y<height; y++)
         (*newVector)[y] = TERRAIN_NONE;

      (*vctMap)[x] = newVector;
   }
}

WorldMap::~WorldMap()
{
   for (int x=0; x<width; x++)
      delete (*vctMap)[x];

   delete vctMap;
}

WorldMap::TerrainType WorldMap::getElement(int x, int y)
{
   return (*(*vctMap)[x])[y];
}

void WorldMap::setElement(int x, int y, TerrainType t)
{
   cout << "getting element" << endl;
   (*(*vctMap)[x])[y] = t;
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
         }

         row++;
      }
      file.close();
   }
   else
      cout << "Could not open the file" << endl;

   return m;
}
