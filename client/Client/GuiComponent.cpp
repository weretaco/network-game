#include "GuiComponent.h"

GuiComponent::GuiComponent(int x, int y, int width, int height, ALLEGRO_FONT *font)
{
   this->x = x;
   this->y = y;
   this->width = width;
   this->height = height;
   this->font = font;
   this->bitmap = al_create_bitmap(width, height);
}

GuiComponent::~GuiComponent(void)
{
   al_destroy_bitmap(this->bitmap);
}

void GuiComponent::draw(ALLEGRO_DISPLAY *display)
{
}

bool GuiComponent::handleEvent(ALLEGRO_EVENT& e)
{
   return false;
}
