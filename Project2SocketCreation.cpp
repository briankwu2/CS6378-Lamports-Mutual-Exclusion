
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

int main(int argc, char** argv)
{

	// Checking that there is only one argument
	if(argc != 2){
		std::cout << "Wrong number of arguments." << std::endl;
		return 0;
	}

	// Config file parser.
	/*

	Input: Command line argument string that is the config file name.

	Output:
	
	global_parameters
	Array size 4 of ints.
	1 - Number of Nodes
	2 - interRequestDelay
	3 - csExeTime
	4 - maxRequests

	node_ips
	node_ports
	Vectors of size # nodes of strings and ints respectively.
	index = node ID.

	node
	Int of process's node ID.
	*/
	std::ifstream configReader(argv[1]);

	if (!configReader.is_open()) {
		std::cout << "Cannot find file." << std::endl;
		return 0;
	}

	// Variables for parser.
	std::vector<std::string> tokens;
	int stageOfConfig = 0;
	bool readingToken = false;
	std::string token = "";
	bool valid = true;
	std::vector<std::string> ipPort;
	bool nodeIDCheckFlag = true;

	char hostname[HOST_NAME_MAX + 1];
	gethostname(hostname, HOST_NAME_MAX + 1);

	// Process's Node ID.
	int node = 0;

	/*
	1 - Number of Nodes
	2 - interRequestDelay
	3 - csExeTime
	4 - maxRequests
	*/
	std::array<int, 4> global_parameters;
	global_parameters.fill(0);

	// Index = node id, [nodeID] = ip (dcXX.utdallas.edu) or port #.
	std::vector<std::string> node_ips;
	std::vector<int> node_ports;

	std::string fileLine;

	while (std::getline(configReader, fileLine)) {
		
		//std::cout << fileLine << std::endl;

		if (fileLine.size() == 0) continue;

		// Checking for nextLine character b/c for some reason just '\n' doesn't work.
		if (fileLine.back() == 13){
			fileLine.pop_back();
		}

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
			if (fileLine.at(i) != ' ' && fileLine.at(i) != 13) {
				if (!readingToken) readingToken = true;
				token.push_back(fileLine.at(i));
			}

			// If end of line then store token.
			if (i == fileLine.size() - 1 && readingToken) {
				tokens.push_back(token);
				token = "";
				readingToken = false;
			}
			
			if(tokens.size() > 4){
				break;
			}

		}

		valid = true;

		if (tokens.size() != 0) {
			//Are we checking for Global parameters and are all the tokens valid if so?
			if (stageOfConfig == 0 && tokens.size() == 4) {
				int tokenLen = tokens.size();
				while (tokenLen != 0) {
					tokenLen--;
					// are the charcters in the token digits.
					for (int j = 0; j < tokens.at(tokenLen).length(); j++) {
						if (!std::isdigit(tokens.at(tokenLen).at(j))) {
							valid = false;
							break;
						}
					}

					if (valid) {
						global_parameters[tokenLen] = std::stoi(tokens.at(tokenLen));
					}
					else {
						global_parameters.fill(0);
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
						if (!std::isdigit(tokens.at(tokenI).at(2))) {
							valid = false;
						}
						if (!std::isdigit(tokens.at(tokenI).at(3))) {
							valid = false;
						}
					}
					else {
						//Are all the chars in the token an int? If not then invalid token.
						for (int j = 0; j < token.length(); j++) {
							if (!std::isdigit(tokens.at(tokenI).at(j))) {
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
					node_ips.push_back(ipPort.at(1).append(".utdallas.edu"));
					node_ports.push_back(std::stoi(ipPort.at(2)));
					if (nodeIDCheckFlag && ipPort.at(1).at(2) == hostname[2] && ipPort.at(1).at(3) == hostname[3]){
						node = node_ips.size()-1;
						nodeIDCheckFlag = false;
					}
					ipPort.clear();
				}
			}
		}
		tokens.clear();
		token = "";
		valid = true;

	}

	if(stageOfConfig == 0){
		std::cout << "ERROR: Global Parameters not properly formated." << std::endl;
		return 0;
	}

	if(global_parameters[0] != node_ips.size()){
		std::cout << "ERROR: Not enough ips given as number of nodes or not properly formated." << std::endl;
		return 0;
	}

	//Getting rid of ipPort and tokens.
	std::vector<std::string>().swap(ipPort);
	std::vector<std::string>().swap(tokens);

	/*for (int gloParm : global_parameters){

		std::cout << gloParm << " ";

	}

	std::cout << std::endl;

	for(int i = 0; i < global_parameters[0] ; i++){
	
		std::cout << node_ips.at(i) << " " << node_ports.at(i) << std::endl;

	}*/

	std :: cout << "NodeID: " << node << std::endl;

	configReader.close();

	/*


	Socket Creation Section

	*/
			
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
	address.sin_port = htons(node_ports.at(node));

	if (bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	printf("Listener on port %d \n", node_ports.at(node));

	if (listen(master_socket, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	addrlen = sizeof(address);
	puts("Waiting for connections ...");

	// Listening for socket connections up until nodeID. (So if nodeID = 3 with cons 0, 1, 4. Then it will listen for 2 connections.
	// Due to the logic it has to be 0 and 1 that are sent. Order doesn't really matter.

	//Needs to be changed to not use nodeCons.
	while ( sockets.size() < node ) {
		if ((new_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		sockets.push_back(new_socket);
	}

	puts("Done waiting...");

	sockets.push_back(-1);

	// After having listened for all the lower nodeIds then tries to initialize the connection for all nodeIds greater than it.
	while (sockets.size() < global_parameters[0]) {

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(node_ports.at(sockets.size()));
		host = gethostbyname(node_ips[sockets.size()].c_str());

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
	while(recv_check_count < global_parameters[0]){

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

	for (int socketClose : sockets){

		close(socketClose);

	}


}