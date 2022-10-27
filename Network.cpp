#include <thread>
#include <queue>
#include <random>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <array>

#include "Request.h"
#include "Network.h"

using namespace std;

// Constructors
Network::Network(vector<string> node_ips, vector<int> node_port, int my_node_id, bool * applicationRequest, bool * CS_ready, bool * releaseFlag)
{
    this->node_ips = node_ips;
    this->node_ports = node_port;
	this->my_node_id = my_node_id;
	this->applicationRequest = applicationRequest;
	this->CS_ready = CS_ready;
	this->releaseFlag = releaseFlag;
    lastTimeStamp.assign(node_port.size(),-1); // Creates vector with number of nodes, and fills with -1.
	std::cout << "HI" << std::endl;


    // Establish Socket Connections

    //Socket Creation Section

	//Socket variables required
	int master_socket, addrlen, new_socket, activity, i, valread, sd;
	int sock = 0;
	int client_fd = 0;
	int max_sd = 0;
	int num_nodes = node_ips.size();
	struct sockaddr_in address;
	struct sockaddr_in serv_addr;
	int opt = 1;
	char buffer[1025] = { 0 };
	struct hostent * host;

	// Initializing and setting up different variables needed.

	// Just used in a few cases is a way to convert nodeID to socket.
	std::map<int, int> SocketToNodeId;

	// Setting up the listening socket.
	fd_set readfds;

	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(node_ports.at(my_node_id));

	if (bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	printf("Listener on port %d \n", node_ports.at(my_node_id));

	if (listen(master_socket, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	addrlen = sizeof(address);
	puts("Waiting for connections ...");

	// Listening for socket connections up until nodeID. (So if nodeID = 3 with cons 0, 1, 4. Then it will listen for 2 connections.
	// Due to the logic it has to be 0 and 1 that are sent. Order doesn't really matter.

	//Needs to be changed to not use nodeCons.
	while ( sockets.size() < my_node_id ) {
		if ((new_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		sockets.push_back(new_socket);
	}

	puts("Done waiting...");

	sockets.push_back(-1);

	// After having listened for all the lower nodeIds then tries to initialize the connection for all nodeIds greater than it.
	while (sockets.size() < num_nodes) {

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(node_ports.at(sockets.size()));
		host = gethostbyname(node_ips[sockets.size()].c_str());

		memcpy(&serv_addr.sin_addr, host->h_addr_list[0], host->h_length);

		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("\n Socket creation error \n");
			exit(EXIT_FAILURE);
		}

		// Tries connecting. If fails then wait for 300ms and try again.
		if ((client_fd
			= connect(sock, (struct sockaddr*)&serv_addr,
				sizeof(serv_addr)))
			< 0) {
			//printf("%s\n", strerror(errno));
			//printf("\nConnection Failed \n");
			usleep(300000);
		}
		else {
			sockets.push_back(sock);
		}

	}

	puts("Done connecting...");

	puts("Checking all connections...");

	std::string check_send_msg = "0";

	// Sending a test message to all other nodes.
	for (int send_check_conn : sockets){

		if(send_check_conn!=-1){
			send(send_check_conn, check_send_msg.c_str(), strlen(check_send_msg.c_str()), 0);
		}
	
	}

	int recv_check_count = 0;

	// Checking all are recieved. Will only get all recieved if all other nodes have finished doing connections.
	while(recv_check_count < num_nodes){

		if(sockets.at(recv_check_count) != -1){
			valread = read(sockets.at(recv_check_count), buffer, 1024);
		}else{
			recv_check_count++;
			continue;
		}

		buffer[valread] = '\0';

		// Just converting the c_str() from socket into a string.
		std::string temp_recv_check(buffer);

		if(temp_recv_check.at(0) == '0') recv_check_count++;
	
	}

	puts("Checked with all other nodes.");

	//usleep(300000);

	/*for (int socketClose : sockets){

		close(socketClose);

	}*/

}

// 
/**
 * @brief Function to be called for threading. Executes CL Protocol.
 * i.e. thread t1(Network(), params)
 * Will implement Chandry Lamport's Mutual Exclusion Algorithm
 */
void Network::execute_protocol()
{
    int addrlen, activity, valread, sd;
    int max_sd = 0;
    struct sockaddr_in address;
    char buffer[1025] = { 0 };
    struct timeval tv;
    Request my_request;

    // FIXME: May be unnecessary?
	// Just used in a few cases is a way to convert nodeID to socket.
	std::map<int, int> SocketToNodeId;

    fd_set readfds;
    
    while(true)
    {
        // Setup for listening
        FD_ZERO(&readfds);

        tv.tv_sec = 0;
        tv.tv_usec = 5000;

        for (int i = 0; i < sockets.size(); i++) {
            sd = sockets[i];
            FD_SET(sd, &readfds);
            
            if (sd > max_sd)
                max_sd = sd;
        }

        // Listening for responses for 5ms
        activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);

        if ((activity < 0) && (errno != EINTR)) {
            //printf("select error");
        }
        else if (activity == 0) {
            //printf("Time Ran out ");
        }

        // Check every socket for if there is data. (Receive Portion)
        for (int i = 0; i < sockets.size(); i++) {
            sd = sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                if ((valread = read(sd, buffer, 1024)) == 0) {
                    //printf("Closing Socket1: %d\n", sd);
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

                    //printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    //printf("Closing Socket: %d\n", sd);

                    close(sd);
                    sockets.erase(sockets.begin() + i);
                }
                else {
                    if (valread < 0)
                        //printf("Something went wrong! %s\n", strerror(errno));

                    buffer[valread] = '\0';

                    // Just converting the c_str() from socket into a string.
                    string message(buffer);
                    int time_stamp = -1;
                    int node_id = -1;

                    // Message should be structured in a "type_of_msg <time_stamp> <node_id>"
                    string type_of_request = message.substr(0,message.find(' '));
                    message.erase(0,message.find(' ') + 1); // Erase the first token
                    time_stamp = stoi(message.substr(0,message.find(' '))); // Find time_stamp
                    message.erase(0,message.find(' ') + 1); // Erase the second token "<time_stamp>"
                    node_id = stoi(message); // Find node_id
                    
                    // Picks the larger time stamp and adds to it
                    lastTimeStamp.at(my_node_id) = max(time_stamp, lastTimeStamp.at(my_node_id)) + 1;

                    // Check for request, reply, or release message
                    if(type_of_request.compare("request"))
                    {
                        Request req;
                        req.time_stamp = time_stamp;
                        req.node_id = node_id;
                        prioQ.push(req); // Pushes the request onto the priority queue

                        // Sends reply message to node i (the node that the message was sent from)
                        // Also increments and updates this node's time stamp
                        string send_msg = "reply " + to_string(++lastTimeStamp.at(my_node_id)) + ' ' + to_string(my_node_id);
                        send(sockets[i],send_msg.c_str(), strlen(send_msg.c_str()), 0);

                    }
                    // Pops the first request off of the priority queue
                    else if (type_of_request.compare("release"))
                    {
                        prioQ.pop();
                    }
                    else // For reply message
                    {
                        //FIXME: Do nothing (you can remove this!)
                    }

                }
            }
        }//End of receive loop

        if (*applicationRequest)
        {
            my_request.node_id = my_node_id;
            my_request.time_stamp = ++lastTimeStamp.at(my_node_id); // FIXME: Does simply requesting count as an internal event itself? 
                                                                        // excluding the send and receive increments, for now it does count
            prioQ.push(my_request);
            for (int i = 0; i < sockets.size();i++)
            {
                // Ignore sending a message to ourselves (this node)
                if(i == my_node_id)
                {
                    continue;
                }
                // Sends a request message to all other nodes
                string send_msg = "request " + to_string(++lastTimeStamp.at(my_node_id)) + ' ' + to_string(my_node_id); // Increments Lamport's logical clock
                send(sockets.at(i),send_msg.c_str(), strlen(send_msg.c_str()), 0);
            }
            *applicationRequest = false;

        }

        // Check the two conditions of Chandry Lamport's Mutual Exclusion Protocol to allow for access to critical section.

        // First condition: check if my_request is in the priority queue and not in critical section
        if (my_request.compare(prioQ.top()) && !*CS_ready)
        {
            // Check for the second condition
            // If my_requests time stamp is less than all other time stamps, including tie breaker
            for (int i = 0; i < sockets.size();i++)
            {
                if(i == my_node_id) // Ignore own time_stamp
                {
                    continue;
                }

                else if (my_request.time_stamp < lastTimeStamp.at(i))
                {
                    *CS_ready = true; //FIXME: Look for concurrent solution, atomic flags?

                }
                else if (my_request.time_stamp == lastTimeStamp.at(i)) // Tie-breaker case
                {
                    if(my_request.node_id < i) // Compare node_ids, lower one breaks the tie
                    {
                        *CS_ready = true; // FIXME: Look for concurrent solution, atomic flags?

                    }

                }
            }

        } 

        if (*releaseFlag)
        {
            // To send a release message to all other nodes
            for (int i = 0; i < sockets.size();i++)
            {
                // Ignore sending a message to ourselves (this node)
                if(i == my_node_id)
                {
                    continue;
                }
                // Sends a request message to all other nodes
                string send_msg = "release " + to_string(++lastTimeStamp.at(my_node_id)) + ' ' + to_string(my_node_id); // Increments Lamport's logical clock
                send(sockets.at(i),send_msg.c_str(), strlen(send_msg.c_str()), 0);
            }
            prioQ.pop();

            *releaseFlag = false; //FIXME: Make it concurrent/thread-safe
        }

    }//End of while loop
}

/**
 * @brief Prints the contents of a Request Priority Queue
 * 
 * @param prioQ 
 */
void showpq(priority_queue<Request, vector<Request>, prioQ_compare> prioQ)
{
    while (!prioQ.empty())
    {
        printf("Request is: time_stamp = %d, node_id = %d\n", prioQ.top().time_stamp, prioQ.top().node_id);
        prioQ.pop(); // Pops off the prioQ
    }
};
