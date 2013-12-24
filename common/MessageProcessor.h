#ifndef _MESSAGE_PROCESSOR_H
#define _MESSAGE_PROCESSOR_H

#include <map>

#include "MessageContainer.h"

using namespace std;

class MessageProcessor {
private:
   int sock;
   ofstream* outputLog;
   int lastUsedId;

   // map from message ids to maps from player addresses to message info
   map<unsigned int, map<unsigned long, MessageContainer> > sentMessages;

   // map from player address to map from message id to time accked
   map<unsigned long, map<unsigned int, unsigned long long> > ackedMessages;

   //unsigned long pid;

public:
   MessageProcessor();
   MessageProcessor(int sock, ofstream* outputLog = NULL);
   ~MessageProcessor();

   int sendMessage(NETWORK_MSG *msg, struct sockaddr_in *dest);
   int receiveMessage(NETWORK_MSG *msg, struct sockaddr_in *source);
   void resendUnackedMessages();
   void cleanAckedMessages();

   map<unsigned int, map<unsigned long, MessageContainer> >& getSentMessages();
   map<unsigned long, map<unsigned int, unsigned long long> >& getAckedMessages();
};

#endif
