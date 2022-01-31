#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "single-pong.h"

int Socket_creation();
void Socket_identification(int);
void Send_Reply_server(int, struct message_server *, struct sockaddr_in*);
void Send_Reply_client(int, struct message_client *, struct sockaddr_in*);
void Receive_message_server(int, struct message_client *, struct sockaddr_in*);
void Receive_message_client(int, struct message_server *, struct sockaddr_in*);
