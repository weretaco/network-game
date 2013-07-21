#ifndef _BUTTON_H
#define _BUTTON_H

#include "GuiComponent.h"

#include <string>

using namespace std;

class Button :
   public GuiComponent
{
private:
   string str;
   void (*callback)();

public:
   Button(int x, int y, int width, int height, ALLEGRO_FONT *font, string str, void (*callback)());
   ~Button(void);

   void draw(ALLEGRO_DISPLAY *display);
   bool handleEvent(ALLEGRO_EVENT& e);
};

#endif