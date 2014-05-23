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
//#include <cmath>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <stdexcept>

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
#include "../../common/GameSummary.h"

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
void createGui(ALLEGRO_FONT* font);

void processMessage(NETWORK_MSG &msg, int &state, chat &chatConsole, map<unsigned int, Player*>& mapPlayers, map<string, int>& mapGames, unsigned int& curPlayerId);
void handleMsgPlayer(NETWORK_MSG &msg, map<unsigned int, Player*>& mapPlayers, map<string, int>& mapGames);
void handleMsgGameInfo(NETWORK_MSG &msg, map<unsigned int, Player*>& mapPlayers, map<string, int>& mapGames);

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
void closeGameSummary();

const float FPS = 60;
const int SCREEN_W = 1024;
const int SCREEN_H = 768;

enum STATE {
   STATE_START,
   STATE_LOBBY,
   STATE_GAME
};

int state;

bool doexit;

vector<GuiComponent*> vctComponents;

Window* wndLogin;
Window* wndRegister;
Window* wndLobby;
Window* wndLobbyDebug;
Window* wndGame;
Window* wndGameSummary;
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
Textbox* txtChat;

int sock;
struct sockaddr_in server, from;
struct hostent *hp;
NETWORK_MSG msgTo, msgFrom;
string username;
chat chatConsole, debugConsole;
bool debugging;
Game* game;
GameSummary* gameSummary;

MessageProcessor msgProcessor;

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display = NULL;
   ALLEGRO_EVENT_QUEUE *event_queue = NULL;
   ALLEGRO_TIMER *timer = NULL;
   map<unsigned int, Player*> mapPlayers;
   map<string, int> mapGames;
   unsigned int curPlayerId = -1;
   ofstream outputLog;

   doexit = false;
   debugging = false;
   bool redraw = true;
   bool fullscreen = false;
   game = NULL;
   gameSummary = NULL;

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

   ALLEGRO_FONT* font;
   #if defined WINDOWS
      font = al_load_ttf_font("../pirulen.ttf", 12, 0);
   #elif defined LINUX
      font = al_load_ttf_font("pirulen.ttf", 12, 0);
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

   debugConsole.addLine("Debug console:");
   debugConsole.addLine("");

   createGui(font);

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
   if (hp == 0)
      error("Unknown host");

   memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);
   server.sin_port = htons(atoi(argv[2]));

   msgProcessor = MessageProcessor(sock, &outputLog);

   al_start_timer(timer);

   while (!doexit)
   {
      ALLEGRO_EVENT ev;

      al_wait_for_event(event_queue, &ev);

      if(wndCurrent->handleEvent(ev)) {
         // do nothing
      }
      else if(ev.type == ALLEGRO_EVENT_TIMER) {
         redraw = true;

         // remove any other timer events in the queue
         while (al_peek_next_event(event_queue, &ev) && ev.type == ALLEGRO_EVENT_TIMER) {
            al_get_next_event(event_queue, &ev);
         }
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
                  msgProcessor.sendMessage(&msgTo, &server);
               }
               break;
            case ALLEGRO_KEY_D:  // drop the current item
               if (state == STATE_GAME) {
                  try {
                     Player* p = mapPlayers.at(curPlayerId);
                     int flagType = OBJECT_NONE;

                     if (p->hasBlueFlag)
                        flagType = OBJECT_BLUE_FLAG;
                     else if (p->hasRedFlag)
                        flagType = OBJECT_RED_FLAG;

                     if (flagType != OBJECT_NONE) {
                        msgTo.type = MSG_TYPE_DROP_FLAG;
                        memcpy(msgTo.buffer, &curPlayerId, 4);
                        msgProcessor.sendMessage(&msgTo, &server);
                     }
                  } catch (const out_of_range& ex) {}
               }
               break;
         }
      }
      else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
         if(wndCurrent == wndGame) {
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

                  msgProcessor.sendMessage(&msgTo, &server);
               }
               else
                  cout << "Invalid point: User did not click on the map" << endl;
            }else if (ev.mouse.button == 2) {   // right click
               cout << "Detected a right-click" << endl;
               map<unsigned int, Player*>::iterator it;

               Player* curPlayer = mapPlayers[curPlayerId];;

               cout << "Got current player" << endl;
               cout << "current game: " << game << endl;

               map<unsigned int, Player*> playersInGame = game->getPlayers();
               Player* target;

               for(it = playersInGame.begin(); it != playersInGame.end(); it++)
               {
                  target = it->second;
                  cout << "set target" << endl;
                  if (target->team != curPlayer->team)
                  {
                     cout << "Found valid target" << endl;

                     POSITION cursorPos;
                     cursorPos.x = ev.mouse.x;
                     cursorPos.y = ev.mouse.y;
                     cursorPos = screenToMap(cursorPos);

                     float distance =posDistance(cursorPos.toFloat(), target->pos);

                     if (distance < 25) {
                        unsigned int targetId = target->getId();

                        msgTo.type = MSG_TYPE_ATTACK;
                        memcpy(msgTo.buffer, &curPlayerId, 4);
                        memcpy(msgTo.buffer+4, &targetId, 4);

                        msgProcessor.sendMessage(&msgTo, &server);
                     }
                  }
               }
            }
         }
      }

      if (msgProcessor.receiveMessage(&msgFrom, &from) >= 0)
         processMessage(msgFrom, state, chatConsole, mapPlayers, mapGames, curPlayerId);

      if (redraw)
      {
         redraw = false;

         msgProcessor.resendUnackedMessages();

         if (debugging && wndCurrent == wndLobby)
            wndLobbyDebug->draw(display);
         else
            wndCurrent->draw(display);

         if (wndCurrent == wndLobby) {
            if (!debugging)
               chatConsole.draw(font, al_map_rgb(255,255,255));

            al_draw_text(font, al_map_rgb(0, 255, 0), SCREEN_W*1/2-100, 120, ALLEGRO_ALIGN_LEFT, "Current Games");

            map<string, int>::iterator it;
            int i=0;
            ostringstream oss;
            for (it = mapGames.begin(); it != mapGames.end(); it++) {
               oss << it->first << " (" << it->second << " players)" << endl;
               al_draw_text(font, al_map_rgb(0, 255, 0), SCREEN_W*1/2-100, 135+i*15, ALLEGRO_ALIGN_LEFT, oss.str().c_str());
               oss.clear();
               oss.str("");
               i++;
            }

            al_draw_text(font, al_map_rgb(0, 255, 0), SCREEN_W*3/4-100, 120, ALLEGRO_ALIGN_LEFT, "Online Players");

            map<unsigned int, Player*>::iterator itPlayers;
            i=0;
            for (itPlayers = mapPlayers.begin(); itPlayers != mapPlayers.end(); itPlayers++) {
               oss << itPlayers->second->name << endl;
               al_draw_text(font, al_map_rgb(0, 255, 0), SCREEN_W*3/4-100, 135+i*15, ALLEGRO_ALIGN_LEFT, oss.str().c_str());
               oss.clear();
               oss.str("");
               i++;
            }
         }
         else if (wndCurrent == wndGame)
         {
            al_draw_text(font, al_map_rgb(0, 255, 0), 4, 4, ALLEGRO_ALIGN_LEFT, "Players");

            map<unsigned int, Player*>& gamePlayers = game->getPlayers();
            map<unsigned int, Player*>::iterator it;

            if (!debugging) {
               int playerCount = 0;
               for (it = gamePlayers.begin(); it != gamePlayers.end(); it++)
               {
                  al_draw_text(font, al_map_rgb(0, 255, 0), 4, 19+(playerCount+1)*15, ALLEGRO_ALIGN_LEFT, it->second->name.c_str());
                  playerCount++;
               }
            }

            ostringstream ossScoreBlue, ossScoreRed;

            ossScoreBlue << "Blue: " << game->getBlueScore() << endl;
            ossScoreRed << "Red: " << game->getRedScore() << endl;

            al_draw_text(font, al_map_rgb(0, 255, 0), 330, 80, ALLEGRO_ALIGN_LEFT, ossScoreBlue.str().c_str());
            al_draw_text(font, al_map_rgb(0, 255, 0), 515, 80, ALLEGRO_ALIGN_LEFT, ossScoreRed.str().c_str());

            // update players
            for (it = game->getPlayers().begin(); it != game->getPlayers().end(); it++)
            {
               it->second->updateTarget(game->getPlayers());
            }

            for (it = game->getPlayers().begin(); it != game->getPlayers().end(); it++)
            {
               it->second->move(game->getMap());    // ignore return value
            }

             // update projectile positions
            map<unsigned int, Projectile>::iterator it2;
            for (it2 = game->getProjectiles().begin(); it2 != game->getProjectiles().end(); it2++)
            {
               it2->second.move(game->getPlayers());
            }

            GameRender::drawMap(game->getMap());
            GameRender::drawPlayers(game->getPlayers(), font, curPlayerId);
            GameRender::drawProjectiles(game->getProjectiles(), game->getPlayers());
         }
         else if (wndCurrent == wndGameSummary)
         {
            ostringstream ossBlueScore, ossRedScore;

            ossBlueScore << "Blue Score: " << gameSummary->getBlueScore();
            ossRedScore << "Red Score: " << gameSummary->getRedScore();

            string strWinner;

            if (gameSummary->getWinner() == 0)
               strWinner = "Blue Team Wins";
            else if (gameSummary->getWinner() == 1)
               strWinner = "Red Team Wins";
            else
               strWinner = "winner set to wrong value";

            al_draw_text(font, al_map_rgb(0, 255, 0), 512, 40, ALLEGRO_ALIGN_CENTRE, gameSummary->getName().c_str());
            al_draw_text(font, al_map_rgb(0, 255, 0), 330, 80, ALLEGRO_ALIGN_LEFT, ossBlueScore.str().c_str());
            al_draw_text(font, al_map_rgb(0, 255, 0), 515, 80, ALLEGRO_ALIGN_LEFT, ossRedScore.str().c_str());
            al_draw_text(font, al_map_rgb(0, 255, 0), 512, 120, ALLEGRO_ALIGN_CENTRE, strWinner.c_str());
         }

         if (debugging) {
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
   
   // delete all components
   for (unsigned int x=0; x<vctComponents.size(); x++)
      delete vctComponents[x];

   delete wndLogin;
   delete wndRegister;
   delete wndLobby;
   delete wndLobbyDebug;
   delete wndGame;
   delete wndGameSummary;

   // game should be deleted when the player leaves a gamw
   if (game != NULL)
      delete game;

   if (gameSummary != NULL)
      delete gameSummary;

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

void createGui(ALLEGRO_FONT* font) {
   // wndLogin

   wndLogin = new Window(0, 0, SCREEN_W, SCREEN_H);
   vctComponents.push_back(wndLogin->addComponent(new Textbox(516, 40, 100, 20, font)));
   vctComponents.push_back(wndLogin->addComponent(new Textbox(516, 70, 100, 20, font)));
   vctComponents.push_back(wndLogin->addComponent(new TextLabel(410, 40, 100, 20, font, "Username:", ALLEGRO_ALIGN_RIGHT)));
   vctComponents.push_back(wndLogin->addComponent(new TextLabel(410, 70, 100, 20, font, "Password:", ALLEGRO_ALIGN_RIGHT)));
   vctComponents.push_back(wndLogin->addComponent(new TextLabel((SCREEN_W-600)/2, 100, 600, 20, font, "", ALLEGRO_ALIGN_CENTRE)));
   vctComponents.push_back(wndLogin->addComponent(new Button(SCREEN_W/2-100, 130, 90, 20, font, "Register", goToRegisterScreen)));
   vctComponents.push_back(wndLogin->addComponent(new Button(SCREEN_W/2+10, 130, 90, 20, font, "Login", login)));
   vctComponents.push_back(wndLogin->addComponent(new Button(920, 10, 80, 20, font, "Quit", quit)));
   vctComponents.push_back(wndLogin->addComponent(new Button(20, 10, 160, 20, font, "Toggle Debugging", toggleDebugging)));

   txtUsername = (Textbox*)wndLogin->getComponent(0);
   txtPassword = (Textbox*)wndLogin->getComponent(1);
   lblLoginStatus = (TextLabel*)wndLogin->getComponent(4);

   cout << "Created login screen" << endl;


   // wndRegister

   wndRegister = new Window(0, 0, SCREEN_W, SCREEN_H);
   vctComponents.push_back(wndRegister->addComponent(new Textbox(516, 40, 100, 20, font)));
   vctComponents.push_back(wndRegister->addComponent(new Textbox(516, 70, 100, 20, font)));
   vctComponents.push_back(wndRegister->addComponent(new TextLabel(410, 40, 100, 20, font, "Username:", ALLEGRO_ALIGN_RIGHT)));
   vctComponents.push_back(wndRegister->addComponent(new TextLabel(410, 70, 100, 20, font, "Password:", ALLEGRO_ALIGN_RIGHT)));
   vctComponents.push_back(wndRegister->addComponent(new RadioButtonList(432, 100, "Pick a class", font)));
   vctComponents.push_back(wndRegister->addComponent(new TextLabel((SCREEN_W-600)/2, 190, 600, 20, font, "", ALLEGRO_ALIGN_CENTRE)));
   vctComponents.push_back(wndRegister->addComponent(new Button(SCREEN_W/2-100, 220, 90, 20, font, "Back", goToLoginScreen)));
   vctComponents.push_back(wndRegister->addComponent(new Button(SCREEN_W/2+10, 220, 90, 20, font, "Submit", registerAccount)));
   vctComponents.push_back(wndRegister->addComponent(new Button(920, 10, 80, 20, font, "Quit", quit)));
   vctComponents.push_back(wndRegister->addComponent(new Button(20, 10, 160, 20, font, "Toggle Debugging", toggleDebugging)));

   txtUsernameRegister = (Textbox*)wndRegister->getComponent(0);
   txtPasswordRegister = (Textbox*)wndRegister->getComponent(1);

   rblClasses = (RadioButtonList*)wndRegister->getComponent(4);
   rblClasses->addRadioButton("Warrior");
   rblClasses->addRadioButton("Ranger");

   lblRegisterStatus = (TextLabel*)wndRegister->getComponent(5);

   cout << "Created register screen" << endl;


   // wndLobby

   txtJoinGame = new Textbox(SCREEN_W*1/2+15+4, 40, 100, 20, font);
   vctComponents.push_back(txtJoinGame);

   txtCreateGame = new Textbox(SCREEN_W*3/4+4, 40, 100, 20, font);
   vctComponents.push_back(txtCreateGame);

   wndLobby = new Window(0, 0, SCREEN_W, SCREEN_H);
   vctComponents.push_back(wndLobby->addComponent(new Button(920, 10, 80, 20, font, "Logout", logout)));
   vctComponents.push_back(wndLobby->addComponent(new TextLabel(SCREEN_W*1/2+15-112, 40, 110, 20, font, "Game Name:", ALLEGRO_ALIGN_RIGHT)));
   wndLobby->addComponent(txtJoinGame);
   vctComponents.push_back(wndLobby->addComponent(new Button(SCREEN_W*1/2+15-100, 80, 200, 20, font, "Join Existing Game", joinGame)));
   vctComponents.push_back(wndLobby->addComponent(new TextLabel(SCREEN_W*3/4-112, 40, 110, 20, font, "Game Name:", ALLEGRO_ALIGN_RIGHT)));
   wndLobby->addComponent(txtCreateGame);
   vctComponents.push_back(wndLobby->addComponent(new Button(SCREEN_W*3/4-100, 80, 200, 20, font, "Create New Game", createGame)));
   vctComponents.push_back(wndLobby->addComponent(new Textbox(95, 40, 300, 20, font)));
   vctComponents.push_back(wndLobby->addComponent(new Button(95, 70, 60, 20, font, "Send", sendChatMessage)));
   vctComponents.push_back(wndLobby->addComponent(new Button(20, 10, 160, 20, font, "Toggle Debugging", toggleDebugging)));

   txtChat = (Textbox*)wndLobby->getComponent(7);

   cout << "Created lobby screen" << endl;


   // wndLobbyDebug

   wndLobbyDebug = new Window(0, 0, SCREEN_W, SCREEN_H);
   vctComponents.push_back(wndLobbyDebug->addComponent(new Button(920, 10, 80, 20, font, "Logout", logout)));
   vctComponents.push_back(wndLobbyDebug->addComponent(new TextLabel(SCREEN_W*1/2+15-112, 40, 110, 20, font, "Game Name:", ALLEGRO_ALIGN_RIGHT)));
   wndLobbyDebug->addComponent(txtJoinGame);
   vctComponents.push_back(wndLobbyDebug->addComponent(new Button(SCREEN_W*1/2+15-100, 80, 200, 20, font, "Join Existing Game", joinGame)));
   vctComponents.push_back(wndLobbyDebug->addComponent(new TextLabel(SCREEN_W*3/4-112, 40, 110, 20, font, "Game Name:", ALLEGRO_ALIGN_RIGHT)));
   wndLobbyDebug->addComponent(txtCreateGame);
   vctComponents.push_back(wndLobbyDebug->addComponent(new Button(SCREEN_W*3/4-100, 80, 200, 20, font, "Create New Game", createGame)));
   vctComponents.push_back(wndLobbyDebug->addComponent(new Button(20, 10, 160, 20, font, "Toggle Debugging", toggleDebugging)));

   cout << "Created debug lobby screen" << endl;


   // wndGame

   wndGame = new Window(0, 0, SCREEN_W, SCREEN_H);
   vctComponents.push_back(wndGame->addComponent(new Button(880, 10, 120, 20, font, "Leave Game", leaveGame)));

   cout << "Created new game screen" << endl;

   wndGameSummary = new Window(0, 0, SCREEN_W, SCREEN_H);
   vctComponents.push_back(wndGameSummary->addComponent(new Button(840, 730, 160, 20, font, "Back to Lobby", closeGameSummary)));

   cout << "Created game summary screen" << endl;
}

void processMessage(NETWORK_MSG &msg, int &state, chat &chatConsole, map<unsigned int, Player*>& mapPlayers, map<string, int>& mapGames, unsigned int& curPlayerId)
{
   cout << "Total players in map: " << mapPlayers.size() << endl;

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
      {
         cout << "In STATE_LOBBY" << endl;
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

                  if (mapPlayers.find(p->getId()) != mapPlayers.end())
                     delete mapPlayers[p->getId()];
                  mapPlayers[p->getId()] = p;
                  curPlayerId = p->getId();

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

               unsigned int playerId;

               // Check if it's about you or another player
               memcpy(&playerId, msg.buffer, 4);
               response = string(msg.buffer+4);

               if (playerId == curPlayerId)
               {
                  cout << "Got logout message for self" << endl;

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
                  mapPlayers.erase(playerId);
               }

               break;
            }
            case MSG_TYPE_CHAT:
            {
               chatConsole.addLine(response);

               break;
            }
            case MSG_TYPE_JOIN_GAME_SUCCESS:
            {
               cout << "Received a JOIN_GAME_SUCCESS message" << endl;

               string gameName(msg.buffer);

               #if defined WINDOWS
                  game = new Game(gameName, "../../data/map.txt", &msgProcessor);
               #elif defined LINUX
                  game = new Game(gameName, "../data/map.txt", &msgProcessor);
               #endif

               cout << "Game name: " << gameName << endl;

               state = STATE_GAME;
               wndCurrent = wndGame;

               msgTo.type = MSG_TYPE_JOIN_GAME_ACK;
               strcpy(msgTo.buffer, gameName.c_str());

               msgProcessor.sendMessage(&msgTo, &server);

               break;
            }
            case MSG_TYPE_JOIN_GAME_FAILURE:
            {
               cout << "Received a JOIN_GAME_FAILURE message" << endl;

               break;
            }
            case MSG_TYPE_PLAYER:
            {
               handleMsgPlayer(msg, mapPlayers, mapGames);

               break;
            }
            case MSG_TYPE_GAME_INFO:
            {
               handleMsgGameInfo(msg, mapPlayers, mapGames);

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
      case STATE_GAME:
      {
         cout << "(STATE_GAME) ";
         switch(msg.type)
         {
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
            case MSG_TYPE_FINISH_GAME:
            {
               cout << "Got a finish game message" << endl;
               cout << "Should switch to STATE_LOBBY and show the final score" << endl;

               unsigned int winner, blueScore, redScore;
               memcpy(&winner, msg.buffer, 4);
               memcpy(&blueScore, msg.buffer+4, 4);
               memcpy(&redScore, msg.buffer+8, 4);

               string gameName(msg.buffer+12);

               gameSummary = new GameSummary(gameName, winner, blueScore, redScore);

               delete game;
               game = NULL;
               state = STATE_LOBBY;
               wndCurrent = wndGameSummary;

               break;
            }
            case MSG_TYPE_LOGOUT:
            {
               cout << "Got a logout message" << endl;

               unsigned int playerId;

               // Check if it's about you or another player
               memcpy(&playerId, msg.buffer, 4);
               response = string(msg.buffer+4);

               if (playerId == curPlayerId)
                  cout << "Received MSG_TYPE_LOGOUT for self in STATE_GAME. This shouldn't happen." << endl;
               else {
                  delete mapPlayers[playerId];
                  mapPlayers.erase(playerId);
               }

               break;
            }
            case MSG_TYPE_PLAYER_JOIN_GAME:
            {
               cout << "Received MSG_TYPE_PLAYER_JOIN_GAME" << endl;

               Player p("", "");
               p.deserialize(msg.buffer);
               cout << "Deserialized player" << endl;
               p.timeLastUpdated = getCurrentMillis();
               p.isChasing = false;
               if (p.health <= 0)
                  p.isDead = true;
               else
                  p.isDead = false;

               if (mapPlayers.find(p.getId()) != mapPlayers.end())
                  *(mapPlayers[p.getId()]) = p;
               else
                  mapPlayers[p.getId()] = new Player(p);

               game->addPlayer(mapPlayers[p.getId()]);

               break;
            }
            case MSG_TYPE_LEAVE_GAME:
            {
               cout << "Received a LEAVE_GAME message" << endl;

               string gameName(msg.buffer+4);
               unsigned int playerId;

               memcpy(&playerId, msg.buffer, 4);
               
               game->removePlayer(playerId);

               break;
            }
            case MSG_TYPE_PLAYER_MOVE:
            {
               cout << "Received PLAYER_MOVE message" << endl;

               unsigned int id;
               int x, y;

               memcpy(&id, msg.buffer, 4);
               memcpy(&x, msg.buffer+4, 4);
               memcpy(&y, msg.buffer+8, 4);

               cout << "id: " << id << endl;

               mapPlayers[id]->target.x = x;
               mapPlayers[id]->target.y = y;

               mapPlayers[id]->isChasing = false;
               mapPlayers[id]->setTargetPlayer(0);

               break;
            }
            case MSG_TYPE_OBJECT:
            {
               cout << "Received object message in STATE_GAME" << endl;

               WorldMap::Object o(0, OBJECT_NONE, 0, 0);
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
            case MSG_TYPE_ATTACK:
            {
               cout << "Received START_ATTACK message" << endl;

               unsigned int id, targetId;
               memcpy(&id, msg.buffer, 4);
               memcpy(&targetId, msg.buffer+4, 4);

               cout << "source id: " << id << endl;
               cout << "target id: " << targetId << endl;

               // need to check the target exists in the current game
               Player* source = game->getPlayers()[id];
               source->setTargetPlayer(targetId);
               source->isChasing = true;

               break;
            }
            case MSG_TYPE_PROJECTILE:
            {
               cout << "Received a PROJECTILE message" << endl;

               unsigned int projId, x, y, targetId;

               memcpy(&projId, msg.buffer, 4);
               memcpy(&x, msg.buffer+4, 4);
               memcpy(&y, msg.buffer+8, 4);
               memcpy(&targetId, msg.buffer+12, 4);

               cout << "projId: " << projId << endl;
               cout << "x: " << x << endl;
               cout << "y: " << y << endl;
               cout << "Target: " << targetId << endl;

               Projectile proj(x, y, targetId, 0);
               proj.setId(projId);

               game->addProjectile(proj);

               break;
            }
            case MSG_TYPE_REMOVE_PROJECTILE:
            {
                cout << "Received a REMOVE_PROJECTILE message" << endl;

               unsigned int id;
               memcpy(&id, msg.buffer, 4);
               
               game->removeProjectile(id);

               break;
            }
            case MSG_TYPE_PLAYER:
            {
               handleMsgPlayer(msg, mapPlayers, mapGames);

               break;
            }
            case MSG_TYPE_GAME_INFO:
            {
               handleMsgGameInfo(msg, mapPlayers, mapGames);

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

   //al_draw_text(font, al_map_rgb(0, 255, 255), serverMsgOffset, 43, ALLEGRO_ALIGN_LEFT, "ID");

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

// message handling functions

void handleMsgPlayer(NETWORK_MSG &msg, map<unsigned int, Player*>& mapPlayers, map<string, int>& mapGames) {
   cout << "Received MSG_TYPE_PLAYER" << endl;

   Player p("", "");
   p.deserialize(msg.buffer);
   p.timeLastUpdated = getCurrentMillis();
   p.isChasing = false;
   if (p.health <= 0)
      p.isDead = true;
   else
      p.isDead = false;

   if (mapPlayers.find(p.getId()) != mapPlayers.end())
      *(mapPlayers[p.getId()]) = p;
   else
      mapPlayers[p.getId()] = new Player(p);
}

void handleMsgGameInfo(NETWORK_MSG &msg, map<unsigned int, Player*>& mapPlayers, map<string, int>& mapGames) {
   cout << "Received a GAME_INFO message" << endl;

   string gameName(msg.buffer+4);
   int numPlayers;

   memcpy(&numPlayers, msg.buffer, 4);
               
   cout << "Received game info for " << gameName << " (num players: " << numPlayers << ")" << endl;
               
   if (numPlayers > 0)
      mapGames[gameName] = numPlayers;
   else
      mapGames.erase(gameName);
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

   msgProcessor.sendMessage(&msgTo, &server);
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

   msgProcessor.sendMessage(&msgTo, &server);

   state = STATE_LOBBY;
}

void logout()
{
   switch(state) {
   case STATE_LOBBY:
      txtJoinGame->clear();
      txtCreateGame->clear();
      break;
   default:
      cout << "Logout called from invalid state: " << state << endl;
      break;
   }

   msgTo.type = MSG_TYPE_LOGOUT;

   strcpy(msgTo.buffer, username.c_str());

   msgProcessor.sendMessage(&msgTo, &server);
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

   msgProcessor.sendMessage(&msgTo, &server);
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

   msgProcessor.sendMessage(&msgTo, &server);
}

void createGame()
{
   cout << "Creating game" << endl;

   string msg = txtCreateGame->getStr();
   txtCreateGame->clear();

   cout << "Sending message: " << msg.c_str() << endl;

   msgTo.type = MSG_TYPE_CREATE_GAME;
   strcpy(msgTo.buffer, msg.c_str());

   msgProcessor.sendMessage(&msgTo, &server);
}

void leaveGame()
{
   cout << "Leaving game" << endl;

   delete game;
   game = NULL;
   
   state = STATE_LOBBY;
   wndCurrent = wndLobby;

   msgTo.type = MSG_TYPE_LEAVE_GAME;

   msgProcessor.sendMessage(&msgTo, &server);
}

void closeGameSummary()
{
    delete gameSummary;
    gameSummary = NULL;
    wndCurrent = wndLobby;
    cout << "Processed button actions" << endl;
}
