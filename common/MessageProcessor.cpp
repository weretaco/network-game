#include "MessageProcessor.h"

#include <iostream>
#include <fstream>

#include "Common.h"

MessageProcessor::MessageProcessor() {
   lastUsedId = 0;
}

MessageProcessor::~MessageProcessor() {
}

int MessageProcessor::sendMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest, ofstream* outputLog) {
   cout << "Sending message of type " << msg->type << endl;

   msg->id = ++lastUsedId;
   MessageContainer message(*msg, *dest);

   if (outputLog)
      (*outputLog) << "Sending message (id " << msg->id << ") of type " << MessageContainer::getMsgTypeString(msg->type) << endl;

   sentMessages[msg->id][dest->sin_addr.s_addr] = message;

   int ret =  sendto(sock, (char*)msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)dest, sizeof(struct sockaddr_in));

   return ret;
}

int MessageProcessor::receiveMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *source, ofstream* outputLog) {
   socklen_t socklen = sizeof(struct sockaddr_in);

   // assume we don't care about the value of socklen
   int ret =  recvfrom(sock, (char*)msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)source, &socklen);

   if (ret == -1)
      return ret;

   // add id to the NETWORK_MSG struct
   if (msg->type == MSG_TYPE_ACK) {
      if (!sentMessages[msg->id][source->sin_addr.s_addr].getAcked()) {
         sentMessages[msg->id][source->sin_addr.s_addr].setAcked(true);
         sentMessages[msg->id][source->sin_addr.s_addr].setTimeAcked(getCurrentMillis());
         if (outputLog)
            (*outputLog) << "Received ack for message id " << msg->id << endl;
      }

      return -1; // don't do any further processing
   }else {
      bool isDuplicate = false;
      map<unsigned int, unsigned long long>& ackedPlayerMessages = ackedMessages[source->sin_addr.s_addr];

      if (ackedPlayerMessages.find(msg->id) != ackedPlayerMessages.end()) {
         isDuplicate = true;
         cout << "Got duplicate of type " << msg->type << endl;
         if (outputLog)
            (*outputLog) << "Received duplicate (id " << msg->id << ") of type " << MessageContainer::getMsgTypeString(msg->type) << endl;
      }else {
         cout << "Got message of type " << msg->type << endl;
         if (outputLog)
            (*outputLog) << "Received message (id " << msg->id << ") of type " << MessageContainer::getMsgTypeString(msg->type) << endl;
      }

      ackedPlayerMessages[msg->id] = getCurrentMillis();

      NETWORK_MSG ack;
      ack.id = msg->id;
      ack.type = MSG_TYPE_ACK;

      sendto(sock, (char*)&ack, sizeof(NETWORK_MSG), 0, (struct sockaddr *)source, sizeof(struct sockaddr_in));

      if (isDuplicate)
         return -1;
   }

   return ret;
}

void MessageProcessor::resendUnackedMessages(int sock, ofstream* outputLog) {
   map<unsigned int, map<unsigned long, MessageContainer> >::iterator it;
   map<unsigned long, MessageContainer>::iterator it2;
   map<unsigned long, MessageContainer> sentMsg;

   for (it = sentMessages.begin(); it != sentMessages.end(); it++) {
      sentMsg = it->second;
      for (it2 = sentMsg.begin(); it2 != sentMsg.end(); it2++) {
         if (!(it2->second.getAcked())) {
            sendto(sock, (const char*)it2->second.getMessage(), sizeof(NETWORK_MSG), 0, (struct sockaddr *)&it2->first, sizeof(struct sockaddr_in));
         }
      }
   }
}

void MessageProcessor::cleanAckedMessages(ofstream* outputLog) {
   map<unsigned int, map<unsigned long, MessageContainer> >::iterator it = sentMessages.begin();
   map<unsigned long, MessageContainer>::iterator it2;

   while (it != sentMessages.end()) {
      it2 = it->second.begin();
      while (it2 != it->second.end()) {
         if (it2->second.getAcked()) {
            if ((getCurrentMillis() - it2->second.getTimeAcked()) > 1000) {
               if (outputLog)
                  (*outputLog) << "Removing id " << it2->second.getMessage()->id << " from the acked record" << endl;
               it->second.erase(it2++);
            }else
               it2++;
         }else
            it2++;
      }

      if (it->second.size() == 0)
         sentMessages.erase(it++);
      else
         it++;
   }

   map<unsigned long, map<unsigned int, unsigned long long> >::iterator it3 = ackedMessages.begin();
   map<unsigned int, unsigned long long>::iterator it4;

   // somehow want to delete the inner map once that player logs out
   while (it3 != ackedMessages.end()) {
      it4 = it3->second.begin();
      while (it4 != it3->second.end()) {
         if ((getCurrentMillis() - it4->second) > 500)
            it3->second.erase(it4++);
         else
            it4++;
      }
      it3++;
   }
}

map<unsigned int, map<unsigned long, MessageContainer> >& MessageProcessor::getSentMessages() {
   return this->sentMessages;
}

map<unsigned long, map<unsigned int, unsigned long long> >& MessageProcessor::getAckedMessages() {
   return this->ackedMessages;
}
