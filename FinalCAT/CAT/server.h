#ifndef CAT_Server_h
#define CAT_Server_h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

class Server
{

public:
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[5001];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     Server(int port_num);
     const char* get_message();
     void error(const char *msg);
     void set_message(const char *string);
     void s_close();
};

#endif

