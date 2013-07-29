#include "MessageContainer.h"

#include <iostream>

#include "Compiler.h"

using namespace std;

MessageContainer::MessageContainer() {
}

MessageContainer::MessageContainer(const MessageContainer& mc) {
   this->msg = mc.msg;
   this->clientAddr = mc.clientAddr;
   this->isAcked = mc.isAcked;
   this->timeAcked = mc.timeAcked;
}

MessageContainer::MessageContainer(NETWORK_MSG msg, struct sockaddr_in clientAddr) {
   this->clientAddr = clientAddr;
   this->msg = msg;
   this->isAcked = false;
   this->timeAcked = 0;
}

MessageContainer::~MessageContainer() {
}

bool MessageContainer::getAcked() {
   return this->isAcked;
}

unsigned long long MessageContainer::getTimeAcked() {
   return this->timeAcked;
}

NETWORK_MSG* MessageContainer::getMessage() {
   return &msg;
}

void MessageContainer::setAcked(bool acked) {
   cout << "acked before: " << this->isAcked << endl;
   this->isAcked = acked;
   cout << "acked after: " << this->isAcked << endl;
}

void MessageContainer::setTimeAcked(unsigned long long time) {
   this->timeAcked = time;
}

/*
string getMsgTypeString(int msgType) {
   switch(msgType) {
      case MSG_TYPE_ACK: return "MSG_TYPE_ACK";
      case MSG_TYPE_REGISTER: return "MSG_TYPE_REGISTER";
      case MSG_TYPE_LOGIN: return "MSG_TYPE_LOGIN";
      case MSG_TYPE_LOGOUT: return "MSG_TYPE_LOGOUT";
      case MSG_TYPE_CHAT: return "MSG_TYPE_CHAT";
      case MSG_TYPE_PLAYER: return "MSG_TYPE_PLAYER";
      case MSG_TYPE_PLAYER_MOVE: return "MSG_TYPE_PLAYER_MOVE";
      case MSG_TYPE_OBJECT: return "MSG_TYPE_OBJECT";
      case MSG_TYPE_REMOVE_OBJECT: return "MSG_TYPE_REMOVE_OBJECT";
      case MSG_TYPE_PICKUP_FLAG: return "MSG_TYPE_PICKUP_FLAG";
      caseMSG_TYPE_DROP_FLAG: return "MSG_TYPE_DROP_FLAG";
      case MSG_TYPE_SCORE: return "MSG_TYPE_SCORE";
      case MSG_TYPE_START_ATTACK: return "MSG_TYPE_START_ATACK";
      case MSG_TYPE_ATTACK: return "MSG_TYPE_ATTACK";
      case MSG_TYPE_PROJECTILE: return "MSG_TYPE_PROJECTILE";
      case MSG_TYPE_REMOVE_PROJECTILE: return "MSG_TYPE_REMOVE_PROJECTILE";
      default: return "Unknown";
   }
}
*/
