/*
 * client.cpp
 *
 *  Created on: Jun 22, 2014
 *      Author: fatemeh
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "client.h"

Client::Client(string servAddr, int port)
{
    portno = port;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
           if (sockfd < 0)
               error("ERROR opening socket");
           server = gethostbyname(servAddr.c_str());
           if (server == NULL) {
               fprintf(stderr,"ERROR, no such host\n");
               exit(0);
           }
           bzero((char *) &serv_addr, sizeof(serv_addr));
           serv_addr.sin_family = AF_INET;
           bcopy((char *)server->h_addr,
                (char *)&serv_addr.sin_addr.s_addr,
                server->h_length);
           serv_addr.sin_port = htons(portno);

           if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
               error("ERROR connecting");
}
void Client::error(const char *msg)
{
    perror(msg);
    exit(0);
}
void Client::set_message(const char *msg)
{
    int n=write(sockfd,msg,strlen(msg));
//	close(sockfd);
}
const char* Client::get_message()
{
    memset(buffer,0, sizeof buffer);
    n = read(sockfd,buffer,5000);
    if (n < 0)
         error("ERROR reading from socket");
     const char* buf=buffer;
   //  close(newsockfd);
   //  close(sockfd);
     return buf;
}
void Client::s_close()
{
    close(sockfd);
}




