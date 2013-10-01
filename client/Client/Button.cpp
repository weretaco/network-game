#include "Button.h"

#include "../../common/Compiler.h"

Button::Button(int x, int y, int width, int height, ALLEGRO_FONT *font, string str, void (*callback)()) :
   GuiComponent(x, y, width, height, font)
{
   this->str = str;
   this->callback = callback;
}

Button::~Button(void)
{
}

void Button::draw(ALLEGRO_DISPLAY *display)
{
   al_set_target_bitmap(bitmap);
   al_clear_to_color(al_map_rgb(0, 0, 0));

   int fontHeight = al_get_font_line_height(font);

   al_draw_text(font, al_map_rgb(0, 255, 0), this->width/2, (this->height-fontHeight)/2, ALLEGRO_ALIGN_CENTRE, str.c_str());

   #ifdef WINDOWS
      al_draw_rectangle(1, 1, this->width, this->height, al_map_rgb(0, 255, 0), 1);
   #else
      al_draw_rectangle(1, 0, this->width, this->height-1, al_map_rgb(0, 255, 0), 1);
   #endif

   al_set_target_bitmap(al_get_backbuffer(display));
   al_draw_bitmap(bitmap, x, y, 0);
}

bool Button::handleEvent(ALLEGRO_EVENT& e)
{
   if (e.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
      if (e.mouse.button == 1) {
         if (this->x < e.mouse.x && e.mouse.x < (this->x+this->width)
            && this->y < e.mouse.y && e.mouse.y < (this->y+this->height))
         {
            this->callback();
            return true;
         }
      }
   }

   return false;
}
