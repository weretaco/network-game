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
