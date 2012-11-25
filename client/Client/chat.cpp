#include "chat.h"

chat::chat(void)
{
}

chat::~chat(void)
{
}

string chat::getInput()
{
   string temp = strEnteredInput;
   strEnteredInput.clear();
   return temp;
}

void chat::draw(ALLEGRO_FONT *font, ALLEGRO_COLOR color)
{
   for(int x=0; x<vctChat.size(); x++)
      al_draw_text(font, color, 10, 10+x*15, ALLEGRO_ALIGN_LEFT, vctChat[x].c_str());

   al_draw_text(font, color, 10, 460, ALLEGRO_ALIGN_LEFT, strPrompt.c_str());
}

void chat::addLine(string s)
{
   vctChat.push_back(s);
}

// returns true if the event was consumed, false if it should be passed on
bool chat::processEvent(ALLEGRO_EVENT ev)
{
   ALLEGRO_KEYBOARD_STATE keys;
   al_get_keyboard_state(&keys);

   if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
      char newChar = 0;

      if (ALLEGRO_KEY_A <= ev.keyboard.keycode && ev.keyboard.keycode <= ALLEGRO_KEY_Z) {
         newChar = 'a'+ev.keyboard.keycode-ALLEGRO_KEY_A;
         if (al_key_down(&keys, ALLEGRO_KEY_LSHIFT) || al_key_down(&keys, ALLEGRO_KEY_RSHIFT))
            newChar -= 32;
      }
      if (ALLEGRO_KEY_0 <= ev.keyboard.keycode && ev.keyboard.keycode <= ALLEGRO_KEY_9)
         newChar = '0'+ev.keyboard.keycode-ALLEGRO_KEY_0;

      if (newChar != 0) {
         strPrompt.append(1, newChar);
         return true;
      }

      if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER) {
         strEnteredInput = strPrompt;
         strPrompt.clear();
         return true;
      }
   }
   
   return false;
}
