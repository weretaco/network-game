#include "MessageProcessor.h"

int MessageProcessor::sendMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest) {
   return 0;
}

int MessageProcessor::receiveMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest) {
   return 0;
}

void MessageProcessor::resendUnackedMessages() {
}

void MessageProcessor::cleanAckedMessages() {
}
