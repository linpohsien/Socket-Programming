// The code below is modified from Beej’s Guide to Network Programming
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <vector>
#include <set>
#include <unordered_map>

#define SERVERA_PORT "30871"	// the port users will be connecting to
#define MAINSERVER_PORT "32871"
#define MAXBUFLEN 1000

using namespace std;

int main()
{
	int sockfd;
	struct addrinfo hints, *servAinfo, *mainservinfo, *p1, *p2;
	int rv;
	int numbytes;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	struct sockaddr_in servermain_addr;
	char s[INET6_ADDRSTRLEN];

    FILE* file = fopen("dataA.txt", "r");	
	char line[256];
	string country, country_list, user_id, user_id_list;
	unordered_map<string, string> country_map;
	unordered_map<string, int> country_user_map;
	unordered_map<string, string>::iterator iter;
	set<string> user_id_set;
	char *id_tmp, user[MAXBUFLEN], user_list[MAXBUFLEN];
	set<string> id_set_tmp;
	unordered_map<string, string> country_friend_map;
	string friend_id;
	string send_msg, recv_msg;
	char *pch;
    set<string> friend_set; 	

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;


	if ((rv = getaddrinfo("127.0.0.1", MAINSERVER_PORT, &hints, &mainservinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p1 = mainservinfo; p1 != NULL; p1 = p1->ai_next)
		break;

	if (p1 == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}


	if ((rv = getaddrinfo("127.0.0.1", SERVERA_PORT, &hints, &servAinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p2 = servAinfo; p2 != NULL; p2 = p2->ai_next) {
		if ((sockfd = socket(p2->ai_family, p2->ai_socktype,
				p2->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
		if (bind(sockfd, p2->ai_addr, p2->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}
		break;
	}

	if (p2 == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servAinfo);

	cout << "Server A is up and running using UDP on port " << SERVERA_PORT << endl;


    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (isdigit(line[0])) {
			friend_id = line;
			id_tmp = strtok(line, " ");
			while (id_tmp != NULL) {
				iter = country_friend_map.find(country + "|" + id_tmp);
				if (iter != country_friend_map.end()) {
					country_friend_map[country + "|" + id_tmp].append(" " + friend_id);
				} else {
					country_friend_map[country + "|" + id_tmp] = friend_id;
				}
				id_tmp = strtok(NULL, " ");
			}
        } else if (isalpha(line[0])) {
			country = line;
			country_list.append(country+" ");
        }
	}

	addr_len = p1->ai_addrlen;

	if (recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		p1->ai_addr, &addr_len) == -1) {
		perror("recvfrom");
		exit(1);
	}

	if (sendto(sockfd, country_list.c_str(), strlen(country_list.c_str()), 0,
		p1->ai_addr, p1->ai_addrlen) == -1) {
		perror("serverA: sendto");
		exit(1);
	}	

	cout << "Server A has sent a country list to Main Server" << endl;


    while(1) {
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
            p1->ai_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';
		recv_msg = buf;

		pch = strtok(buf, "|");
		string country(pch);
		pch = strtok(NULL, "|");
		string user_id(pch);

		cout << "Server A has received a request for finding possible friends of User " << user_id << " in " << country << endl;
        iter = country_friend_map.find(recv_msg);

		if (iter != country_friend_map.end()) {

			strcpy(user_list, const_cast<char*>(iter->second.c_str()));

			pch = strtok(user_list, " ");
			while (pch != NULL) {
				friend_set.insert(pch);
				pch = strtok(NULL, " ");
			}
            
			send_msg = "";
			for (const auto &s : friend_set) {
				if (s != user_id)
				    send_msg += s + ", ";
			}
			if (send_msg.length() > 2) {
				send_msg = send_msg.substr(0, send_msg.size()-2);
			}

			if (friend_set.size() > 1) {

				if (sendto(sockfd, send_msg.c_str(), strlen(send_msg.c_str()), 0,
					p1->ai_addr, p1->ai_addrlen) == -1) {
					perror("serverA: sendto");
					exit(1);
				}

                cout << "Server A found the following possible friends for User " << user_id << " in " << country << ": " << send_msg << endl;;			
				cout << "Server A has sent the result to Main Server" << endl;
			} else {

                send_msg = "None";
				if (sendto(sockfd, send_msg.c_str(), strlen(send_msg.c_str()), 0,
					p1->ai_addr, p1->ai_addrlen) == -1) {
					perror("serverA: sendto");
					exit(1);
				}

                cout << "Server A found the following possible friends for User " << user_id << " in " << country << ": None" << endl;;			
				cout << "Server A has sent the result to Main Server" << endl;
			}			

		} else {
			send_msg = "User " + user_id + " not found";
			if (sendto(sockfd, send_msg.c_str(), strlen(send_msg.c_str()), 0,
				p1->ai_addr, p1->ai_addrlen) == -1) {
				perror("serverA: sendto");
				exit(1);
			}			
			cout << "User " << user_id << " does not show up in " << country << endl;
			cout << "Server A has sent \"User " << user_id << " not found\" to Main Server" << endl;
		}
    }

	freeaddrinfo(mainservinfo);
	close(sockfd);

	return 0;
}