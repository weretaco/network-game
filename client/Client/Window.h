#ifndef _WINDOW_H
#define _WINDOW_H

#include "GuiComponent.h"

#include <vector>

using namespace std;

class Window :
   public GuiComponent
{
private:
   vector<GuiComponent*> vctGui;

public:
   Window(int x, int y, int width, int height);
   ~Window(void);

   GuiComponent* addComponent(GuiComponent* comp);
   GuiComponent* getComponent(int x);
   void draw(ALLEGRO_DISPLAY *display);
   bool handleEvent(ALLEGRO_EVENT& e);
};

#endif