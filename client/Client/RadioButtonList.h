#ifndef _RADIOBUTTONLIST_H
#define _RADIOBUTTONLIST_H

#include "GuiComponent.h"

#include <string>
#include <vector>

using namespace std;

class RadioButtonList :
   public GuiComponent
{
private:
   string strLabel;
   vector<string> vctRadioButtons;
   int selectedButton;

public:
   RadioButtonList(int x, int y, string strLabel, ALLEGRO_FONT *font);
   ~RadioButtonList(void);

   void draw(ALLEGRO_DISPLAY *display);
   bool handleEvent(ALLEGRO_EVENT& e);

   void addRadioButton(string s);
   int getSelectedButton();
   void setSelectedButton(int b);
};

#endif

