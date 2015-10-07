#include "MessageContainer.h"

#include <iostream>

using namespace std;

MessageContainer::MessageContainer() {
}

MessageContainer::MessageContainer(const MessageContainer& mc) {
   this->msg = mc.msg;
   this->clientAddr = mc.clientAddr;
   this->timeSent = mc.timeSent;
   this->isAcked = mc.isAcked;
   this->timeAcked = mc.timeAcked;
}

MessageContainer::MessageContainer(NETWORK_MSG msg, struct sockaddr_in clientAddr) {
   this->clientAddr = clientAddr;
   this->msg = msg;
   this->timeSent = 0;
   this->isAcked = false;
   this->timeAcked = 0;
}

MessageContainer::~MessageContainer() {
}

NETWORK_MSG* MessageContainer::getMessage() {
   return &msg;
}

bool MessageContainer::getAcked() {
   return this->isAcked;
}

unsigned long long MessageContainer::getTimeSent() {
   return this->timeSent;
}

unsigned long long MessageContainer::getTimeAcked() {
   return this->timeAcked;
}

void MessageContainer::setAcked(bool acked) {
   this->isAcked = acked;
}

void MessageContainer::setTimeSent(unsigned long long time) {
   this->timeSent = time;
}

void MessageContainer::setTimeAcked(unsigned long long time) {
   this->timeAcked = time;
}

string MessageContainer::getMsgTypeString(int msgType) {
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
      case MSG_TYPE_DROP_FLAG: return "MSG_TYPE_DROP_FLAG";
      case MSG_TYPE_SCORE: return "MSG_TYPE_SCORE";
      case MSG_TYPE_ATTACK: return "MSG_TYPE_ATTACK";
      case MSG_TYPE_PROJECTILE: return "MSG_TYPE_PROJECTILE";
      case MSG_TYPE_REMOVE_PROJECTILE: return "MSG_TYPE_REMOVE_PROJECTILE";
      case MSG_TYPE_CREATE_GAME: return "MSG_TYPE_CREATE_GAME";
      case MSG_TYPE_JOIN_GAME: return "MSG_TYPE_JOIN_GAME";
      case MSG_TYPE_LEAVE_GAME: return "MSG_TYPE_LEAVE_GAME";
      case MSG_TYPE_GAME_INFO: return "MSG_TYPE_GAME_INFO";
      case MSG_TYPE_JOIN_GAME_SUCCESS: return "MSG_TYPE_JOIN_GAME_SUCCESS";
      case MSG_TYPE_JOIN_GAME_FAILURE: return "MSG_TYPE_JOIN_GAME_FAILURE";
      case MSG_TYPE_JOIN_GAME_ACK: return "MSG_TYPE_JOIN_GAME_ACK";
      case MSG_TYPE_PLAYER_JOIN_GAME: return "MSG_TYPE_PLAYER_JOIN_GAME";
      case MSG_TYPE_FINISH_GAME: return "MSG_TYPE_FINISH_GAME";
      default: return "Unknown";
   }
}
