#ifndef _GUICOMPONENT_H
#define _GUICOMPONENT_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>

class GuiComponent
{
protected:
   int x, y, width, height;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_FONT *font;

public:
   GuiComponent(int x, int y, int width, int height, ALLEGRO_FONT *font);
   virtual ~GuiComponent(void);

   virtual void draw(ALLEGRO_DISPLAY *display);
   virtual bool handleEvent(ALLEGRO_EVENT& e);
};

#endif