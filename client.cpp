// The code below is modified from Beej’s Guide to Network Programming
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <cstring>

#define PORT "33871" // the port client will be connecting to
#define MAXDATASIZE 1000 // max number of bytes we can get at once
// get sockaddr, IPv4 or IPv6:

using namespace std;

int main()
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char hostname[128];

    int getsock_check;
    struct sockaddr_in c;
    socklen_t cLen = sizeof(c);

    string msg, country, user, user_id;
    char *pch;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }    
    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    // retrieve the locally-bound name of the specified socket and store it in the sockaddr structure
    // error checking
    if ((getsock_check = getsockname(sockfd, (struct sockaddr*) &c, &cLen)) == -1) 
    { 
        perror("getsockname"); 
        exit(1);
    }

    freeaddrinfo(servinfo); // all done with this structure

    cout << "Client is up and running" << endl; 
    cout << "Enter Country Name: ";
    cin >> country;
    cout << "Enter user ID: ";
    cin >> user;

    while (1)
    {

        msg = country + "|" + user;

        if (send(sockfd, msg.c_str(), strlen(msg.c_str()), 0) == -1)
        {
            perror("send");
            exit(1);
        }
        else 
            cout << "Client has sent " << country << " and User " <<  user << " to Main Server using TCP over port " << PORT << endl; 

        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
        {
            perror("recv");
            exit(1);
        }
        else
        {
            buf[numbytes] = '\0';

            msg = "";
            if (isdigit(buf[0])) {
                pch = strtok(buf, ", ");
                while (pch != NULL) {
                    user_id = pch;
                    msg += "User " + user_id + ", ";
                    pch = strtok(NULL, ", ");
                }
                msg = msg.substr(0, msg.size()-2);
                cout << msg << " is/are possible friend(s) of User " << user << " in " << country << endl;

            } else if (buf[0] == 'U'){
                cout << "User " << user << ": Not found" << endl;
            } else {
                cout << buf << endl;
            }

            cout << "-----Start a new query-----" << endl;
            cout << "Enter Country Name: ";
            cin >> country;
            cout << "Enter user ID: ";
            cin >> user;
        }

    }

    close(sockfd);
    
    return 0;
}