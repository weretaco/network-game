#pragma once

#include <string>
#include <vector>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>

using namespace std;

class chat
{
private:
   vector<string> vctChat;
   string strPrompt;
   string strEnteredInput;
public:
   chat(void);
   ~chat(void);

   string getInput();

   void draw(ALLEGRO_FONT *font, ALLEGRO_COLOR color);
   void addLine(string s);

   bool processEvent(ALLEGRO_EVENT ev);
};

