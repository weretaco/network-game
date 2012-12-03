#include "Textbox.h"

#include <iostream>

using namespace std;

Textbox::Textbox(int x, int y, int width, int height, ALLEGRO_FONT *font) :
   GuiComponent(x, y, width, height, font)
{
   str = "";
   selected = false;
}

Textbox::~Textbox(void)
{
}

const string& Textbox::getStr() const
{
   return str;
}

void Textbox::clear(void)
{
   str.clear();
}

void Textbox::draw(ALLEGRO_DISPLAY *display)
{
   al_set_target_bitmap(bitmap);
   al_clear_to_color(al_map_rgb(0, 0, 0));

   int textWidth = al_get_text_width(font, str.c_str());
   int fontHeight = al_get_font_line_height(font);
   
   int textPos = 1;
   if(textWidth > this->width)
      textPos = this->width-textWidth-3;

   al_draw_text(font, al_map_rgb(0, 255, 0), textPos, (this->height-fontHeight)/2, ALLEGRO_ALIGN_LEFT, str.c_str());
   al_draw_rectangle(1, 1, this->width, this->height, al_map_rgb(0, 255, 0), 1);

   al_set_target_bitmap(al_get_backbuffer(display));
   al_draw_bitmap(bitmap, x, y, 0);
}

bool Textbox::handleEvent(ALLEGRO_EVENT& e)
{
   ALLEGRO_KEYBOARD_STATE keys;
   al_get_keyboard_state(&keys);

   if (e.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
      if (e.mouse.button == 1) {
         if (this->x < e.mouse.x && e.mouse.x < (this->x+this->width)
            && this->y < e.mouse.y && e.mouse.y < (this->y+this->height))
         {
            selected = true;
            return true;
         }else
            selected = false;
      }

      return false;
   }

   if (!selected)
      return false;

   if (e.type == ALLEGRO_EVENT_KEY_DOWN) {
      char newChar = 0;

      if (ALLEGRO_KEY_A <= e.keyboard.keycode && e.keyboard.keycode <= ALLEGRO_KEY_Z) {
         newChar = 'a'+e.keyboard.keycode-ALLEGRO_KEY_A;
         if (al_key_down(&keys, ALLEGRO_KEY_LSHIFT) || al_key_down(&keys, ALLEGRO_KEY_RSHIFT))
            newChar -= 32;
      }
      else if (ALLEGRO_KEY_0 <= e.keyboard.keycode && e.keyboard.keycode <= ALLEGRO_KEY_9)
         newChar = '0'+e.keyboard.keycode-ALLEGRO_KEY_0;
      else if (e.keyboard.keycode = ALLEGRO_KEY_BACKSPACE && str.size() > 0)
         str = str.substr(0, str.size()-1);

      if (newChar != 0) {
         str.append(1, newChar);
         return true;
      }
   }

   return false;
}
