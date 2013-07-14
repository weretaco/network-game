#include "MessageProcessor.h"

#include <iostream>

MessageProcessor::MessageProcessor() {
   lastUsedId = 0;
}

MessageProcessor::~MessageProcessor() {
}

int MessageProcessor::sendMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest) {
   MessageContainer message(*msg, *dest);
   message.id = ++lastUsedId;
   sentMessages[message.id] = message;

   int ret =  sendto(sock, (char*)msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)dest, sizeof(struct sockaddr_in));

   cout << "Send a message of type " << msg->type << endl;

   return ret;
}

int MessageProcessor::receiveMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *source) {
   socklen_t socklen = sizeof(struct sockaddr_in);

   // assume we don't care about the value of socklen
   int ret =  recvfrom(sock, (char*)msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)source, &socklen);

   // add id to the NETWORK_MSG struct
   if (msg->type == MSG_TYPE_ACK) {
      sentMessages.erase(msg->id);
   }else {
      NETWORK_MSG ack;
      ack.id = msg->id;

      sendto(sock, (char*)&ack, sizeof(NETWORK_MSG), 0, (struct sockaddr *)source, sizeof(struct sockaddr_in));
   }

   return ret;
}

void MessageProcessor::resendUnackedMessages(int sock) {
   map<int, MessageContainer>::iterator it;

   for(it = sentMessages.begin(); it != sentMessages.end(); it++) {
      sendto(sock, (char*)&it->second.msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)&it->second.clientAddr, sizeof(struct sockaddr_in));
   }
}

void MessageProcessor::cleanAckedMessages() {
   // shouldn't be needed since I can just remove messages when I get their ACKs
}
