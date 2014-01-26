#include "RadioButtonList.h"

#include <string>
#include <cmath>

using namespace std;

RadioButtonList::RadioButtonList(int x, int y, string strLabel, ALLEGRO_FONT *font) :
   GuiComponent(x, y, 300, 300, font)
{
   this->strLabel = strLabel;
   this->selectedButton = -1;
}


RadioButtonList::~RadioButtonList(void)
{
}

void RadioButtonList::draw(ALLEGRO_DISPLAY *display)
{
   al_set_target_bitmap(bitmap);
   al_clear_to_color(al_map_rgb(0, 0, 0));

   //int fontHeight = al_get_font_line_height(font);

   al_draw_text(font, al_map_rgb(0, 255, 0), 0, 0, ALLEGRO_ALIGN_LEFT, this->strLabel.c_str());
   for(unsigned int i=0; i<this->vctRadioButtons.size(); i++) {
      al_draw_circle(12, 26+i*20, 8, al_map_rgb(0, 255, 0), 1);
      if (i == this->selectedButton)
         al_draw_filled_circle(12, 26+i*20, 6, al_map_rgb(0, 255, 0));
      al_draw_text(font, al_map_rgb(0, 255, 0), 26, 20+i*20, ALLEGRO_ALIGN_LEFT, this->vctRadioButtons[i].c_str());
   }

   al_set_target_bitmap(al_get_backbuffer(display));
   al_draw_bitmap(bitmap, x, y, 0);
}

bool RadioButtonList::handleEvent(ALLEGRO_EVENT& e)
{
   int centerX, centerY;
   centerX = x+12;

   if (e.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
      if (e.mouse.button == 1) {
         for (unsigned int i=0; i<this->vctRadioButtons.size(); i++) {
            centerY = y+26+i*20;

            if (sqrt(pow((float)(e.mouse.x-centerX), 2.0f)+pow((float)(e.mouse.y-centerY), 2.0f))< 8) {
               this->selectedButton = i;
               return true;
            }
         }
      }
   }

   return false;
}

void RadioButtonList::addRadioButton(string s) {
   this->vctRadioButtons.push_back(s);
}

int RadioButtonList::getSelectedButton() {
   return this->selectedButton;
}

void RadioButtonList::setSelectedButton(int b) {
   this->selectedButton = b;
}
