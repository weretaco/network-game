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
   sentMessages[msg->id] = message;

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
      if (!sentMessages[msg->id].isAcked) {
         cout << "Received new ack" << endl;
         sentMessages[msg->id].isAcked = true;
         sentMessages[msg->id].timeAcked = getCurrentMillis();
      }

      return -1; // don't do any further processing
   }else {
      cout << "Received message" << endl;
      cout << "id: " << msg->id << endl;
      cout << "type: " << msg->type << endl;
      cout << "buffer: " << msg->buffer << endl;

      if (ackedMessages.find(msg->id) != ackedMessages.end()) {
         cout << "Not a duplicate" << endl;

         ackedMessages[msg->id] = getCurrentMillis();

         NETWORK_MSG ack;
         ack.id = msg->id;
         ack.type = MSG_TYPE_ACK;

         sendto(sock, (char*)&ack, sizeof(NETWORK_MSG), 0, (struct sockaddr *)source, sizeof(struct sockaddr_in));
      }else
         cout << "Got duplicate ack" << endl;
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
   map<int, MessageContainer>::iterator it = sentMessages.begin();

   while (it != sentMessages.end()) {
      if (it->second.isAcked) {
//         cout << "Found acked message" << endl;
//         cout << "time acked" << it->second.timeAcked << endl;
//         cout << "cur time" << getCurrentMillis() << endl;
         if ((getCurrentMillis() - it->second.timeAcked) > 1000) {
            cout << "Message was acked. time to delete it" << endl;
            cout << "old map size" << sentMessages.size() << endl;
            sentMessages.erase(it++);
            cout << "new map size" << sentMessages.size() << endl;
         }else
            it++;
      }else
         it++;
   }

   map<unsigned int, unsigned long long>::iterator it2 = ackedMessages.begin();

   while (it2 != ackedMessages.end()) {
      if ((getCurrentMillis() - it2->second) > 5000) {
         ackedMessages.erase(it2++);
         cout << "Deleting ack record" << endl;
      }else
         it2++;
   }
}
