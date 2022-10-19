// ConsoleApplication9.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


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
	// File parser
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

			if (fileLine.at(i) != ' ' && fileLine.at(i) != '\n') {
				if (!readingToken) readingToken = true;
				token.push_back(fileLine.at(i));
			}

			if (i == fileLine.size() - 1 && readingToken ) {
				tokens.push_back(token);
				token = "";
				readingToken = false;
			}
		}

		valid = true;

		if (tokens.size() != 0) {
			//Are we checking for Global parameters and are all the tokens valid if so?
			if (stageOfConfig == 0 && tokens.size() == 6) {
				int tokenLen = tokens.size();
				while (tokenLen != 0) {
					tokenLen--;
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
				//Are we checking for nodes and ports and are all the tokens valid if so?
			}
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
				if (valid && numPorts == globalParameters[0]) stageOfConfig = 2;
				//Are we checking for connections and are they within the expected length?
			}
			else if (stageOfConfig == 2 && tokens.size() < globalParameters[0]) {
				int tokenLen = tokens.size();
				for (int tokenI = 0; tokenI < tokenLen; tokenI++) {
					for (int j = 0; j < tokens.at(tokenI).length(); j++) {
						if (int(tokens.at(tokenI).at(j)) < 48 || int(tokens.at(tokenI).at(j)) > 57) {
							valid = false;
						}
					}
					if (valid) {
						nodeConns.push_back(std::stoi(tokens.at(tokenI)));
					}
					else {
						nodeConns.clear();
						break;
					}
				}

				if (valid && nodeConns.size() != 0) {
					systemConns.push_back(nodeConns);
					numConns = numConns + 1;
					nodeConns.clear();
				}

				if (valid && numConns == globalParameters[0]); //Done with Loop;
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
	1 - minPerActive
	2 - maxPerActive
	3 - minSendDelay  (ms)
	4 - snapshotDelay (ms)
	5 - maxNumber
	*/

	/* IP and Ports (Vector) (Vector) (String)

	for each vector of strings (by node ID):

	0 - NodeID
	1 - IP (dcXX)
	2 - Port #
	*/

	/* Connections (Vector) (Vector) (Int)

	for each vector of ints (by node ID):
	
	nodeID connected to.

	*/
	int numNodes = globalParameters[0];
	int minPerActive = globalParameters[1];
	int maxPerActive = globalParameters[2];
	int minSendDelay = globalParameters[3];
	double minSendDelaySecs = minSendDelay / 1000.0;
	int snapshotDelay = globalParameters[4];
	double snapshotDelaySecs = snapshotDelay / 1000.0;
	int maxNumber = globalParameters[5];

	std::vector<std::string> nodeIps;
	std::vector<int> nodePorts;
	
	for (int ipOfNodes = 0; ipOfNodes < numNodes; ipOfNodes++) {
		std::string intermediateIP = ipPorts.at(ipOfNodes).at(1) + ".utdallas.edu";
		nodeIps.push_back(intermediateIP);
		nodePorts.push_back(std::stoi(ipPorts.at(ipOfNodes).at(2)));
	}

	std::vector<int> sockets;

	std::vector<std::vector<int>> nodeCons;

	for (int consOfNodes = 0; consOfNodes < numNodes; consOfNodes++) {
		nodeCons.push_back(systemConns.at(consOfNodes));
	}

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

	// just count for number of application messages sent and if the node is active or not.
	int sentMsgs = 0;
	bool isActive = false;

	std::chrono::time_point<std::chrono::system_clock> start, end;
	std::chrono::time_point<std::chrono::system_clock> startSnap, endSnap;

	// Setting up snapShotting. Bool for if snapShotting and creating 2d vector of all the snapShots sent to node 0.
	bool snapShotting = false;

	int snapShotsRecv = 0;
	std::vector<std::string> snapShots;
	for (int i = 0; i < numNodes; i++) {
		snapShots.push_back("");
	}
	std::vector<std::vector<int>> snapShotsInt;

	bool mapTerminated = false;

	srand(time(NULL));

	int activeMsgCount = (rand() % (maxPerActive-minPerActive+1)) + minPerActive;

	// Long and probably unncessarily complex way of getting the node id.

	char hostname[HOST_NAME_MAX + 1];

	gethostname(hostname, HOST_NAME_MAX + 1);

	int node = 0;

	for (int gettingNodeID = 0; gettingNodeID < numNodes; gettingNodeID++) {
		std::cout << ipPorts.at(gettingNodeID).at(1) << std::endl;
		if (ipPorts.at(gettingNodeID).at(1).at(2) == hostname[2] && ipPorts.at(gettingNodeID).at(1).at(3) == hostname[3]) {
			node = gettingNodeID;
			break;
		}
	}

	// Initilizaing the Vector clocks and retrivedMsgs which is for a node to check they have recieved a marker message from every conn.
	std::vector<int> vectorClock(numNodes, 0);

	std::vector<int> vectorClockSnap(numNodes, 0);

	std::vector<bool> retrivedMsg(nodeCons.at(node).size(), false);

	int retrivedMsgCount = nodeCons.at(node).size();

	std::ofstream outputSnapFile;

	std::string fileName = "config-" + std::to_string(node) + ".out";

	outputSnapFile.open(fileName);

	//Basic min spanning tree. 
	/*
		Roughly something like: (Using example)
		0 -> 1
		0 -> 4
		1 -> 2
		1 -> 3

		parentList would look like:
		0, 0, 1, 1, 0

		when snapshot process for a process is done the process sends it's snapshot to its parent
		which sends it to its parent and so on till it reaches 0. With how the it is set up there is no
		way for node 0 to not be the root as node 0 starts the tree and the next nodes that are checked are the children of 0 or the children of the children of 0.

	*/
	std::vector<int> parentList(numNodes, -1);

	parentList.at(0) = 0;

	std::string markerMessage = "m";
	markerMessage = markerMessage + std::to_string(node);

	//More effiecent ways of doing this, but one time call and simple.
	bool parentListFilled = false;
	while (!parentListFilled) {
		parentListFilled = true;
		for (int i = 0; i < parentList.size(); i++) {
			if (parentList.at(i) != -1) {
				for (int j = 0; j < nodeCons.at(i).size(); j++) {
					if (parentList.at(nodeCons.at(i).at(j)) == -1) {
						parentList.at(nodeCons.at(i).at(j)) = i;
					}
				}
			}
		}

		for (int i = 0; i < parentList.size(); i++) {
			if (parentList.at(i) == -1) {
				parentListFilled = false;
				break;
			}
		}
	}

	if (node == 0) {
		isActive = true;
	}

	// Just used in a few cases is a way to convert nodeID to socket.
	std::map<int, int> nodeIdToSocket;

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

	while (nodeCons[node].size() > sockets.size() && nodeCons[node][sockets.size()] < node) {
		if ((new_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		sockets.push_back(new_socket);
	}

	puts("Done waiting...");

	// After having listened for all the lower nodeIds then tries to initialize the connection for all nodeIds greater than it.
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
	startSnap = std::chrono::system_clock::now();

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

		// Snapshot start detection.
		// If active and it has been more than delay then send message to random connection and check if sent the random number of messages.

		endSnap = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds_snap = endSnap - startSnap;

		if (isActive && elapsed_seconds.count() > minSendDelaySecs) {
			start = std::chrono::system_clock::now();
			int nodeMsg = rand() % (sockets.size());

			vectorClock.at(node) = vectorClock.at(node) + 1;

			std::string sentVectorClock = vectorToString(vectorClock);

			send(sockets[nodeMsg], sentVectorClock.c_str(), strlen(sentVectorClock.c_str()), 0);
			
			sentMsgs = sentMsgs + 1;

			//printf("Sent message to %d. Messages Sent: %d. Elapsed Time: %f. Delay: %f.\n", nodeCons.at(node).at(nodeMsg), sentMsgs, elapsed_seconds.count(), minSendDelaySecs);

			activeMsgCount = activeMsgCount - 1;

			//printf("Current vector clock: %s\n\n", vectorToString(vectorClock).c_str());

			if (activeMsgCount == 0) {
				isActive = false;
				printf("No longer active. \n");
			}
		}else if (!snapShotting && node == 0 && elapsed_seconds_snap.count() > snapshotDelaySecs) {
			snapShotting = true;
			
			printf("Staring Snapshot Protocol\n");

			//Send marker message to each connected socket.
			for (int i = 0; i < sockets.size(); i++) {
				send(sockets[i], markerMessage.c_str(), strlen(markerMessage.c_str()), 0);
				//printf("Sent marker message to %d. Elapsed Time: %f.\n", nodeCons.at(node).at(i), elapsed_seconds_snap.count());
			}

			for (int vectIt = 0; vectIt < vectorClock.size(); vectIt++) {
				vectorClockSnap.at(vectIt) = vectorClock.at(vectIt);
			}

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

					std::string sentVectorClock(buffer);

					//If a message does not start with m or s then it is a application message.
					if (sentVectorClock.at(0) != 'm' && sentVectorClock.at(0) != 's') {

						// If there is an application message recv. and still have messages to send.
						if (!isActive&&sentMsgs < maxNumber) {
							isActive = true;
							activeMsgCount = rand() % (maxPerActive - minPerActive + 1) + minPerActive;
						}

						//printf("Sent vector clock:    %s\n", sentVectorClock.c_str());

						vectorClock.at(node) = vectorClock.at(node) + 1;

						// Checking if it was an in-transit message between marker messages
						if (!retrivedMsg.at(i)) vectorClockSnap.at(node) = vectorClockSnap.at(node) + 1;

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
					//If m and not snapshotting the snapshotting process should start.
					else if (sentVectorClock.at(0) == 'm' && !snapShotting) {
						snapShotting = true;

						std::string markedMessage = sentVectorClock;

						int startSnapNode = std::stoi(markedMessage.substr(1));

						retrivedMsg.at(i) = true;
						
						retrivedMsgCount = retrivedMsgCount - 1;

						//printf("Recived marker message from %d.\n", startSnapNode);

						for (int vectIt = 0; vectIt < vectorClock.size(); vectIt++) {
							vectorClockSnap.at(vectIt) = vectorClock.at(vectIt);
						}

						//Send marker message to each connected socket.

						for (int snapSock = 0; snapSock < nodeCons.at(node).size(); snapSock++) {
							//if (nodeCons.at(node).at(snapSock) != startSnapNode) {

								send(sockets[snapSock], markerMessage.c_str(), strlen(markerMessage.c_str()), 0);
								//printf("Sent marker message to %d.\n", nodeCons.at(node).at(snapSock));
							//}
						}

					} // If snapShotting and marker message then just mark as having recieved from this socket.
					else if (sentVectorClock.at(0) == 'm' && snapShotting && !retrivedMsg.at(i)) {

						std::string markedMessage = sentVectorClock;

						int startSnapNode = std::stoi(markedMessage.substr(1));

						//printf("Recived marker message from %d.\n", startSnapNode);

						retrivedMsg.at(i) = true;

						retrivedMsgCount = retrivedMsgCount - 1;
					}
					// Checking for snap shotting.
					else if (sentVectorClock.at(0) == 's' && node != 0) {
						std::string sentSnapInfo = sentVectorClock;

						send(sockets[nodeIdToSocket.at(parentList.at(node))], sentSnapInfo.c_str(), strlen(sentSnapInfo.c_str()), 0);
						//printf("Recived snapshot message from %d.\n", std::stoi(sentSnapInfo.substr(1, sentSnapInfo.find(' ') - 1)));

						/*if (sentSnapInfo.find('s') == sentSnapInfo.find_last_of('s')) {
							if (!retrivedMsg.at(i)) {
								//printf("Decrementing\n");
								retrivedMsg.at(i) = true;
								retrivedMsgCount = retrivedMsgCount - 1;
							}
						}
						else {
							while (sentSnapInfo.find('s') != sentSnapInfo.find_last_of('s')) {
								if (!retrivedMsg.at(nodeIdToSocket.at(std::stoi(sentSnapInfo.substr(sentSnapInfo.find('s')+1, sentSnapInfo.find('|')))))) {
									//printf("Decrementing\n");
									retrivedMsg.at(nodeIdToSocket.at(std::stoi(sentSnapInfo.substr(sentSnapInfo.find('s') + 1, sentSnapInfo.find('|'))))) = true;
									retrivedMsgCount = retrivedMsgCount - 1;
								}
								sentSnapInfo = sentSnapInfo.substr(sentSnapInfo.substr(sentSnapInfo.find('s')+1).find('s'));
							}
						}*/
					}
					// If snapShot and this is node 0 then store the information.
					else if (sentVectorClock.at(0) == 's' && node == 0) {
						std::string sentSnapInfo = sentVectorClock;
						if (sentSnapInfo.find('s') == sentSnapInfo.find_last_of('s')) {
							snapShots.at(std::stoi(sentSnapInfo.substr(1, sentSnapInfo.find(' ') - 1))) = sentSnapInfo.substr(sentSnapInfo.find(' ') + 1);
							snapShotsRecv = snapShotsRecv + 1;
							//printf("Recived snapshot message from %d. %s\n", std::stoi(sentSnapInfo.substr(1, sentSnapInfo.find(' ') - 1)), sentSnapInfo.c_str());
						}
						else {
							// Condition in-case multiple snapshot messages are sent before being read. Just parses through them.
							while (sentSnapInfo.find('s') != sentSnapInfo.find_last_of('s')) {
								//s3|0|0|1|1|1s13
								//0|0|1|1|1s13
								//0|0|1|1|1
								snapShots.at(std::stoi(sentSnapInfo.substr(1, sentSnapInfo.find(' ') - 1))) = sentSnapInfo.substr(sentSnapInfo.find(' ') + 1, sentSnapInfo.substr(1).find('s')+1);
								snapShotsRecv = snapShotsRecv + 1;
								//printf("Recived snapshot message from %d. %s\n", std::stoi(sentSnapInfo.substr(1, sentSnapInfo.find(' ') - 1)), sentSnapInfo.c_str());
								sentSnapInfo = sentSnapInfo.substr(sentSnapInfo.substr(1).find('s')+1);
							}
							snapShots.at(std::stoi(sentSnapInfo.substr(1, sentSnapInfo.find(' ') - 1))) = sentSnapInfo.substr(sentSnapInfo.find(' ') + 1);
							snapShotsRecv = snapShotsRecv + 1;
							//printf("Recived snapshot message from %d. %s\n", std::stoi(sentSnapInfo.substr(1, sentSnapInfo.find(' ') - 1)), sentSnapInfo.c_str());
						}
					}
					// All marker messages have been recieved
					if (retrivedMsgCount == 0 && node != 0) {

						snapShotting = false;

						retrivedMsgCount = nodeCons.at(node).size();

						for (int retReset = 0; retReset < retrivedMsg.size(); retReset++) {
							retrivedMsg.at(retReset) = false;
						}

						std::string sendVectorClock = vectorToString(vectorClockSnap);

						outputSnapFile << sendVectorClock << std::endl;

						int isClockDiff = 0;

						for (int checkVectorDiff = 0; checkVectorDiff < vectorClock.size(); checkVectorDiff++) {
							if (vectorClock.at(checkVectorDiff) != vectorClockSnap.at(checkVectorDiff)) {
								isClockDiff = 1;
								break;
							}
						}

						if (isActive) {
							sendVectorClock = "s" + std::to_string(node) + " 1 " + std::to_string(isClockDiff) + " " + sendVectorClock;
						}
						else {
							sendVectorClock = "s" + std::to_string(node) + " 0 " + std::to_string(isClockDiff) + " " + sendVectorClock;
						}

						send(sockets[nodeIdToSocket.at(parentList.at(node))], sendVectorClock.c_str(), strlen(sendVectorClock.c_str()), 0);
						//printf("Snapshot Finished. Sent to: %d.\n", parentList.at(node));

					}
					else if (retrivedMsgCount == 0 && node == 0) {

						snapShotting = false;

						startSnap = std::chrono::system_clock::now();

						retrivedMsgCount = nodeCons.at(node).size();

						for (int retReset = 0; retReset < retrivedMsg.size(); retReset++) {
							retrivedMsg.at(retReset) = false;
						}

						std::string sendVectorClock = vectorToString(vectorClockSnap);

						outputSnapFile << sendVectorClock << std::endl;

						int isClockDiff = 0;

						for (int checkVectorDiff = 0; checkVectorDiff < vectorClock.size(); checkVectorDiff++) {
							if (vectorClock.at(checkVectorDiff) != vectorClockSnap.at(checkVectorDiff)) {
								isClockDiff = 1;
								break;
							}
						}

						if (isActive) {
							sendVectorClock = "1 " + std::to_string(isClockDiff) + " " + sendVectorClock;
						}
						else {
							sendVectorClock = "0 " + std::to_string(isClockDiff) + " " + sendVectorClock;
						}

						snapShots.at(0) = sendVectorClock;
						snapShotsRecv = snapShotsRecv + 1;
						//printf("Snapshot Finished. Sent to: %d. Value: %s.\n", parentList.at(node), sendVectorClock.c_str());
					}

				}
			}
		}//End of recieve loop

		if (snapShotsRecv == numNodes) {
			snapShotsRecv = 0;

			//mapTerminated = true;

			/*
			Logic for checking if process has terminated:
				First: Check if every process is passive. If not then just continue.
				Second: Check if every channel state is empty.
					How to do that?
						Wait for a second snapshot that all processes are passive? (Could have messages in channel states...)
						Maybe send difference bool if difference in current vector and snap vector? IF there is a difference then there were messages in channel state.
			*/

			bool testTerminate = true;

			for (int snapI = 0; snapI < numNodes; snapI++) {

				//If the process is still active then map protocol has not terminated.
				if (snapShots.at(snapI).at(0) == '1') {
					testTerminate = false;
					break;
				}

				//If the process has registered messages in channel state then map protocl has not terminated.
				if (snapShots.at(snapI).at(2) == '1') {
					testTerminate = false;
					break;
				}
			}

			if (testTerminate) {
				mapTerminated = true;
				printf("The map protocol has terminated.\n");
			}

			/*
			Logic for proving that the snapshot is consistent:
				Show that every local state is concurrent:
					Basically check that the value of vectorClock[i][i] >= vectorClock[j][i] for any value j where j is 0 -> numNodes-1, where i is 0 -> numNodes-1;
			*/

			//Note need to creat snapShotsInt and stringToVector
			for (int snapI = 0; snapI < numNodes; snapI++) {

				std::vector <int> tempSnapVect = stringToVector(snapShots.at(snapI));

				snapShotsInt.push_back(tempSnapVect);

			}

			bool isConsistent = true;

			/*for (int snapI = 0; snapI < numNodes; snapI++) {
				for (int snapJ = 0; snapJ < numNodes; snapJ++) {
					printf(" %d", snapShotsInt.at(snapJ).at(snapI));
				}
				printf("\n");
			}*/

			for (int snapI = 0; snapI < numNodes; snapI++) {
				for (int snapJ = 0; snapJ < numNodes; snapJ++) {
					if (snapShotsInt.at(snapI).at(snapI) < snapShotsInt.at(snapJ).at(snapI)) {
						isConsistent = false;
						//printf("ERROR AT: %d %d Values: %d %d\n", snapI, snapJ, snapShotsInt.at(snapI).at(snapI), snapShots.at(snapJ).at(snapI));
					}
					//printf(" %d", snapShotsInt.at(snapJ).at(snapI));
				}
				//printf("\n");
			}

			if (isConsistent) {
				printf("The snapshots are consistent.\n");
			}
			else {
				printf("Error the snapshots are not consistent.\n");
			}
			
			snapShotsInt.clear();
		}

	}//End of while loop

	outputSnapFile.close();

	//printf("Hello message sent\n");
	//printf("Hostname: %s\n", hostname);
	//printf("NodeID: %d\n", node);
	return 0;
}