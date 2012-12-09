#ifndef _TEXTBOX_H
#define _TEXTBOX_H

#include "GuiComponent.h"

#include <string>
#include <map>

using namespace std;

class Textbox :
   public GuiComponent
{
private:
   string str;
   bool selected;
   bool shiftPressed;
   map<char, char> shiftMap;

public:
   Textbox(int x, int y, int width, int height, ALLEGRO_FONT *font);
   ~Textbox(void);

   // This might lead to a memory leak if the textbox is deleted because it
   // returns a reference to the string.
   // string getStr() const; will copy the string and return it instead
   const string& getStr() const;

   void clear(void);
   void draw(ALLEGRO_DISPLAY *display);
   bool handleEvent(ALLEGRO_EVENT& e);
};

#endif

