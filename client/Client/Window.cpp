#include "Window.h"

Window::Window(int x, int y, int width, int height) :
   GuiComponent(x, y, width, height, NULL)
{
}

Window::~Window(void)
{
   for(unsigned int x=0; x<this->vctGui.size(); x++)
      delete this->vctGui[x];
}

void Window::addComponent(GuiComponent *comp)
{
   this->vctGui.push_back(comp);
}

GuiComponent* Window::getComponent(int x)
{
   return this->vctGui[x];
}

void Window::draw(ALLEGRO_DISPLAY *display)
{
   al_clear_to_color(al_map_rgb(0, 0, 0));

   for(unsigned int x=0; x<this->vctGui.size(); x++)
      this->vctGui[x]->draw(display);
}

bool Window::handleEvent(ALLEGRO_EVENT& e)
{
   for(unsigned int x=0; x<this->vctGui.size(); x++) {
      if(this->vctGui[x]->handleEvent(e)) {
         return true;
      }
   }

   return false;
}