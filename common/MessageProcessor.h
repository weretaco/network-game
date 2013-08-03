#ifndef _MESSAGE_PROCESSOR_H
#define _MESSAGE_PROCESSOR_H

#include <map>

#include "MessageContainer.h"

using namespace std;

class MessageProcessor {
private:
   int lastUsedId;

   // map from message ids to maps from player addresses to message info
   map<unsigned int, map<unsigned long, MessageContainer> > sentMessages;

   // map from player address to map from message id to time accked
   map<unsigned long, map<unsigned int, unsigned long long> > ackedMessages;

   unsigned long pid;

public:
   MessageProcessor();
   ~MessageProcessor();

   int sendMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest, ofstream* outputLog = NULL);
   int receiveMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest, ofstream* outputLog = NULL);
   void resendUnackedMessages(int sock, ofstream* outputLog = NULL);
   void cleanAckedMessages(ofstream* outputLog = NULL);

   map<unsigned int, map<unsigned long, MessageContainer> >& getSentMessages();
   map<unsigned long, map<unsigned int, unsigned long long> >& getAckedMessages();
};

#endif
