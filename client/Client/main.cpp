#include "../../common/Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
   #include <ws2tcpip.h>
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
#include <fstream>
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
#include "../../common/Game.h"

#include "Window.h"
#include "TextLabel.h"
#include "Button.h"
#include "Textbox.h"
#include "RadioButtonList.h"

#include "GameRender.h"

#include "chat.h"

#ifdef WINDOWS
   #pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

void initWinSock();
void shutdownWinSock();
void processMessage(NETWORK_MSG &msg, int &state, chat &chatConsole, WorldMap *gameMap, map<unsigned int, Player*>& mapPlayers, map<unsigned int, Projectile>& mapProjectiles, unsigned int& curPlayerId, int &scoreBlue, int &scoreRed);
int getRefreshRate(int width, int height);
void drawMessageStatus(ALLEGRO_FONT* font);

// Callback declarations
void goToLoginScreen();
void goToRegisterScreen();
void registerAccount();
void login();
void logout();
void quit();
void sendChatMessage();
void toggleDebugging();
void joinGame();
void createGame();
void leaveGame();

void error(const char *);

const float FPS = 60;
const int SCREEN_W = 1024;
const int SCREEN_H = 768;

enum STATE {
   STATE_START,
   STATE_LOBBY,
   STATE_GAME,
   STATE_NEW_GAME
};

int state;

bool doexit;

Window* wndLogin;
Window* wndRegister;
Window* wndLobby;
Window* wndGame;
Window* wndNewGame;
Window* wndGameDebug;
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

// wndLobby
Textbox* txtJoinGame;
Textbox* txtCreateGame;

// wndGame
Textbox* txtChat;

int sock;
struct sockaddr_in server, from;
struct hostent *hp;
NETWORK_MSG msgTo, msgFrom;
string username;
chat chatConsole, debugConsole;
bool debugging;
map<string, int> mapGames;
Game* game;

MessageProcessor msgProcessor;
ofstream outputLog;

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display = NULL;
   ALLEGRO_EVENT_QUEUE *event_queue = NULL;
   ALLEGRO_TIMER *timer = NULL;
   bool key[4] = { false, false, false, false };
   map<unsigned int, Player*> mapPlayers;
   map<unsigned int, Projectile> mapProjectiles;
   unsigned int curPlayerId = -1;
   int scoreBlue, scoreRed;

   doexit = false;
   debugging = false;
   bool redraw = true;
   bool fullscreen = false;
   game = NULL;

   scoreBlue = 0;
   scoreRed = 0;

   state = STATE_START;

   if(!al_init()) {
      fprintf(stderr, "failed to initialize allegro!\n");
      return -1;
   }

   outputLog.open("client.log", ios::app);
   outputLog << "Started client on " << getCurrentDateTimeString() << endl;

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

   wndLobby = new Window(0, 0, SCREEN_W, SCREEN_H);
   wndLobby->addComponent(new Button(920, 10, 80, 20, font, "Logout", logout));
   wndLobby->addComponent(new TextLabel(SCREEN_W*1/2+15-112, 40, 110, 20, font, "Game Name:", ALLEGRO_ALIGN_RIGHT));
   wndLobby->addComponent(new Textbox(SCREEN_W*1/2+15+4, 40, 100, 20, font));
   wndLobby->addComponent(new Button(SCREEN_W*1/2+15-100, 80, 200, 20, font, "Join Existing Game", joinGame));
   wndLobby->addComponent(new TextLabel(SCREEN_W*3/4-112, 40, 110, 20, font, "Game Name:", ALLEGRO_ALIGN_RIGHT));
   wndLobby->addComponent(new Textbox(SCREEN_W*3/4+4, 40, 100, 20, font));
   wndLobby->addComponent(new Button(SCREEN_W*3/4-100, 80, 200, 20, font, "Create New Game", createGame));
   wndLobby->addComponent(new Textbox(95, 40, 300, 20, font));
   wndLobby->addComponent(new Button(95, 70, 60, 20, font, "Send", sendChatMessage));

   txtJoinGame = (Textbox*)wndLobby->getComponent(2);
   txtCreateGame = (Textbox*)wndLobby->getComponent(5);
   txtChat = (Textbox*)wndLobby->getComponent(7);

   cout << "Created lobby screen" << endl;

   // this is the old game screen
   wndGame = new Window(0, 0, SCREEN_W, SCREEN_H);
   wndGame->addComponent(new Textbox(95, 40, 300, 20, font));
   wndGame->addComponent(new Button(95, 70, 60, 20, font, "Send", sendChatMessage));
   wndGame->addComponent(new Button(20, 10, 160, 20, font, "Toggle Debugging", toggleDebugging));
   wndGame->addComponent(new Button(920, 10, 80, 20, font, "Logout", logout));

   wndGameDebug = new Window(0, 0, SCREEN_W, SCREEN_H);
   wndGameDebug->addComponent(new Button(20, 10, 160, 20, font, "Toggle Debugging", toggleDebugging));
   wndGameDebug->addComponent(new Button(920, 10, 80, 20, font, "Logout", logout));

   cout << "Created game screen" << endl;

   // this is the new game screen, without a debug console
   wndNewGame = new Window(0, 0, SCREEN_W, SCREEN_H);
   wndNewGame->addComponent(new Button(880, 10, 120, 20, font, "Leave Game", leaveGame));

   cout << "Created new game screen" << endl;

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
               if (state == STATE_GAME) {
                  msgTo.type = MSG_TYPE_PICKUP_FLAG;
                  memcpy(msgTo.buffer, &curPlayerId, 4);
                  msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);
               }
               break;
            case ALLEGRO_KEY_D:  // drop the current item
               if (state == STATE_GAME) {
                  // find the current player in the player list
                  map<unsigned int, Player*>::iterator it;
                  Player* p = NULL;
                  for(it = mapPlayers.begin(); it != mapPlayers.end(); it++)
                  {
                     if (it->second->id == curPlayerId)
                        p = it->second;
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
                        msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);
                     }
                  }
               }
               break;
         }
      }
      else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
         if(wndCurrent == wndLobby) {
            if (ev.mouse.button == 1) { // left click
               txtJoinGame->clear();
               txtCreateGame->clear();
               state = STATE_GAME;
               wndCurrent = wndGame;
            }
         }else if(wndCurrent == wndGame) {
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

                  msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);
               }
               else
                  cout << "Invalid point: User did not click on the map" << endl;
            }else if (ev.mouse.button == 2) {   // right click
                  map<unsigned int, Player*>::iterator it;

                  cout << "Detected a right-click" << endl;

                  Player* curPlayer;
                  for(it = mapPlayers.begin(); it != mapPlayers.end(); it++)
                  {
                     if (it->second->id == curPlayerId)
                        curPlayer = it->second;
                  }

                  Player* target;
                  for(it = mapPlayers.begin(); it != mapPlayers.end(); it++)
                  {
                     // need to check if the right-click was actually on this player
                     // right now, this code will target all players other than the current one
                     target = it->second;
                     if (target->id != curPlayerId && target->team != curPlayer->team)
                     {
                        msgTo.type = MSG_TYPE_START_ATTACK;
                        memcpy(msgTo.buffer, &curPlayerId, 4);
                        memcpy(msgTo.buffer+4, &target->id, 4);

                        msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);
                     }
                  }
            }
         }
      }

      if (msgProcessor.receiveMessage(&msgFrom, sock, &from, &outputLog) >= 0)
         processMessage(msgFrom, state, chatConsole, gameMap, mapPlayers, mapProjectiles, curPlayerId, scoreBlue, scoreRed);

      if (redraw)
      {
         redraw = false;

         msgProcessor.resendUnackedMessages(sock, &outputLog);
         //msgProcessor.cleanAckedMessages(&outputLog);

         if (debugging && wndCurrent == wndGame)
            wndGameDebug->draw(display);
         else
            wndCurrent->draw(display);

         if (wndCurrent == wndLobby) {
            chatConsole.draw(font, al_map_rgb(255,255,255));

            map<string, int>::iterator it;
            int i=0;
            ostringstream ossGame;
            for (it = mapGames.begin(); it != mapGames.end(); it++) {
               ossGame << it->first << " (" << it->second << " players)" << endl;
               al_draw_text(font, al_map_rgb(0, 255, 0), SCREEN_W*1/4-100, 120+i*15, ALLEGRO_ALIGN_LEFT, ossGame.str().c_str());
               ossGame.clear();
               ossGame.str("");
               i++;
            }
         }
         else if (wndCurrent == wndNewGame)
         {
            ostringstream ossScoreBlue, ossScoreRed;

            ossScoreBlue << "Blue: " << game->getBlueScore() << endl;
            ossScoreRed << "Red: " << game->getRedScore() << endl;

            al_draw_text(font, al_map_rgb(0, 255, 0), 330, 80, ALLEGRO_ALIGN_LEFT, ossScoreBlue.str().c_str());
            al_draw_text(font, al_map_rgb(0, 255, 0), 515, 80, ALLEGRO_ALIGN_LEFT, ossScoreRed.str().c_str());

            GameRender::drawMap(game->getMap());
            GameRender::drawPlayers(game->getPlayers(), font, curPlayerId);
         }
         else if (wndCurrent == wndGame)
         {
            if (!debugging)
               chatConsole.draw(font, al_map_rgb(255,255,255));

            al_draw_text(font, al_map_rgb(0, 255, 0), 4, 43, ALLEGRO_ALIGN_LEFT, "Message:");

            ostringstream ossScoreBlue, ossScoreRed;

            ossScoreBlue << "Blue: " << scoreBlue << endl;
            ossScoreRed << "Red: " << scoreRed << endl;

            al_draw_text(font, al_map_rgb(0, 255, 0), 330, 80, ALLEGRO_ALIGN_LEFT, ossScoreBlue.str().c_str());
            al_draw_text(font, al_map_rgb(0, 255, 0), 515, 80, ALLEGRO_ALIGN_LEFT, ossScoreRed.str().c_str());

            // update players
            map<unsigned int, Player*>::iterator it;
            for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
            {
               it->second->updateTarget(mapPlayers);
            }

            for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
            {
               it->second->move(gameMap);    // ignore return value
            }

            // update projectile positions
            map<unsigned int, Projectile>::iterator it2;
            for (it2 = mapProjectiles.begin(); it2 != mapProjectiles.end(); it2++)
            {
               it2->second.move(mapPlayers);
            }

            GameRender::drawMap(gameMap);
            GameRender::drawPlayers(mapPlayers, font, curPlayerId);

            // draw projectiles
            for (it2 = mapProjectiles.begin(); it2 != mapProjectiles.end(); it2++)
            {
               Projectile proj = it2->second;

               FLOAT_POSITION target = mapPlayers[proj.target]->pos;
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
   delete wndRegister;
   delete wndLobby;
   delete wndGame;
   delete wndGameDebug;

   delete gameMap;

   if (game != NULL)
      delete game;

   map<unsigned int, Player*>::iterator it;

   for (it = mapPlayers.begin(); it != mapPlayers.end(); it++) {
      delete it->second;
   }

   al_destroy_event_queue(event_queue);
   al_destroy_display(display);
   al_destroy_timer(timer);

   outputLog << "Stopped client on " << getCurrentDateTimeString() << endl;
   outputLog.close();

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

void processMessage(NETWORK_MSG &msg, int &state, chat &chatConsole, WorldMap *gameMap, map<unsigned int, Player*>& mapPlayers, map<unsigned int, Projectile>& mapProjectiles, unsigned int& curPlayerId, int &scoreBlue, int &scoreRed)
{
   // this is outdated since most messages now don't contain just a text string
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
      case STATE_LOBBY:
         cout << "In STATE_LOBBY" << endl;
      case STATE_GAME:
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
                  wndCurrent = wndLobby;
                
                  // this message should only be sent when a player first logs in so they know their id
  
                  Player* p = new Player("", "");
                  p->deserialize(msg.buffer);

                  if (mapPlayers.find(p->id) != mapPlayers.end())
                     delete mapPlayers[p->id];
                  mapPlayers[p->id] = p;
                  curPlayerId = p->id;

                  cout << "Got a valid login response with the player" << endl;
                  cout << "Player id: " << curPlayerId << endl;
                  cout << "Player health: " << p->health << endl;
                  cout << "player map size: " << mapPlayers.size() << endl;
               }

               break;
            }
            case MSG_TYPE_LOGOUT:
            {
               cout << "Got a logout message" << endl;

               int playerId;

               // Check if it's about you or another player
               memcpy(&playerId, msg.buffer, 4);
               response = string(msg.buffer+4);

               if (playerId == curPlayerId)
               {
                  if (response.compare("You have successfully logged out.") == 0)
                  {
                     cout << "Logged out" << endl;
                     state = STATE_START;
                     goToLoginScreen();
                  }

                  // if there was an error logging out, nothing happens
               }
               else
               {
                  delete mapPlayers[playerId];
               }

               break;
            }
            case MSG_TYPE_PLAYER:
            {
               cout << "Received MSG_TYPE_PLAYER" << endl;

               Player* p = new Player("", "");
               p->deserialize(msg.buffer);
               p->timeLastUpdated = getCurrentMillis();
               p->isChasing = false;
               if (p->health <= 0)
                  p->isDead = true;
               else
                  p->isDead = false;

               if (mapPlayers.find(p->id) != mapPlayers.end())
                    delete mapPlayers[p->id];
               mapPlayers[p->id] = p;

               break;
            }
            case MSG_TYPE_PLAYER_MOVE:
            {
               unsigned int id;
               int x, y;

               memcpy(&id, msg.buffer, 4);
               memcpy(&x, msg.buffer+4, 4);
               memcpy(&y, msg.buffer+8, 4);

               mapPlayers[id]->target.x = x;
               mapPlayers[id]->target.y = y;

               break;
            }
            case MSG_TYPE_CHAT:
            {
               chatConsole.addLine(response);

               break;
            }
            case MSG_TYPE_OBJECT:
            {
               cout << "Received OBJECT message" << endl;

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

               Player* source = mapPlayers[id];
               source->targetPlayer = targetID;
               source->isChasing = true;

               break;
            }
            case MSG_TYPE_PROJECTILE:
            {
               cout << "Received a PROJECTILE message" << endl;

               unsigned int id, x, y, targetId;

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
            case MSG_TYPE_GAME_INFO:
            {
               cout << "Received a GAME_INFO message" << endl;

               string gameName(msg.buffer+4);
               int numPlayers;

               memcpy(&numPlayers, msg.buffer, 4);
               
               cout << "Received game info for " << gameName << " (num players: " << numPlayers << ")" << endl;
               
               mapGames[gameName] = numPlayers;

               break;
            }
            case MSG_TYPE_JOIN_GAME_SUCCESS:
            {
               cout << "Received a JOIN_GAME_SUCCESS message" << endl;

               string gameName(msg.buffer);         
               game = new Game(gameName, "../../data/map.txt");
               cout << "Game name: " << gameName << endl;

               state = STATE_NEW_GAME;
               wndCurrent = wndNewGame;

               msgTo.type = MSG_TYPE_JOIN_GAME_ACK;
               strcpy(msgTo.buffer, gameName.c_str());

               msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);

               break;
            }
            case MSG_TYPE_JOIN_GAME_FAILURE:
            {
               cout << "Received a JOIN_GAME_FAILURE message" << endl;

               break;
            }
            default:
            {
               cout << "(STATE_LOBBY) Received invlaid message of type " << msg.type << endl;

               break;
            }
         }

         break;
      }
      case STATE_NEW_GAME:
      {
         cout << "(STATE_NEW_GAME) ";
         switch(msg.type)
         {
            case MSG_TYPE_GAME_INFO:
            {
               cout << "Received a GAME_INFO message" << endl;

               string gameName(msg.buffer+4);
               int numPlayers;

               memcpy(&numPlayers, msg.buffer, 4);
               
               cout << "Received game info for " << gameName << " (num players: " << numPlayers << ")" << endl;
               
               mapGames[gameName] = numPlayers;

               break;
            }
            case MSG_TYPE_OBJECT:
            {
               cout << "Received object message in STATE_NEW_GAME" << endl;

               WorldMap::Object o(0, WorldMap::OBJECT_NONE, 0, 0);
               o.deserialize(msg.buffer);
               cout << "object id: " << o.id << endl;
               game->getMap()->updateObject(o.id, o.type, o.pos.x, o.pos.y);

               break;
            }
            case MSG_TYPE_REMOVE_OBJECT:
            {
               cout << "Received REMOVE_OBJECT message!" << endl;

               int id;
               memcpy(&id, msg.buffer, 4);

               cout << "Removing object with id " << id << endl;

               if (!game->getMap()->removeObject(id))
                  cout << "Did not remove the object" << endl;

               break;
            }
            case MSG_TYPE_SCORE:
            {
               cout << "Received SCORE message!" << endl;

               int blueScore;
               memcpy(&blueScore, msg.buffer, 4);
               cout << "blue score: " << blueScore << endl;
               game->setBlueScore(blueScore);

               int redScore;
               memcpy(&redScore, msg.buffer+4, 4);
               cout << "red score: " << redScore << endl;
               game->setRedScore(redScore);

               cout << "Processed SCORE message!" << endl;
 
               break;
            }
            default:
            {
               cout << "Received invalid message of type " << msg.type << endl;

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
   int clientMsgOffset = 5;
   int serverMsgOffset = 950;

   al_draw_text(font, al_map_rgb(0, 255, 255), 0+clientMsgOffset, 43, ALLEGRO_ALIGN_LEFT, "ID");
   al_draw_text(font, al_map_rgb(0, 255, 255), 20+clientMsgOffset, 43, ALLEGRO_ALIGN_LEFT, "Type");
   al_draw_text(font, al_map_rgb(0, 255, 255), 240+clientMsgOffset, 43, ALLEGRO_ALIGN_LEFT, "Acked?");

   al_draw_text(font, al_map_rgb(0, 255, 255), serverMsgOffset, 43, ALLEGRO_ALIGN_LEFT, "ID");

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

         al_draw_text(font, al_map_rgb(0, 255, 0), clientMsgOffset, 60+15*msgCount, ALLEGRO_ALIGN_LEFT, ossId.str().c_str());
         al_draw_text(font, al_map_rgb(0, 255, 0), 20+clientMsgOffset, 60+15*msgCount, ALLEGRO_ALIGN_LEFT, typeStr.c_str());
         al_draw_text(font, al_map_rgb(0, 255, 0), 240+clientMsgOffset, 60+15*msgCount, ALLEGRO_ALIGN_LEFT, ossAcked.str().c_str());

         msgCount++;
      }
   }

   if (msgProcessor.getAckedMessages().size() > 0) {
      map<unsigned int, unsigned long long> ackedMessages = msgProcessor.getAckedMessages()[0];
      map<unsigned int, unsigned long long>::iterator it3;

      msgCount = 0;
      for (it3 = ackedMessages.begin(); it3 != ackedMessages.end(); it3++) {
         ossId.str("");;
         ossId << it3->first;

         al_draw_text(font, al_map_rgb(255, 0, 0), 25+serverMsgOffset, 60+15*msgCount, ALLEGRO_ALIGN_LEFT, ossId.str().c_str());

         msgCount++;
      }
   }
}

// Callback definitions

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

   msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);
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

   msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);

   state = STATE_LOBBY;
}

void logout()
{
   switch(state) {
   case STATE_LOBBY:
      txtJoinGame->clear();
      txtCreateGame->clear();
      break;
   case STATE_GAME:
      txtChat->clear();
      chatConsole.clear();
      break;
   default:
      cout << "Logout called from invalid state: " << state << endl;
      break;
   }

   msgTo.type = MSG_TYPE_LOGOUT;

   strcpy(msgTo.buffer, username.c_str());

   msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);
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

   msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);
}

void toggleDebugging()
{
   debugging = !debugging;
}

void joinGame()
{
   cout << "Joining game" << endl;

   string msg = txtJoinGame->getStr();
   txtJoinGame->clear();

   msgTo.type = MSG_TYPE_JOIN_GAME;
   strcpy(msgTo.buffer, msg.c_str());

   msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);
}

void createGame()
{
   cout << "Creating game" << endl;

   string msg = txtCreateGame->getStr();
   txtCreateGame->clear();

   cout << "Sending message: " << msg.c_str() << endl;

   msgTo.type = MSG_TYPE_CREATE_GAME;
   strcpy(msgTo.buffer, msg.c_str());

   msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);
}

void leaveGame()
{
   cout << "Leaving game" << endl;

   game = NULL;
   
   state = STATE_LOBBY;
   wndCurrent = wndLobby;

   msgTo.type = MSG_TYPE_LEAVE_GAME;

   msgProcessor.sendMessage(&msgTo, sock, &server, &outputLog);
}
