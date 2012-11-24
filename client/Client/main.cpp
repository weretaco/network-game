#include "../../common/compiler.h"

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
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <vector>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#include "../../common/message.h"

#ifdef WINDOWS
	#pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

void initWinSock();
void shutdownWinSock();
void error(const char *);

const float FPS = 60;
const int SCREEN_W = 640;
const int SCREEN_H = 480;
const int BOUNCER_SIZE = 32;
enum MYKEYS {
   KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT
};
 
int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display = NULL;
   ALLEGRO_EVENT_QUEUE *event_queue = NULL;
   ALLEGRO_TIMER *timer = NULL;
   ALLEGRO_BITMAP *bouncer = NULL;
   float bouncer_x = SCREEN_W / 2.0 - BOUNCER_SIZE / 2.0;
   float bouncer_y = SCREEN_H / 2.0 - BOUNCER_SIZE / 2.0;
   bool key[4] = { false, false, false, false };
   bool redraw = true;
   bool doexit = false;

   vector<string> vctChat;
   string strPrompt;
   string strEnteredInput;

   if(!al_init()) {
      fprintf(stderr, "failed to initialize allegro!\n");
      return -1;
   }

   al_init_font_addon();
   al_init_ttf_addon();

   ALLEGRO_FONT *font = al_load_ttf_font("../pirulen.ttf", 12, 0);
 
   if (!font) {
      fprintf(stderr, "Could not load 'pirulen.ttf'.\n");
      getchar();
	  return -1;
   }
 
   if(!al_install_keyboard()) {
      fprintf(stderr, "failed to initialize the keyboard!\n");
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
 
   al_clear_to_color(al_map_rgb(0,0,0));
 
   al_flip_display();

   int sock, n;
   struct sockaddr_in server, from;
   struct hostent *hp;
   char buffer[256];
   NETWORK_MSG msgTo, msgFrom;

   if (argc != 3) {
      cout << "Usage: server port" << endl;
      exit(1);
   }

   initWinSock();
	
   sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0)
      error("socket");

   server.sin_family = AF_INET;
   hp = gethostbyname(argv[1]);
   if (hp==0)
      error("Unknown host");

   memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);
   server.sin_port = htons(atoi(argv[2]));

   strcpy(msgTo.buffer, "Hello");
   n=sendMessage(&msgTo, sock, &server);
   if (n < 0)
      error("sendMessage");

   n = receiveMessage(&msgFrom, sock, &from);
   if (n < 0)
      error("receiveMessage");
	
   vctChat.push_back(string(msgFrom.buffer));

   al_start_timer(timer);
 
   while(!doexit)
   {
      ALLEGRO_EVENT ev;
      al_wait_for_event(event_queue, &ev);
 
      if(ev.type == ALLEGRO_EVENT_TIMER) {
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

			case ALLEGRO_KEY_ENTER:
			   strEnteredInput = strPrompt;
			   strPrompt.clear();

			   if (strEnteredInput.compare("q") == 0) {
			      doexit = true;
		       }
			   else
			   {
                  strcpy(msgTo.buffer, strEnteredInput.c_str());
		          n=sendMessage(&msgTo, sock, &server);
		          if (n < 0)
			         error("sendMessage");

		          n = receiveMessage(&msgFrom, sock, &from);
		          if (n < 0)
			         error("receiveMessage");

		          vctChat.push_back(string(msgFrom.buffer));
			   }

			   break;
         }

		 if(ALLEGRO_KEY_A <= ev.keyboard.keycode && ev.keyboard.keycode <= ALLEGRO_KEY_Z)
		 {
            char newChar = 'a'+ev.keyboard.keycode-ALLEGRO_KEY_A;
            strPrompt.append(1, newChar);
		 }

		 if(ALLEGRO_KEY_0 <= ev.keyboard.keycode && ev.keyboard.keycode <= ALLEGRO_KEY_9)
		 {
            char newChar = '0'+ev.keyboard.keycode-ALLEGRO_KEY_0;
            strPrompt.append(1, newChar);
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
 
      if(redraw && al_is_event_queue_empty(event_queue)) {
         redraw = false;
 
         al_clear_to_color(al_map_rgb(0,0,0));
 
         al_draw_bitmap(bouncer, bouncer_x, bouncer_y, 0);

		 for(int x=0; x<vctChat.size(); x++)
            al_draw_text(font, al_map_rgb(255,255,255), 10, 10+x*15, ALLEGRO_ALIGN_LEFT, vctChat[x].c_str());

		 al_draw_text(font, al_map_rgb(255,255,255), 10, 460, ALLEGRO_ALIGN_LEFT, strPrompt.c_str());
         
         al_flip_display();
      }
   }

   #if defined WINDOWS
      closesocket(sock);
   #elif defined LINUX
      close(sock);
   #endif

   shutdownWinSock();
   
   al_destroy_event_queue(event_queue);
   al_destroy_bitmap(bouncer);
   al_destroy_display(display);
   al_destroy_timer(timer);
 
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

// need to make a function like this that works on windows
void error(const char *msg)
{
   perror(msg);
   shutdownWinSock();
   exit(1);
}
