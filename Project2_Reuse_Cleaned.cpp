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
#include <chrono>  
#include <map>
#include <fstream>
#include <array>


int main()
{
	// File parser. Probs include argument for file name/path.
	std::ifstream configReader("config.txt");

	if (!configReader.is_open()) {
		std::cout << "Cannot find file.";
		return 0;
	}

	std::vector<std::string> tokens;

	int stageOfConfig = 0;
	bool readingToken = false;
	std::string token;
	int numPorts = 0;
	bool valid = true;

	/*
	1 - NumNodes
	2 - interRequestDelay
	3 - csExeTime
	4 - maxRequests
	*/

	std::array<int, 4> globalParameters;
	globalParameters.fill(0);

	std::vector<std::vector<std::string>> ipPorts;
	std::vector<std::string> ipPort;

	std::string fileLine;

	while (std::getline(configReader, fileLine)) {

		fileLine.pop_back();

		if (fileLine.size() == 0) continue;

		//Tokenize the given line.
		for (int i = 0; i < fileLine.size(); i++) {
			// If reading a token and a space is hit then must be end of token.
			if (fileLine.at(i) == ' ' && readingToken) {
				tokens.push_back(token);
				token = "";
				readingToken = false;
			}

			//If a # character is found then just save last token and exit.
			if (fileLine.at(i) == '#' && readingToken) {
				tokens.push_back(token);
				token = "";
				readingToken = false;
				break;
			}
			else if (fileLine.at(i) == '#') {
				token = "";
				readingToken = false;
				break;
			}

			// If character that is not a newline then
			// if not currently readingToken. Start readingtoken.
			// push the char into a string that will be the token.
			if (fileLine.at(i) != ' ' && fileLine.at(i) != '\n') {
				if (!readingToken) readingToken = true;
				token.push_back(fileLine.at(i));
			}

			// If end of line then store token.
			if (i == fileLine.size() - 1 && readingToken ) {
				tokens.push_back(token);
				token = "";
				readingToken = false;
			}
		}

		valid = true;

		if (tokens.size() != 0) {
			//Are we checking for Global parameters and are all the tokens valid if so?
			if (stageOfConfig == 0 && tokens.size() == globalParameters.size()) {
				int tokenLen = tokens.size();
				while (tokenLen != 0) {
					tokenLen--;
					// are the charcters in the token digits.
					for (int j = 0; j < tokens.at(tokenLen).length(); j++) {
						if (int(tokens.at(tokenLen).at(j)) < 48 || int(tokens.at(tokenLen).at(j)) > 57) {
							valid = false;
							break;
						}
					}

					if (valid) {
						globalParameters[tokenLen] = std::stoi(tokens.at(tokenLen));
					}
					else {
						globalParameters.fill(0);
						break;
					}
				}
				if (valid) stageOfConfig = 1;
			}
			//Are we checking for nodes and ports and are all the tokens valid if so?
			else if (stageOfConfig == 1 && tokens.size() == 3) {
				int tokenLen = tokens.size();
				for (int tokenI = 0; tokenI < tokenLen; tokenI++) {
					//Check if the second token and if second token follows the format of "dc##"
					if (tokenI == 1 && tokens.at(1).size() == 4 && tokens.at(1).substr(0, 2).compare("dc") == 0) {
						if (int(tokens.at(tokenI).at(2)) < 48 || int(tokens.at(tokenI).at(2)) > 57) {
							valid = false;
						}
						if (int(tokens.at(tokenI).at(3)) < 48 || int(tokens.at(tokenI).at(3)) > 57) {
							valid = false;
						}
					}
					else {
						//Are all the chars in the token an int? If not then invalid token.
						for (int j = 0; j < token.length(); j++) {
							if (int(tokens.at(tokenI).at(j)) < 48 || int(tokens.at(tokenI).at(j)) > 57) {
								valid = false;
								break;
							}
						}
					}
					if (valid) {
						ipPort.push_back(tokens.at(tokenI));
					}
					else {
						ipPort.clear();
						break;
					}
				}
				//If valid 3 tokens for node# ip port.
				if (valid && ipPort.size() != 0) {
					ipPorts.push_back(ipPort);
					numPorts = numPorts + 1;
					ipPort.clear();
				}
			}
		}
		tokens.clear();
		token = "";
		valid = true;

	}

	configReader.close();

	//Just given paramaters:
	/* Global Parameters (Vector) (Ints) -
	0 - NumNodes
	1 - interRequestDelay
	2 - csExeTime
	3 - maxRequests
	*/

	/* IP and Ports (Vector) (Vector) (String)

	for each vector of strings (by node ID):

	0 - NodeID
	1 - IP (dcXX)
	2 - Port #
	*/

	int numNodes = globalParameters[0];
	int interRequestDelay = globalParameters[1];
	int csExeTime = globalParameters[2];
	int maxRequests = globalParameters[3];

	// Just setup for later chrono stuff. Needs to be edited to use the random value created from the mean global Parameters.
	double interRequestDelaySecs = interRequestDelay / 1000.0;
	double csExeTimeSecs = csExeTime / 1000.0;

	// Just getting all the ports into a simplier array and attaching .utdallas.edu to the end of the given dcXX.
	// Can and probably should be refactored to be part of the parser or smth.
	std::vector<std::string> nodeIps;
	std::vector<int> nodePorts;
	
	for (int ipOfNodes = 0; ipOfNodes < numNodes; ipOfNodes++) {
		std::string intermediateIP = ipPorts.at(ipOfNodes).at(1) + ".utdallas.edu";
		nodeIps.push_back(intermediateIP);
		nodePorts.push_back(std::stoi(ipPorts.at(ipOfNodes).at(2)));
	}

	std::vector<int> sockets;

	//Socket variables required
	int master_socket, addrlen, new_socket, activity, i, valread, sd;
	int sock = 0;
	int client_fd = 0;
	int max_sd = 0;
	struct sockaddr_in address;
	struct sockaddr_in serv_addr;
	int opt = 1;
	char buffer[1025] = { 0 };
	struct timeval tv;
	struct hostent * host;

	// Initializing and setting up different variables needed.

	std::chrono::time_point<std::chrono::system_clock> start, end;

	// Long and probably unncessarily complex way of getting the node id.

	char hostname[HOST_NAME_MAX + 1];

	gethostname(hostname, HOST_NAME_MAX + 1);

	int node = 0;

	// Basically just going through the ip addresses and finding where for dcXX and dcXX.utdallas.edu the XX is the same. Could be refactored.
	// Since index of vector = node ID we can just use index.
	for (int gettingNodeID = 0; gettingNodeID < numNodes; gettingNodeID++) {
		std::cout << ipPorts.at(gettingNodeID).at(1) << std::endl;
		if (ipPorts.at(gettingNodeID).at(1).at(2) == hostname[2] && ipPorts.at(gettingNodeID).at(1).at(3) == hostname[3]) {
			node = gettingNodeID;
			break;
		}
	}

	// Just used in a few cases is a way to convert nodeID to socket.
	std::map<int, int> nodeIdToSocket;

	// Needs to be edited to fit a completed graph.
	// This really just needs to be looked completely over.
	// smth like nodeIdToSocket key = the ith connection nodeid for current node connections. Value = i.
	for (int i = 0; i < nodeCons.at(node).size(); i++) {
		nodeIdToSocket[nodeCons.at(node).at(i)] = i;
	}

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
	address.sin_port = htons(nodePorts.at(node));

	if (bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	printf("Listener on port %d \n", nodePorts.at(node));

	if (listen(master_socket, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	addrlen = sizeof(address);
	puts("Waiting for connections ...");

	// Listening for socket connections up until nodeID. (So if nodeID = 3 with cons 0, 1, 4. Then it will listen for 2 connections.
	// Due to the logic it has to be 0 and 1 that are sent. Order doesn't really matter.

	//Needs to be changed to not use nodeCons.
	while (nodeCons[node].size() > sockets.size() && nodeCons[node][sockets.size()] < node) {
		if ((new_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		sockets.push_back(new_socket);
	}

	puts("Done waiting...");

	// After having listened for all the lower nodeIds then tries to initialize the connection for all nodeIds greater than it.
	// Needs to be changed to not use nodecons.
	while (sockets.size() < nodeCons[node].size()) {

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(nodePorts.at(nodeCons.at(node).at(sockets.size())));
		host = gethostbyname(nodeIps[nodeCons[node][sockets.size()]].c_str());

		memcpy(&serv_addr.sin_addr, host->h_addr_list[0], host->h_length);

		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("\n Socket creation error \n");
			return -1;
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

	start = std::chrono::system_clock::now();

	// The main loop
	while (1 == 1) {

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

		end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;

		if (isActive && elapsed_seconds.count() > minSendDelaySecs) {
			start = std::chrono::system_clock::now();

			send(sockets[nodeMsg], sentVectorClock.c_str(), strlen(sentVectorClock.c_str()), 0);
			
			sentMsgs = sentMsgs + 1;

			//printf("Sent message to %d. Messages Sent: %d. Elapsed Time: %f. Delay: %f.\n", nodeCons.at(node).at(nodeMsg), sentMsgs, elapsed_seconds.count(), minSendDelaySecs);

			//printf("Current vector clock: %s\n\n", vectorToString(vectorClock).c_str());

		}

		// Check every socket for if there is data.
		for (int i = 0; i < sockets.size(); i++) {
			sd = sockets[i];

			if (FD_ISSET(sd, &readfds)) {

				printf("Reading Message from %d\n", nodeCons.at(node).at(i));
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
					std::string sentVectorClock(buffer);

					//If a message does not start with m or s then it is a application message.
					if (sentVectorClock.at(0) != 'm' && sentVectorClock.at(0) != 's') {

						//printf("Sent vector clock:    %s\n", sentVectorClock.c_str());

						// Incrementing Clock
						vectorClock.at(node) = vectorClock.at(node) + 1;

						// Comparing sent VectorClock to self VectorClock and taking max.
						for (int j = 0; j < vectorClock.size(); j++) {
							int givenClockValue = 0;
							if (j < vectorClock.size() - 1) {
								givenClockValue = std::stoi(sentVectorClock.substr(0, sentVectorClock.find(" ")));
								sentVectorClock.erase(0, sentVectorClock.find(" ") + 1);
							}
							else {
								givenClockValue = std::stoi(sentVectorClock);
							}
							vectorClock.at(j) = (vectorClock.at(j) < givenClockValue) ? givenClockValue : vectorClock.at(j);
						}
						//printf("Current vector clock: %s\n\n", vectorToString(vectorClock).c_str());
					}

				}
			}
		}//End of recieve loop

	}//End of while loop

	outputSnapFile.close();

	//printf("Hello message sent\n");
	//printf("Hostname: %s\n", hostname);
	//printf("NodeID: %d\n", node);
	return 0;
}