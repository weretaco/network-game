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

#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>

#include <map>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

#include "../../common/Message.h"
#include "../../common/Common.h"
#include "../../common/Player.h"_

#include "Window.h"
#include "Textbox.h"
#include "Button.h"
#include "chat.h"

#ifdef WINDOWS
   #pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

void initWinSock();
void shutdownWinSock();
void processMessage(NETWORK_MSG &msg, int &state, chat &chatConsole, map<unsigned int, Player>& mapPlayers, unsigned int& curPlayerId);
void drawPlayers(map<unsigned int, Player>& mapPlayers, unsigned int curPlayerId);

// callbacks
void registerAccount();
void login();
void logout();
void quit();
void sendChatMessage();

void error(const char *);

const float FPS = 60;
const int SCREEN_W = 640;
const int SCREEN_H = 480;
const int BOUNCER_SIZE = 32;
enum MYKEYS {
   KEY_UP,
   KEY_DOWN,
   KEY_LEFT,
   KEY_RIGHT
};

enum STATE {
   STATE_START,
   STATE_LOGIN // this means you're already logged in
};

int state;

bool doexit;

Window* wndLogin;
Window* wndMain;
Window* wndCurrent;

Textbox* txtUsername;
Textbox* txtPassword;
Textbox* txtChat;

int sock;
struct sockaddr_in server, from;
struct hostent *hp;
NETWORK_MSG msgTo, msgFrom;
string username;
chat chatConsole;
 
int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display = NULL;
   ALLEGRO_EVENT_QUEUE *event_queue = NULL;
   ALLEGRO_TIMER *timer = NULL;
   ALLEGRO_BITMAP *bouncer = NULL;
   bool key[4] = { false, false, false, false };
   bool redraw = true;
   doexit = false;
   map<unsigned int, Player> mapPlayers;
   unsigned int curPlayerId = -1;

   float bouncer_x = SCREEN_W / 2.0 - BOUNCER_SIZE / 2.0;
   float bouncer_y = SCREEN_H / 2.0 - BOUNCER_SIZE / 2.0;

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
 
   display = al_create_display(SCREEN_W, SCREEN_H);
   if(!display) {
      fprintf(stderr, "failed to create display!\n");
      al_destroy_timer(timer);
      return -1;
   }

   wndLogin = new Window(0, 0, SCREEN_W, SCREEN_H);
   wndLogin->addComponent(new Textbox(104, 40, 100, 20, font));
   wndLogin->addComponent(new Textbox(104, 70, 100, 20, font));
   wndLogin->addComponent(new Button(22, 100, 90, 20, font, "Register", registerAccount));
   wndLogin->addComponent(new Button(122, 100, 60, 20, font, "Login", login));
   wndLogin->addComponent(new Button(540, 10, 80, 20, font, "Quit", quit));

   txtUsername = (Textbox*)wndLogin->getComponent(0);
   txtPassword = (Textbox*)wndLogin->getComponent(1);

   wndMain = new Window(0, 0, SCREEN_W, SCREEN_H);
   wndMain->addComponent(new Textbox(95, 40, 525, 20, font));
   wndMain->addComponent(new Button(95, 70, 160, 20, font, "Send Message", sendChatMessage));
   wndMain->addComponent(new Button(540, 10, 80, 20, font, "Logout", logout));

   txtChat = (Textbox*)wndMain->getComponent(0);

   wndCurrent = wndLogin;

   bouncer = al_create_bitmap(BOUNCER_SIZE, BOUNCER_SIZE);
   if(!bouncer) {
      fprintf(stderr, "failed to create bouncer bitmap!\n");
      al_destroy_display(display);
      al_destroy_timer(timer);
      return -1;
   }
 
   event_queue = al_create_event_queue();
   if(!event_queue) {
      fprintf(stderr, "failed to create event_queue!\n");
      al_destroy_bitmap(bouncer);
      al_destroy_display(display);
      al_destroy_timer(timer);
      return -1;
   }

   al_set_target_bitmap(bouncer);
 
   al_clear_to_color(al_map_rgb(255, 0, 255));
 
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
         if(key[KEY_UP] && bouncer_y >= 4.0) {
            bouncer_y -= 4.0;
         }
 
         if(key[KEY_DOWN] && bouncer_y <= SCREEN_H - BOUNCER_SIZE - 4.0) {
            bouncer_y += 4.0;
         }
 
         if(key[KEY_LEFT] && bouncer_x >= 4.0) {
            bouncer_x -= 4.0;
         }
 
         if(key[KEY_RIGHT] && bouncer_x <= SCREEN_W - BOUNCER_SIZE - 4.0) {
            bouncer_x += 4.0;
         }
 
         redraw = true;
      }
      else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         doexit = true;
      }
      else if(ev.type == ALLEGRO_EVENT_KEY_DOWN) {
         switch(ev.keyboard.keycode) {
            case ALLEGRO_KEY_UP:
               key[KEY_UP] = true;
               break;
 
            case ALLEGRO_KEY_DOWN:
               key[KEY_DOWN] = true;
               break;
 
            case ALLEGRO_KEY_LEFT: 
               key[KEY_LEFT] = true;
               break;
 
            case ALLEGRO_KEY_RIGHT:
               key[KEY_RIGHT] = true;
               break;
         }
      }
      else if(ev.type == ALLEGRO_EVENT_KEY_UP) {
         switch(ev.keyboard.keycode) {
            case ALLEGRO_KEY_UP:
               key[KEY_UP] = false;
               break;
 
            case ALLEGRO_KEY_DOWN:
               key[KEY_DOWN] = false;
               break;
 
            case ALLEGRO_KEY_LEFT: 
               key[KEY_LEFT] = false;
               break;
 
            case ALLEGRO_KEY_RIGHT:
               key[KEY_RIGHT] = false;
               break;
 
            case ALLEGRO_KEY_ESCAPE:
               doexit = true;
               break;
         }
      }
      else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
         mapPlayers[curPlayerId].pos.x = ev.mouse.x;
         mapPlayers[curPlayerId].pos.y = ev.mouse.y;

         // send the server a MSG_TYPE_PLAYER_MOVE message
         msgTo.type = MSG_TYPE_PLAYER_MOVE;

         ostringstream oss;
         oss << ev.mouse.x;
         oss << ev.mouse.y;

         memcpy(msgTo.buffer, oss.str().c_str(), oss.str().length());
         sendMessage(&msgTo, sock, &server);
      }

      if (receiveMessage(&msgFrom, sock, &from) >= 0)
      {
         processMessage(msgFrom, state, chatConsole, mapPlayers, curPlayerId);
         cout << "state: " << state << endl;
      }
 
      if (redraw && al_is_event_queue_empty(event_queue))
      {
         redraw = false;

         wndCurrent->draw(display);

         drawPlayers(mapPlayers, curPlayerId);

         chatConsole.draw(font, al_map_rgb(255,255,255));

         if(wndCurrent == wndLogin) {
            al_draw_text(font, al_map_rgb(0, 255, 0), 4, 43, ALLEGRO_ALIGN_LEFT, "Username:");
            al_draw_text(font, al_map_rgb(0, 255, 0), 1, 73, ALLEGRO_ALIGN_LEFT, "Password:");
         }
         else if(wndCurrent == wndMain) {
            al_draw_text(font, al_map_rgb(0, 255, 0), 4, 43, ALLEGRO_ALIGN_LEFT, "Message:");
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

   al_destroy_event_queue(event_queue);
   al_destroy_bitmap(bouncer);
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

void processMessage(NETWORK_MSG &msg, int &state, chat &chatConsole, map<unsigned int, Player>& mapPlayers, unsigned int& curPlayerId)
{
   string response = string(msg.buffer);

   cout << "Got message: " << msg.type << endl;

   switch(state)
   {
      case STATE_START:
      {
         cout << "In STATE_START" << endl;

         switch(msg.type)
         {
            case MSG_TYPE_REGISTER:
            {
               break;
            }
            case MSG_TYPE_LOGIN:
            {
               if (response.compare("Player has already logged in.") == 0)
               {
                  username.clear();
                  cout << "User login failed" << endl;
               }
               else if (response.compare("Incorrect username or password") == 0)
               {
                  username.clear();
                  cout << "User login failed" << endl;
               }
               else
               {
                  state = STATE_LOGIN;
                  wndCurrent = wndMain;
                  
                  Player p("", "");
                  p.deserialize(msg.buffer);
                  mapPlayers[p.id] = p;
                  curPlayerId = p.id;

                  cout << "Got a valid login response with the player" << endl;
                  cout << "Player id: " << curPlayerId << endl; 
               }

               break;
            }
         }

         break;
      }
      case STATE_LOGIN:
      {
         switch(msg.type)
         {
            case MSG_TYPE_REGISTER:
            {
               break;
            }
            case MSG_TYPE_LOGIN:
            {
               chatConsole.addLine(response);

               if (response.compare("You have successfully logged out.") == 0)
               {
                  cout << "Logged out" << endl;
                  state = STATE_START;
                  wndCurrent = wndLogin;
               }
               else
               {
                  cout << "Added new line" << endl;
               }

               break;
            }
            case MSG_TYPE_PLAYER:
            {
               Player p("", "");
               p.deserialize(msg.buffer);
               mapPlayers[p.id] = p;

               cout << "Received MSG_TYPE_PLAYER message" << endl;

               break;
            }
            case MSG_TYPE_CHAT:
            {
               chatConsole.addLine(response);

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

void drawPlayers(map<unsigned int, Player>& mapPlayers, unsigned int curPlayerId)
{
   map<unsigned int, Player>::iterator it;

   for(it = mapPlayers.begin(); it != mapPlayers.end(); it++)
   {
      Player *p = &it->second;

      if (p->id == curPlayerId)
         al_draw_filled_circle(p->pos.x, p->pos.y, 15, al_map_rgb(0, 255, 0));
      else
         al_draw_filled_circle(p->pos.x, p->pos.y, 30, al_map_rgb(255, 0, 0));
   }
}

void registerAccount()
{
   string username = txtUsername->getStr();
   string password = txtPassword->getStr();

   txtUsername->clear();
   txtPassword->clear();

   msgTo.type = MSG_TYPE_REGISTER;

   strcpy(msgTo.buffer, username.c_str());
   strcpy(msgTo.buffer+username.size()+1, password.c_str());

   sendMessage(&msgTo, sock, &server);
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

   sendMessage(&msgTo, sock, &server);
}

void logout()
{
   txtChat->clear();

   msgTo.type = MSG_TYPE_LOGOUT;

   strcpy(msgTo.buffer, username.c_str());

   sendMessage(&msgTo, sock, &server);
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

   sendMessage(&msgTo, sock, &server);
}
