#ifndef _TEXTLABEL_H
#define _TEXTLABEL_H

#include "GuiComponent.h"

#include <string>

using namespace std;

class TextLabel :
   public GuiComponent
{
private:
   string str;
   int alignment;

public:
   TextLabel(int x, int y, int width, int height, ALLEGRO_FONT *font, string str, int alignment);
   ~TextLabel(void);

   void draw(ALLEGRO_DISPLAY *display);
   bool handleEvent(ALLEGRO_EVENT& e);

   void setText(string str);
};

#endif