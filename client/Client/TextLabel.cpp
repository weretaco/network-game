#include "TextLabel.h"

#include <iostream>

TextLabel::TextLabel(int x, int y, int width, int height, ALLEGRO_FONT *font, string str, int alignment) :
   GuiComponent(x, y, width, height, font)
{
   this->str = str;
   this->alignment = alignment;
}

TextLabel::~TextLabel(void)
{
}

void TextLabel::draw(ALLEGRO_DISPLAY *display)
{
   al_set_target_bitmap(bitmap);
   al_clear_to_color(al_map_rgb(0, 0, 0));

   int fontHeight = al_get_font_line_height(font);
   int targetX;

   switch(this->alignment) {
      case ALLEGRO_ALIGN_LEFT:
         targetX = 0;
         break;
      case ALLEGRO_ALIGN_RIGHT:
         targetX = this->width;
         break;
      case ALLEGRO_ALIGN_CENTRE:
         targetX = this->width/2;
         break;
      default:
         cout << "Invalid alignment: " << this->alignment << endl;
         targetX = 0;
         break;
   }

   al_draw_text(font, al_map_rgb(0, 255, 0), targetX, (this->height-fontHeight)/2, this->alignment, this->str.c_str());

   al_set_target_bitmap(al_get_backbuffer(display));
   al_draw_bitmap(bitmap, x, y, 0);
}

bool TextLabel::handleEvent(ALLEGRO_EVENT& e)
{
   return false;
}

void TextLabel::setText(string str) {
   this->str = str;
}
