#include "MessageProcessor.h"

#include <iostream>

#include "Common.h"

MessageProcessor::MessageProcessor() {
   lastUsedId = 0;
}

MessageProcessor::~MessageProcessor() {
}

int MessageProcessor::sendMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest) {
   msg->id = ++lastUsedId;
   MessageContainer message(*msg, *dest);
   sentMessages[msg->id][dest->sin_addr.s_addr] = message;

   cout << "Sending message" << endl;
   cout << "id: " << msg->id << endl;
   cout << "type: " << msg->type << endl;
   cout << "buffer: " << msg->buffer << endl;

   int ret =  sendto(sock, (char*)msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)dest, sizeof(struct sockaddr_in));

   cout << "Send a message of type " << msg->type << endl;

   return ret;
}

int MessageProcessor::receiveMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *source) {
   socklen_t socklen = sizeof(struct sockaddr_in);

   // assume we don't care about the value of socklen
   int ret =  recvfrom(sock, (char*)msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)source, &socklen);

   if (ret == -1)
      return ret;

   // add id to the NETWORK_MSG struct
   if (msg->type == MSG_TYPE_ACK) {
      if (!sentMessages[msg->id][source->sin_addr.s_addr].isAcked) {
         cout << "Received new ack" << endl;
         sentMessages[msg->id][source->sin_addr.s_addr].isAcked = true;
         sentMessages[msg->id][source->sin_addr.s_addr].timeAcked = getCurrentMillis();
      }

      return -1; // don't do any further processing
   }else {
      bool isDuplicate = false;

      cout << "Received message" << endl;
      cout << "id: " << msg->id << endl;
      cout << "type: " << msg->type << endl;
      cout << "buffer: " << msg->buffer << endl;

      if (ackedMessages.find(msg->id) == ackedMessages.end())
         isDuplicate = true;

      ackedMessages[msg->id] = getCurrentMillis();

      NETWORK_MSG ack;
      ack.id = msg->id;
      ack.type = MSG_TYPE_ACK;

      sendto(sock, (char*)&ack, sizeof(NETWORK_MSG), 0, (struct sockaddr *)source, sizeof(struct sockaddr_in));

      if (isDuplicate)
         return -1;
   }

   return ret;
}

void MessageProcessor::resendUnackedMessages(int sock) {
   map<int, map<unsigned long, MessageContainer> >::iterator it;
   map<unsigned long, MessageContainer>::iterator it2;
   map<unsigned long, MessageContainer> sentMsg;

   for (it = sentMessages.begin(); it != sentMessages.end(); it++) {
      sentMsg = it->second;
      for (it2 = sentMsg.begin(); it2 != sentMsg.end(); it2++) {
         sendto(sock, (char*)&it2->second.msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)&it2->first, sizeof(struct sockaddr_in));
      }
   }
}

void MessageProcessor::cleanAckedMessages() {
   map<int, map<unsigned long, MessageContainer> >::iterator it = sentMessages.begin();
   map<unsigned long, MessageContainer>::iterator it2;

   while (it != sentMessages.end()) {
      it2 = it->second.begin();
      while (it2 != it->second.begin()) {
         if (it2->second.isAcked) {
            if ((getCurrentMillis() - it2->second.timeAcked) > 1000)
               it->second.erase(it2++);
            else
               it2++;
         }else
            it2++;
      }

      if (it->second.size() == 0)
         sentMessages.erase(it++);
      else
         it++;
   }

   map<unsigned int, unsigned long long>::iterator it3 = ackedMessages.begin();

   while (it3 != ackedMessages.end()) {
      if ((getCurrentMillis() - it3->second) > 500) {
         ackedMessages.erase(it3++);
         cout << "Deleting ack record" << endl;
      }else
         it3++;
   }
}
