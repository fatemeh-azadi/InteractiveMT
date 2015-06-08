/*
 * client.h
 *
 *  Created on: Jun 22, 2014
 *      Author: fatemeh
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
using namespace std;

class Client
{

public:
     int sockfd, portno, n;
     struct sockaddr_in serv_addr;
     struct hostent *server;
     Client(string servAddr, int port);
     char buffer[5001];
     void set_message(const char *string);
     const char* get_message();
     void error(const char *msg);
     void s_close();

};




#endif /* CLIENT_H_ */
