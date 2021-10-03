// The code below is modified from Beej’s Guide to Network Programming
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

#define SERVERA_PORT "30871"	// the port users will be connecting to
#define SERVERB_PORT "31871"
#define MAINSERVER_PORT_UDP "32871"
#define MAINSERVER_PORT_TCP "33871" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 1000 // max number of bytes we can get at once
#define MAXBUFLEN 1000

using namespace std;

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int main(void)
{
    int sockfd_tcp, sockfd_udp, new_fd, numbytes; // listen on sock_fd, new connection on new_fd
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servAinfo, *servBinfo, *mainservinfo_tcp, *mainservinfo_udp, *p0, *p1, *p2, *p3;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
	socklen_t addrA_len;
	socklen_t addrB_len;

    char line[256];
    int country_count = 0;
    int server_id;
    vector<int> server_list;
    vector<int> country_count_list;
    set<string> country_set;  
    map<string, int> country_server_map;
    map<string, int>::iterator iter;
    int client_id = 0;
    string send_msg = "send";
    char *pch, *pchA, *pchB;
	char bufA[MAXBUFLEN];
	char bufB[MAXBUFLEN];
    vector<string> country_listA, country_listB;
    set<string> friend_set; 

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("127.0.0.1", MAINSERVER_PORT_TCP, &hints, &mainservinfo_tcp)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for (p0 = mainservinfo_tcp; p0 != NULL; p0 = p0->ai_next) {
        if ((sockfd_tcp = socket(p0->ai_family, p0->ai_socktype, p0->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd_tcp, p0->ai_addr, p0->ai_addrlen) == -1) {
            close(sockfd_tcp);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(mainservinfo_tcp); // all done with this structure

    if (p0 == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd_tcp, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }



	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo("127.0.0.1", MAINSERVER_PORT_UDP, &hints, &mainservinfo_udp)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p1 = mainservinfo_udp; p1 != NULL; p1 = p1->ai_next) {
		if ((sockfd_udp = socket(p1->ai_family, p1->ai_socktype,
				p1->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		if (bind(sockfd_udp, p1->ai_addr, p1->ai_addrlen) == -1) {
			close(sockfd_udp);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p1 == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	freeaddrinfo(mainservinfo_udp);

	if ((rv = getaddrinfo("127.0.0.1", SERVERA_PORT, &hints, &servAinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p2 = servAinfo; p2 != NULL; p2 = p2->ai_next)
		break;

	if (p2 == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	if ((rv = getaddrinfo("127.0.0.1", SERVERB_PORT, &hints, &servBinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p3 = servBinfo; p3 != NULL; p3 = p3->ai_next)
		break;

	if (p3 == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}    

    cout << "Main server is up and running" << endl;

	addrA_len = p2->ai_addrlen;
	addrB_len = p3->ai_addrlen;

	if (sendto(sockfd_udp, send_msg.c_str(), strlen(send_msg.c_str()), 0,
	p2->ai_addr, p2->ai_addrlen) == -1) {
		perror("sendto");
		exit(1);
	}

	if ((numbytes = recvfrom(sockfd_udp, bufA, MAXBUFLEN-1 , 0,
		p2->ai_addr, &addrA_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	bufA[numbytes] = '\0';
	cout << "Main server has received the country list from server A using UDP over port " << SERVERA_PORT << endl;

	if (sendto(sockfd_udp, send_msg.c_str(), strlen(send_msg.c_str()), 0,
	p3->ai_addr, p3->ai_addrlen) == -1) {
		perror("sendto");
		exit(1);
	}

	if ((numbytes = recvfrom(sockfd_udp, bufB, MAXBUFLEN-1 , 0,
		p3->ai_addr, &addrB_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	bufB[numbytes] = '\0';
	cout << "Main server has received the country list from server B using UDP over port " << SERVERB_PORT << endl;


	cout << "Server A" << endl;
	pchA = strtok(bufA, " ");
	while (pchA != NULL) {
		string country(pchA);
		cout << country << endl;
		country_listA.push_back(country);
		pchA = strtok(NULL, " ");
	}

	cout << "Server B" << endl;
	pchB = strtok(bufB, " ");
	while (pchB != NULL) {
		string country(pchB);
		cout << country << endl;
		country_listB.push_back(country);
		pchB = strtok(NULL, " ");
	}

    while (1)
    { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd_tcp, (struct sockaddr *)&their_addr, &sin_size);
        client_id++;

        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        if (!fork())
        { // this is the child process
	        while (1) 
            {
                if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1)
                    perror("recv");
                buf[numbytes] = '\0';
                send_msg = buf;

                pch = strtok(buf, "|");
                string country(pch);
                pch = strtok(NULL, "|");
                string user_id(pch);

                cout << "Main server has received the request on User " << user_id << " in " << country;
                cout << " from client " << client_id << " using TCP over port " << MAINSERVER_PORT_TCP << endl;

                if (find(country_listA.begin(), country_listA.end(), country) != country_listA.end()) {

                    cout << country << " shows up in server A" << endl;
                    if (sendto(sockfd_udp, send_msg.c_str(), strlen(send_msg.c_str()), 0,
                    p2->ai_addr, p2->ai_addrlen) == -1) {
                        perror("servermain: sendto");
                        exit(1);
                    }
                    
                    cout << "Main Server has sent request of User " << user_id << " to server A using UDP over port " << MAINSERVER_PORT_UDP << endl;
                    if ((numbytes = recvfrom(sockfd_udp, bufA, MAXBUFLEN-1 , 0,
                        p2->ai_addr, &addrA_len)) == -1) {
                        perror("recvfrom");
                        exit(1);
                    }
                    bufA[numbytes] = '\0';

                    if (bufA[0] != 'U' && bufA[0] != 'N') {                        
                        cout << "Main server has received searching result of User " << user_id << " from server A" << endl;
                        if (send(new_fd, bufA, strlen(bufA), 0) == -1)
                            perror("send");                        
                        cout << "Main Server has sent message to client " << client_id << " using TCP over " << MAINSERVER_PORT_TCP << endl;
                    } else if (bufA[0] == 'N') {
                        cout << "Main server has received searching result of User " << user_id << " from server A" << endl;
                        if (send(new_fd, bufA, strlen(bufA), 0) == -1)
                            perror("send");                        
                        cout << "Main Server has sent message to client " << client_id << " using TCP over " << MAINSERVER_PORT_TCP << endl;
                    } else {
                        cout << "Main server has received \"User " << user_id << ": Not found\" from server A" << endl;
                        if (send(new_fd, bufA, strlen(bufA), 0) == -1)
                            perror("send");                              
                        cout << "Main Server has sent message to client " << client_id << " using TCP over " << MAINSERVER_PORT_TCP << endl;
                    }
                }
                else if (find(country_listB.begin(), country_listB.end(), country) != country_listB.end()) {

                    cout << country << " shows up in server B" << endl;
                    if (sendto(sockfd_udp, send_msg.c_str(), strlen(send_msg.c_str()), 0,
                    p3->ai_addr, p3->ai_addrlen) == -1) {
                        perror("servermain: sendto");
                        exit(1);
                    }
                    
                    cout << "Main Server has sent request of User " << user_id << " to server B using UDP over port " << MAINSERVER_PORT_UDP << endl;
                    if ((numbytes = recvfrom(sockfd_udp, bufB, MAXBUFLEN-1 , 0,
                        p3->ai_addr, &addrB_len)) == -1) {
                        perror("recvfrom");
                        exit(1);
                    }
                    bufB[numbytes] = '\0';

                    if (bufB[0] != 'U' && bufB[0] != 'N') {                        
                        cout << "Main server has received searching result of User " << user_id << " from server B" << endl;
                        if (send(new_fd, bufB, strlen(bufB), 0) == -1)
                            perror("send");
                        cout << "Main Server has sent message to client " << client_id << " using TCP over " << MAINSERVER_PORT_TCP << endl;
                    } else if (bufB[0] == 'N') {
                        cout << "Main server has received searching result of User " << user_id << " from server B" << endl;
                        if (send(new_fd, bufB, strlen(bufB), 0) == -1)
                            perror("send");                        
                        cout << "Main Server has sent message to client " << client_id << " using TCP over " << MAINSERVER_PORT_TCP << endl;
                    } else {
                        cout << "Main server has received \"User " << user_id << ": Not found\" from server B" << endl;
                        if (send(new_fd, bufB, strlen(bufB), 0) == -1)
                            perror("send");                              
                        cout << "Main Server has sent message to client " << client_id << " using TCP over " << MAINSERVER_PORT_TCP << endl;
                    }
                }
                else {
                    send_msg = country + ": Not found";
                    if (send(new_fd, send_msg.c_str(), strlen(send_msg.c_str()), 0) == -1)
                        perror("send");                    
                    cout << country << " does not show up in server A&B" << endl;
                    cout << "Main Server has sent \"" << country << ": Not found\" to client " << client_id << " using TCP over port " << MAINSERVER_PORT_TCP << endl;
                }                
            }

            close(new_fd);
            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }

	freeaddrinfo(servAinfo);
	freeaddrinfo(servBinfo);
	close(sockfd_tcp);
    close(sockfd_udp);
    return 0;
}