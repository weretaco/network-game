#include "../../common/Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
   #include <WS2tcpip.h>
#elif defined LINUX
   #include <sys/types.h>
   #include <unistd.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <netdb.h>
   #include <cstring>
#endif

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <sstream>
#include <map>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

#include "../../common/Common.h"
#include "../../common/MessageContainer.h"
#include "../../common/MessageProcessor.h"
#include "../../common/WorldMap.h"
#include "../../common/Player.h"
#include "../../common/Projectile.h"

#include "Window.h"
#include "Textbox.h"
#include "Button.h"
#include "RadioButtonList.h"
#include "TextLabel.h"
#include "chat.h"

#ifdef WINDOWS
   #pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

void initWinSock();
void shutdownWinSock();
void processMessage(NETWORK_MSG &msg, int &state, chat &chatConsole, WorldMap *gameMap, map<unsigned int, Player>& mapPlayers, map<unsigned int, Projectile>& mapProjectiles, unsigned int& curPlayerId, int &scoreBlue, int &scoreRed);
void drawMap(WorldMap* gameMap);
void drawPlayers(map<unsigned int, Player>& mapPlayers, ALLEGRO_FONT* font, unsigned int curPlayerId);
POSITION screenToMap(POSITION pos);
POSITION mapToScreen(POSITION pos);
int getRefreshRate(int width, int height);

// callbacks
void goToLoginScreen();
void goToRegisterScreen();
void registerAccount();
void login();
void logout();
void quit();
void sendChatMessage();
void toggleDebugging();
void drawMessageStatus(ALLEGRO_FONT* font);

void error(const char *);

const float FPS = 60;
const int SCREEN_W = 1024;
const int SCREEN_H = 768;

enum STATE {
   STATE_START,
   STATE_LOGIN // this means you're already logged in
};

int state;

bool doexit;

Window* wndLogin;
Window* wndRegister;
Window* wndMain;
Window* wndMainDebug;
Window* wndCurrent;

// wndLogin
Textbox* txtUsername;
Textbox* txtPassword;
TextLabel* lblLoginStatus;

// wndRegister
Textbox* txtUsernameRegister;
Textbox* txtPasswordRegister;
RadioButtonList* rblClasses;
TextLabel* lblRegisterStatus;

// wndMain
Textbox* txtChat;

int sock;
struct sockaddr_in server, from;
struct hostent *hp;
NETWORK_MSG msgTo, msgFrom;
string username;
chat chatConsole, debugConsole;
bool debugging;

MessageProcessor msgProcessor;

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display = NULL;
   ALLEGRO_EVENT_QUEUE *event_queue = NULL;
   ALLEGRO_TIMER *timer = NULL;
   bool key[4] = { false, false, false, false };
   bool redraw = true;
   doexit = false;
   map<unsigned int, Player> mapPlayers;
   map<unsigned int, Projectile> mapProjectiles;
   unsigned int curPlayerId = -1;
   int scoreBlue, scoreRed;
   bool fullscreen = false;
   debugging = false;

   scoreBlue = 0;
   scoreRed = 0;

   state = STATE_START;

   if(!al_init()) {
      fprintf(stderr, "failed to initialize allegro!\n");
      return -1;
   }

   if (al_init_primitives_addon())
      cout << "Primitives initialized" << endl;
   else
      cout << "Primitives not initialized" << endl;

   al_init_font_addon();
   al_init_ttf_addon();

   #if defined WINDOWS
      ALLEGRO_FONT *font = al_load_ttf_font("../pirulen.ttf", 12, 0);
   #elif defined LINUX
      ALLEGRO_FONT *font = al_load_ttf_font("pirulen.ttf", 12, 0);
   #endif

   if (!font) {
      fprintf(stderr, "Could not load 'pirulen.ttf'.\n");
      getchar();
	  return -1;
   }
 
   if(!al_install_keyboard()) {
      fprintf(stderr, "failed to initialize the keyboard!\n");
      return -1;
   }

    if(!al_install_mouse()) {
      fprintf(stderr, "failed to initialize the mouse!\n");
      return -1;
   }
 
   timer = al_create_timer(1.0 / FPS);
   if(!timer) {
      fprintf(stderr, "failed to create timer!\n");
      return -1;
   }
 
   int refreshRate = getRefreshRate(SCREEN_W, SCREEN_H);
   // if the computer doesn't support this resolution, just use windowed mode
   if (refreshRate > 0 && fullscreen) {
      al_set_new_display_flags(ALLEGRO_FULLSCREEN);
      al_set_new_display_refresh_rate(refreshRate);
   }
   display = al_create_display(SCREEN_W, SCREEN_H);
   if(!display) {
      fprintf(stderr, "failed to create display!\n");
      al_destroy_timer(timer);
      return -1;
   }

   WorldMap* gameMap = WorldMap::loadMapFromFile("../../data/map.txt");

   cout << "Loaded map" << endl;

   debugConsole.addLine("Debug console:");
   debugConsole.addLine("");

   wndLogin = new Window(0, 0, SCREEN_W, SCREEN_H);
   wndLogin->addComponent(new Textbox(516, 40, 100, 20, font));
   wndLogin->addComponent(new Textbox(516, 70, 100, 20, font));
   wndLogin->addComponent(new TextLabel(410, 40, 100, 20, font, "Username:", ALLEGRO_ALIGN_RIGHT));
   wndLogin->addComponent(new TextLabel(410, 70, 100, 20, font, "Password:", ALLEGRO_ALIGN_RIGHT));
   wndLogin->addComponent(new TextLabel((SCREEN_W-600)/2, 100, 600, 20, font, "", ALLEGRO_ALIGN_CENTRE));
   wndLogin->addComponent(new Button(SCREEN_W/2-100, 130, 90, 20, font, "Register", goToRegisterScreen));
   wndLogin->addComponent(new Button(SCREEN_W/2+10, 130, 90, 20, font, "Login", login));
   wndLogin->addComponent(new Button(920, 10, 80, 20, font, "Quit", quit));
   wndLogin->addComponent(new Button(20, 10, 160, 20, font, "Toggle Debugging", toggleDebugging));

   txtUsername = (Textbox*)wndLogin->getComponent(0);
   txtPassword = (Textbox*)wndLogin->getComponent(1);
   lblLoginStatus = (TextLabel*)wndLogin->getComponent(4);

   cout << "Created login screen" << endl;

   wndRegister = new Window(0, 0, SCREEN_W, SCREEN_H);
   wndRegister->addComponent(new Textbox(516, 40, 100, 20, font));
   wndRegister->addComponent(new Textbox(516, 70, 100, 20, font));
   wndRegister->addComponent(new TextLabel(410, 40, 100, 20, font, "Username:", ALLEGRO_ALIGN_RIGHT));
   wndRegister->addComponent(new TextLabel(410, 70, 100, 20, font, "Password:", ALLEGRO_ALIGN_RIGHT));
   wndRegister->addComponent(new RadioButtonList(432, 100, "Pick a class", font));
   wndRegister->addComponent(new TextLabel((SCREEN_W-600)/2, 190, 600, 20, font, "", ALLEGRO_ALIGN_CENTRE));
   wndRegister->addComponent(new Button(SCREEN_W/2-100, 220, 90, 20, font, "Back", goToLoginScreen));
   wndRegister->addComponent(new Button(SCREEN_W/2+10, 220, 90, 20, font, "Submit", registerAccount));
   wndRegister->addComponent(new Button(920, 10, 80, 20, font, "Quit", quit));
   wndRegister->addComponent(new Button(20, 10, 160, 20, font, "Toggle Debugging", toggleDebugging));

   txtUsernameRegister = (Textbox*)wndRegister->getComponent(0);
   txtPasswordRegister = (Textbox*)wndRegister->getComponent(1);

   rblClasses = (RadioButtonList*)wndRegister->getComponent(4);
   rblClasses->addRadioButton("Warrior");
   rblClasses->addRadioButton("Ranger");

   lblRegisterStatus = (TextLabel*)wndRegister->getComponent(5);

   cout << "Created register screen" << endl;

   wndMain = new Window(0, 0, SCREEN_W, SCREEN_H);
   wndMain->addComponent(new Textbox(95, 40, 300, 20, font));
   wndMain->addComponent(new Button(95, 70, 60, 20, font, "Send", sendChatMessage));
   wndMain->addComponent(new Button(20, 10, 160, 20, font, "Toggle Debugging", toggleDebugging));
   wndMain->addComponent(new Button(920, 10, 80, 20, font, "Logout", logout));

   txtChat = (Textbox*)wndMain->getComponent(0);

   wndMainDebug = new Window(0, 0, SCREEN_W, SCREEN_H);
   wndMainDebug->addComponent(new Button(20, 10, 160, 20, font, "Toggle Debugging", toggleDebugging));
   wndMainDebug->addComponent(new Button(920, 10, 80, 20, font, "Logout", logout));

   cout << "Created main screen" << endl;

   goToLoginScreen();
 
   event_queue = al_create_event_queue();
   if(!event_queue) {
      fprintf(stderr, "failed to create event_queue!\n");
      al_destroy_display(display);
      al_destroy_timer(timer);
      return -1;
   }
 
   al_set_target_bitmap(al_get_backbuffer(display));
 
   al_register_event_source(event_queue, al_get_display_event_source(display));
   al_register_event_source(event_queue, al_get_timer_event_source(timer));
   al_register_event_source(event_queue, al_get_keyboard_event_source());
   al_register_event_source(event_queue, al_get_mouse_event_source());
 
   al_clear_to_color(al_map_rgb(0,0,0));
 
   al_flip_display();

   if (argc != 3) {
      cout << "Usage: server port" << endl;
      exit(1);
   }

   initWinSock();
	
   sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0)
      error("socket");

   set_nonblock(sock);

   server.sin_family = AF_INET;
   hp = gethostbyname(argv[1]);
   if (hp==0)
      error("Unknown host");

   memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);
   server.sin_port = htons(atoi(argv[2]));

   al_start_timer(timer);

   while(!doexit)
   {
      ALLEGRO_EVENT ev;
      
      al_wait_for_event(event_queue, &ev);

      if(wndCurrent->handleEvent(ev)) {
         // do nothing
      }
      else if(ev.type == ALLEGRO_EVENT_TIMER) {
         redraw = true; // seems like we should just call a draw function here instead
      }
      else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         doexit = true;
      }
      else if(ev.type == ALLEGRO_EVENT_KEY_DOWN) {
      }
      else if(ev.type == ALLEGRO_EVENT_KEY_UP) {
         switch(ev.keyboard.keycode) {
            case ALLEGRO_KEY_ESCAPE:
               doexit = true;
               break;
            case ALLEGRO_KEY_S:  // pickup an item next to you
               if (state == STATE_LOGIN) {
                  msgTo.type = MSG_TYPE_PICKUP_FLAG;
                  memcpy(msgTo.buffer, &curPlayerId, 4);
                  msgProcessor.sendMessage(&msgTo, sock, &server);
               }
               break;
            case ALLEGRO_KEY_D:  // drop the current item
               if (state == STATE_LOGIN) {
                  // find the current player in the player list
                  map<unsigned int, Player>::iterator it;
                  Player* p = NULL;
                  for(it = mapPlayers.begin(); it != mapPlayers.end(); it++)
                  {
                     if (it->second.id == curPlayerId)
                        p = &it->second;
                  }

                  if (p != NULL) {
                     int flagType = WorldMap::OBJECT_NONE;

                     if (p->hasBlueFlag)
                        flagType = WorldMap::OBJECT_BLUE_FLAG;
                     else if (p->hasRedFlag)
                        flagType = WorldMap::OBJECT_RED_FLAG;

                     if (flagType != WorldMap::OBJECT_NONE) {
                        msgTo.type = MSG_TYPE_DROP_FLAG;
                        memcpy(msgTo.buffer, &curPlayerId, 4);
                        msgProcessor.sendMessage(&msgTo, sock, &server);
                     }
                  }
               }
               break;
         }
      }
      else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
         if(wndCurrent == wndMain) {
            if (ev.mouse.button == 1) {   // left click
               msgTo.type = MSG_TYPE_PLAYER_MOVE;

               POSITION pos;
               pos.x = ev.mouse.x;
               pos.y = ev.mouse.y;
               pos = screenToMap(pos);

               if (pos.x != -1)
               {
                  memcpy(msgTo.buffer, &curPlayerId, 4);
                  memcpy(msgTo.buffer+4, &pos.x, 4);
                  memcpy(msgTo.buffer+8, &pos.y, 4);

                  msgProcessor.sendMessage(&msgTo, sock, &server);
               }
               else
                  cout << "Invalid point: User did not click on the map" << endl;
            }else if (ev.mouse.button == 2) {   // right click
                  map<unsigned int, Player>::iterator it;

                  cout << "Detected a right-click" << endl;

                  Player* curPlayer;
                  for(it = mapPlayers.begin(); it != mapPlayers.end(); it++)
                  {
                     if (it->second.id == curPlayerId)
                        curPlayer = &it->second;
                  }

                  Player* target;
                  for(it = mapPlayers.begin(); it != mapPlayers.end(); it++)
                  {
                     // need to check if the right-click was actually on this player
                     // right now, this code will target all players other than the current one
                     target = &it->second;
                     if (target->id != curPlayerId && target->team != curPlayer->team) {
                        msgTo.type = MSG_TYPE_START_ATTACK;
                        memcpy(msgTo.buffer, &curPlayerId, 4);
                        memcpy(msgTo.buffer+4, &target->id, 4);

                        msgProcessor.sendMessage(&msgTo, sock, &server);
                     }
                  }
            }
         }
      }

      if (msgProcessor.receiveMessage(&msgFrom, sock, &from) >= 0)
         processMessage(msgFrom, state, chatConsole, gameMap, mapPlayers, mapProjectiles, curPlayerId, scoreBlue, scoreRed);

      if (redraw)
      {
         redraw = false;

         msgProcessor.resendUnackedMessages(sock);
         //msgProcessor.cleanAckedMessages();

         if (debugging && wndCurrent == wndMain)
            wndMainDebug->draw(display);
         else
            wndCurrent->draw(display);

         if(wndCurrent == wndMain) {
            if (!debugging)
               chatConsole.draw(font, al_map_rgb(255,255,255));

            al_draw_text(font, al_map_rgb(0, 255, 0), 4, 43, ALLEGRO_ALIGN_LEFT, "Message:");

            ostringstream ossScoreBlue, ossScoreRed;

            ossScoreBlue << "Blue: " << scoreBlue << endl;
            ossScoreRed << "Red: " << scoreRed << endl;

            al_draw_text(font, al_map_rgb(0, 255, 0), 330, 80, ALLEGRO_ALIGN_LEFT, ossScoreBlue.str().c_str());
            al_draw_text(font, al_map_rgb(0, 255, 0), 515, 80, ALLEGRO_ALIGN_LEFT, ossScoreRed.str().c_str());

            // update players
            map<unsigned int, Player>::iterator it;
            for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
            {
               it->second.updateTarget(mapPlayers);
            }

            for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
            {
               it->second.move(gameMap);   // ignore return value
            }

            // update projectile positions
            map<unsigned int, Projectile>::iterator it2;
            for (it2 = mapProjectiles.begin(); it2 != mapProjectiles.end(); it2++)
            {
               it2->second.move(mapPlayers);
            }

            drawMap(gameMap);
            drawPlayers(mapPlayers, font, curPlayerId);

            // draw projectiles
            for (it2 = mapProjectiles.begin(); it2 != mapProjectiles.end(); it2++)
            {
               Projectile proj = it2->second;

               FLOAT_POSITION target = mapPlayers[proj.target].pos;
               float angle =  atan2(target.y-proj.pos.toFloat().y, target.x-proj.pos.toFloat().x);

               POSITION start, end;
               start.x = cos(angle)*15+proj.pos.x;
               start.y = sin(angle)*15+proj.pos.y;
               end.x = proj.pos.x;
               end.y = proj.pos.y;

               start = mapToScreen(start);
               end = mapToScreen(end);

               al_draw_line(start.x, start.y, end.x, end.y, al_map_rgb(0, 0, 0), 4);
            }
         }

         if (debugging) {
            //debugConsole.draw(font, al_map_rgb(255,255,255));
            drawMessageStatus(font);
         }

         al_flip_display();
      }
   }

   #if defined WINDOWS
      closesocket(sock);
   #elif defined LINUX
      close(sock);
   #endif

   shutdownWinSock();
   
   delete wndLogin;
   delete wndMain;

   delete gameMap;

   al_destroy_event_queue(event_queue);
   al_destroy_display(display);
   al_destroy_timer(timer);
 
   return 0;
}



// need to make a function like this that works on windows
void error(const char *msg)
{
   perror(msg);
   shutdownWinSock();
   exit(1);
}

void initWinSock()
{
#if defined WINDOWS
   WORD wVersionRequested;
   WSADATA wsaData;
   int wsaerr;

   wVersionRequested = MAKEWORD(2, 2);
   wsaerr = WSAStartup(wVersionRequested, &wsaData);
	
   if (wsaerr != 0) {
      cout << "The Winsock dll not found." << endl;
      exit(1);
   }else
      cout << "The Winsock dll was found." << endl;
#endif
}

void shutdownWinSock()
{
#if defined WINDOWS
   WSACleanup();
#endif
}

POSITION screenToMap(POSITION pos)
{
   pos.x = pos.x-300;
   pos.y = pos.y-100;

   if (pos.x < 0 || pos.y < 0)
   {
      pos.x = -1;
      pos.y = -1;
   }

   return pos;
}

POSITION mapToScreen(POSITION pos)
{
   pos.x = pos.x+300;
   pos.y = pos.y+100;

   return pos;
}

POSITION mapToScreen(FLOAT_POSITION pos)
{
   POSITION p;
   p.x = pos.x+300;
   p.y = pos.y+100;

   return p;
}

void processMessage(NETWORK_MSG &msg, int &state, chat &chatConsole, WorldMap *gameMap, map<unsigned int, Player>& mapPlayers, map<unsigned int, Projectile>& mapProjectiles, unsigned int& curPlayerId, int &scoreBlue, int &scoreRed)
{
   string response = string(msg.buffer);

   switch(state)
   {
      case STATE_START:
      {
         cout << "In STATE_START" << endl;

         switch(msg.type)
         {
            case MSG_TYPE_REGISTER:
            {
               lblRegisterStatus->setText(response);
               break;
            }
            default:
            {
               cout << "(STATE_REGISTER) Received invalid message of type " << msg.type << endl;
               break;
            }
         }

         break;
      }
      case STATE_LOGIN:
      {
         switch(msg.type)
         {
            case MSG_TYPE_LOGIN:
            {
               if (response.compare("Player has already logged in.") == 0)
               {
                  goToLoginScreen();
                  state = STATE_START;

                  lblLoginStatus->setText(response);
               }
               else if (response.compare("Incorrect username or password") == 0)
               {
                  goToLoginScreen();
                  state = STATE_START;

                  lblLoginStatus->setText(response);
               }
               else
               {
                  wndCurrent = wndMain;
                  
                  Player p("", "");
                  p.deserialize(msg.buffer);
                  mapPlayers[p.id] = p;
                  curPlayerId = p.id;

                  cout << "Got a valid login response with the player" << endl;
                  cout << "Player id: " << curPlayerId << endl;
                  cout << "Player health: " << p.health << endl;
                  cout << "player map size: " << mapPlayers.size() << endl;
               }

               break;
            }
            case MSG_TYPE_LOGOUT:
            {
               cout << "Got a logout message" << endl;

               if (response.compare("You have successfully logged out.") == 0)
               {
                  cout << "Logged out" << endl;
                  state = STATE_START;
                  goToLoginScreen();
               }

               break;
            }
            case MSG_TYPE_PLAYER:
            {
               cout << "Received MSG_TYPE_PLAYER" << endl;

               Player p("", "");
               p.deserialize(msg.buffer);
               p.timeLastUpdated = getCurrentMillis();
               p.isChasing = false;
               if (p.health <= 0)
                  p.isDead = true;
               else
                  p.isDead = false;

               mapPlayers[p.id] = p;

               break;
            }
            case MSG_TYPE_PLAYER_MOVE:
            {
               unsigned int id;
               int x, y;

               memcpy(&id, msg.buffer, 4);
               memcpy(&x, msg.buffer+4, 4);
               memcpy(&y, msg.buffer+8, 4);

               mapPlayers[id].target.x = x;
               mapPlayers[id].target.y = y;

               break;
            }
            case MSG_TYPE_CHAT:
            {
               chatConsole.addLine(response);

               break;
            }
            case MSG_TYPE_OBJECT:
            {
               cout << "Received object message in STATE_LOGIN." << endl;

               WorldMap::Object o(0, WorldMap::OBJECT_NONE, 0, 0);
               o.deserialize(msg.buffer);
               cout << "object id: " << o.id << endl;
               gameMap->updateObject(o.id, o.type, o.pos.x, o.pos.y);

               break;
            }
            case MSG_TYPE_REMOVE_OBJECT:
            {
               cout << "Received REMOVE_OBJECT message!" << endl;

               int id;
               memcpy(&id, msg.buffer, 4);

               cout << "Removing object with id " << id << endl;

               if (!gameMap->removeObject(id))
                  cout << "Did not remove the object" << endl;

               break;
            }
            case MSG_TYPE_SCORE:
            {
               memcpy(&scoreBlue, msg.buffer, 4);
               memcpy(&scoreRed, msg.buffer+4, 4);
 
               break;
            }
            case MSG_TYPE_ATTACK:
            {
               cout << "Received ATTACK message" << endl;

               break;
            }
            case MSG_TYPE_START_ATTACK:
            {
               cout << "Received START_ATTACK message" << endl;

               unsigned int id, targetID;
               memcpy(&id, msg.buffer, 4);
               memcpy(&targetID, msg.buffer+4, 4);

               cout << "source id: " << id << endl;
               cout << "target id: " << targetID << endl;

               Player* source = &mapPlayers[id];
               source->targetPlayer = targetID;
               source->isChasing = true;

               break;
            }
            case MSG_TYPE_PROJECTILE:
            {
               cout << "Received a PROJECTILE message" << endl;

               int id, x, y, targetId;

               memcpy(&id, msg.buffer, 4);
               memcpy(&x, msg.buffer+4, 4);
               memcpy(&y, msg.buffer+8, 4);
               memcpy(&targetId, msg.buffer+12, 4);

               cout << "id: " << id << endl;
               cout << "x: " << x << endl;
               cout << "y: " << y << endl;
               cout << "Target: " << targetId << endl;

               Projectile proj(x, y, targetId, 0);
               proj.setId(id);

               mapProjectiles[id] = proj;

               break;
            }
            case MSG_TYPE_REMOVE_PROJECTILE:
            {
                cout << "Received a REMOVE_PROJECTILE message" << endl;

               int id;

               memcpy(&id, msg.buffer, 4);
               
               mapProjectiles.erase(id);

               break;
            }
            default:
            {
               cout << "(STATE_LOGIN) Received invlaid message of type " << msg.type << endl;
               break;
            }
         }

         break;
      }
      default:
      {
         cout << "The state has an invalid value: " << state << endl;

         break;
      }
   }
}

// this should probably be in the WorldMap class
void drawMap(WorldMap* gameMap)
{
   POSITION mapPos;
   mapPos.x = 0;
   mapPos.y = 0;
   mapPos = mapToScreen(mapPos);

   for (int x=0; x<gameMap->width; x++)
   {
      for (int y=0; y<gameMap->height; y++)
      {
         WorldMap::TerrainType el = gameMap->getElement(x, y);
         WorldMap::StructureType structure = gameMap->getStructure(x, y);

         if (el == WorldMap::TERRAIN_GRASS)
            al_draw_filled_rectangle(x*25+mapPos.x, y*25+mapPos.y, x*25+25+mapPos.x, y*25+25+mapPos.y, al_map_rgb(0, 255, 0));
         else if (el == WorldMap::TERRAIN_OCEAN)
            al_draw_filled_rectangle(x*25+mapPos.x, y*25+mapPos.y, x*25+25+mapPos.x, y*25+25+mapPos.y, al_map_rgb(0, 0, 255));
         else if (el == WorldMap::TERRAIN_ROCK)
            al_draw_filled_rectangle(x*25+mapPos.x, y*25+mapPos.y, x*25+25+mapPos.x, y*25+25+mapPos.y, al_map_rgb(100, 100, 0));

         if (structure == WorldMap::STRUCTURE_BLUE_FLAG) {
            al_draw_circle(x*25+12+mapPos.x, y*25+12+mapPos.y, 12, al_map_rgb(0, 0, 0), 3);
            //al_draw_filled_rectangle(x*25+5+mapPos.x, y*25+5+mapPos.y, x*25+20+mapPos.x, y*25+20+mapPos.y, al_map_rgb(0, 0, 255));
         }else if (structure == WorldMap::STRUCTURE_RED_FLAG) {
            al_draw_circle(x*25+12+mapPos.x, y*25+12+mapPos.y, 12, al_map_rgb(0, 0, 0), 3);
            //al_draw_filled_rectangle(x*25+5+mapPos.x, y*25+5+mapPos.y, x*25+20+mapPos.x, y*25+20+mapPos.y, al_map_rgb(255, 0, 0));
         }
      }
   }

   for (int x=0; x<gameMap->width; x++)
   {
      for (int y=0; y<gameMap->height; y++)
      {
         vector<WorldMap::Object> vctObjects = gameMap->getObjects(x, y);

         vector<WorldMap::Object>::iterator it;
         for(it = vctObjects.begin(); it != vctObjects.end(); it++) {
            switch(it->type) {
               case WorldMap::OBJECT_BLUE_FLAG:
                  al_draw_filled_rectangle(it->pos.x-8+mapPos.x, it->pos.y-8+mapPos.y, it->pos.x+8+mapPos.x, it->pos.y+8+mapPos.y, al_map_rgb(0, 0, 255));
                  break;
               case WorldMap::OBJECT_RED_FLAG:
                  al_draw_filled_rectangle(it->pos.x-8+mapPos.x, it->pos.y-8+mapPos.y, it->pos.x+8+mapPos.x, it->pos.y+8+mapPos.y, al_map_rgb(255, 0, 0));
                  break;
            }
         }
      }
   }
}

void drawPlayers(map<unsigned int, Player>& mapPlayers, ALLEGRO_FONT* font, unsigned int curPlayerId)
{
   map<unsigned int, Player>::iterator it;

   Player* p;
   POSITION pos;
   ALLEGRO_COLOR color;

   for(it = mapPlayers.begin(); it != mapPlayers.end(); it++)
   {
      p = &it->second;

      if (p->isDead)
         continue;

      pos = mapToScreen(p->pos);

      if (p->id == curPlayerId)
         al_draw_filled_circle(pos.x, pos.y, 14, al_map_rgb(0, 0, 0));
      
      if (p->team == 0)
         color = al_map_rgb(0, 0, 255);
      else if (p->team == 1)
         color = al_map_rgb(255, 0, 0);
      
      al_draw_filled_circle(pos.x, pos.y, 12, color);

      // draw player class
      int fontHeight = al_get_font_line_height(font);

      string strClass;
      switch (p->playerClass) {
      case Player::CLASS_WARRIOR:
         strClass = "W";
         break;
      case Player::CLASS_RANGER:
         strClass = "R";
         break;
      case Player::CLASS_NONE:
         strClass = "";
         break;
      default:
         strClass = "";
         break;
      }
      al_draw_text(font, al_map_rgb(0, 0, 0), pos.x, pos.y-fontHeight/2, ALLEGRO_ALIGN_CENTRE, strClass.c_str());

      // draw player health
      al_draw_filled_rectangle(pos.x-12, pos.y-24, pos.x+12, pos.y-16, al_map_rgb(0, 0, 0));
      if (it->second.maxHealth != 0)
         al_draw_filled_rectangle(pos.x-11, pos.y-23, pos.x-11+(22*it->second.health)/it->second.maxHealth, pos.y-17, al_map_rgb(255, 0, 0));

      if (p->hasBlueFlag)
         al_draw_filled_rectangle(pos.x+4, pos.y-18, pos.x+18, pos.y-4, al_map_rgb(0, 0, 255));
      else if (p->hasRedFlag)
         al_draw_filled_rectangle(pos.x+4, pos.y-18, pos.x+18, pos.y-4, al_map_rgb(255, 0, 0));
   }
}

void goToRegisterScreen()
{
   txtUsernameRegister->clear();
   txtPasswordRegister->clear();
   lblRegisterStatus->setText("");
   rblClasses->setSelectedButton(-1);

   wndCurrent = wndRegister;
}

void goToLoginScreen()
{
   txtUsername->clear();
   txtPassword->clear();
   lblLoginStatus->setText("");

   wndCurrent = wndLogin;
}

// maybe need a goToGameScreen function as well and add state changes to these functions as well

void registerAccount()
{
   string username = txtUsernameRegister->getStr();
   string password = txtPasswordRegister->getStr();

   txtUsernameRegister->clear();
   txtPasswordRegister->clear();
   // maybe clear rblClasses as well (add a method to RadioButtonList to enable this)

   Player::PlayerClass playerClass;

   switch (rblClasses->getSelectedButton()) {
   case 0:
      playerClass = Player::CLASS_WARRIOR;
      break;
   case 1:
      playerClass = Player::CLASS_RANGER;
      break;
   default:
      cout << "Invalid class selection" << endl;
      playerClass = Player::CLASS_NONE;
      break;
   }

   msgTo.type = MSG_TYPE_REGISTER;

   strcpy(msgTo.buffer, username.c_str());
   strcpy(msgTo.buffer+username.size()+1, password.c_str());
   memcpy(msgTo.buffer+username.size()+password.size()+2, &playerClass, 4);

   msgProcessor.sendMessage(&msgTo, sock, &server);
}

void login()
{
   string strUsername = txtUsername->getStr();
   string strPassword = txtPassword->getStr();
   username = strUsername;

   txtUsername->clear();
   txtPassword->clear();

   msgTo.type = MSG_TYPE_LOGIN;

   strcpy(msgTo.buffer, strUsername.c_str());
   strcpy(msgTo.buffer+username.size()+1, strPassword.c_str());

   msgProcessor.sendMessage(&msgTo, sock, &server);

   state = STATE_LOGIN;
}

void logout()
{
   txtChat->clear();
   chatConsole.clear();

   msgTo.type = MSG_TYPE_LOGOUT;

   strcpy(msgTo.buffer, username.c_str());

   msgProcessor.sendMessage(&msgTo, sock, &server);
}

void quit()
{
   doexit = true;
}

void sendChatMessage()
{
   string msg = txtChat->getStr();

   txtChat->clear();

   msgTo.type = MSG_TYPE_CHAT;

   strcpy(msgTo.buffer, msg.c_str());

   msgProcessor.sendMessage(&msgTo, sock, &server);
}

void toggleDebugging()
{
   debugging = !debugging;
}

int getRefreshRate(int width, int height)
{
   int numRefreshRates = al_get_num_display_modes();
   ALLEGRO_DISPLAY_MODE displayMode;

   for(int i=0; i<numRefreshRates; i++) {
      al_get_display_mode(i, &displayMode);

      if (displayMode.width == width && displayMode.height == height)
         return displayMode.refresh_rate;
   }

   return 0;
}

void drawMessageStatus(ALLEGRO_FONT* font)
{
   int clientMsgOffset = 0;
   int serverMsgOffset = 650;

   al_draw_text(font, al_map_rgb(0, 255, 255), 5, 43, ALLEGRO_ALIGN_LEFT, "ID");
   al_draw_text(font, al_map_rgb(0, 255, 255), 25, 43, ALLEGRO_ALIGN_LEFT, "Type");
   al_draw_text(font, al_map_rgb(0, 255, 255), 245, 43, ALLEGRO_ALIGN_LEFT, "Acked?");

   al_draw_text(font, al_map_rgb(0, 255, 255), 5+serverMsgOffset, 43, ALLEGRO_ALIGN_LEFT, "ID");
   al_draw_text(font, al_map_rgb(0, 255, 255), 25+serverMsgOffset, 43, ALLEGRO_ALIGN_LEFT, "Type");

   map<unsigned int, map<unsigned long, MessageContainer> >& sentMessages = msgProcessor.getSentMessages();
   int id, type;
   bool acked;
   ostringstream ossId, ossAcked;

   map<unsigned int, map<unsigned long, MessageContainer> >::iterator it;

   int msgCount = 0;
   for (it = sentMessages.begin(); it != sentMessages.end(); it++) {
      map<unsigned long, MessageContainer> playerMessage = it->second;
      map<unsigned long, MessageContainer>::iterator it2;
      for (it2 = playerMessage.begin(); it2 !=  playerMessage.end(); it2++) {

         id = it->first;
         ossId.str("");;
         ossId << id;

         type = it2->second.getMessage()->type;
         string typeStr = MessageContainer::getMsgTypeString(type);

         acked = it2->second.getAcked();
         ossAcked.str("");;
         ossAcked << boolalpha << acked;

         al_draw_text(font, al_map_rgb(0, 255, 0), 5, 60+15*msgCount, ALLEGRO_ALIGN_LEFT, ossId.str().c_str());
         al_draw_text(font, al_map_rgb(0, 255, 0), 25, 60+15*msgCount, ALLEGRO_ALIGN_LEFT, typeStr.c_str());
         al_draw_text(font, al_map_rgb(0, 255, 0), 245, 60+15*msgCount, ALLEGRO_ALIGN_LEFT, ossAcked.str().c_str());

         msgCount++;
      }
   }

   map<unsigned int, MessageContainer>& ackedMessages = msgProcessor.getAckedMessages();
   map<unsigned int, MessageContainer>::iterator it3;

   msgCount = 0;
   for (it3 = ackedMessages.begin(); it3 != ackedMessages.end(); it3++) {
      ossId.str("");;
      ossId << it3->first;

      string typeStr = MessageContainer::getMsgTypeString(it3->second.getMessage()->type);

      al_draw_text(font, al_map_rgb(255, 0, 0), 5+serverMsgOffset, 60+15*msgCount, ALLEGRO_ALIGN_LEFT, ossId.str().c_str());
      al_draw_text(font, al_map_rgb(255, 0, 0), 25+serverMsgOffset, 60+15*msgCount, ALLEGRO_ALIGN_LEFT, typeStr.c_str());

      msgCount++;
   }
}