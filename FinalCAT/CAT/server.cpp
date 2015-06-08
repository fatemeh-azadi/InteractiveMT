/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include "server.h"

using namespace std;
Server::Server(int port_num)
{
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = port_num;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
              error("ERROR on binding vvv");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
     if (newsockfd < 0)
         error("ERROR on accept");


}

const char* Server::get_message()
{
    bzero(buffer,5001);
     n = read(newsockfd,buffer,5000);
     if (n < 0) error("ERROR reading from socket");
    // printf("Here is the message: %s\n",buffer);
    // n = write(newsockfd,"I got your message",18);
   //  if (n < 0) error("ERROR writing to socket");
     const char* buf=buffer;

     return buf;
}
void Server::set_message(const char *msg)
{
    int k = strlen(msg);
    if(!k)
        msg = "#EMPTY";


    int n=write(newsockfd,msg,strlen(msg));
    if (n < 0) error("ERROR writing to socket");
//    cout << n << endl;
//	close(sockfd);
}

void Server::s_close()
{
    close(newsockfd);
    close(sockfd);
}

void Server::error(const char *msg)
{
    close(newsockfd);
    close(sockfd);
    perror(msg);
    exit(1);
}
