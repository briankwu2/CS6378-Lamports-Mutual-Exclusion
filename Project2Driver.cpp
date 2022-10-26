// Project2Parser.cpp : This file contains the 'main' function. Program execution begins and ends there.
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
#include <thread>
#include <functional>

#include "Request.h"
#include "Network.h"

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

	bool* cs_enter = new bool;
	bool* cs_ready = new bool;
	bool* cs_leave = new bool;

	*cs_enter = false;
	*cs_ready = false;
	*cs_leave = false;

  	std::cout << "Creating object." << std::endl;

	Network net1(node_ips, node_ports, node, cs_enter, cs_ready, cs_leave);

  	std::cout << "Created object." << std::endl;

	std::thread t1(std::bind(&Network::netty, net1));

	std::cout << "Created thread." << std::endl;

	srand(time(NULL));

	// Calls Parser or is given the values

	//double interRequestDelay = 20.0 / 1000.0;
	//double csExeTime = 10.0 / 1000.0;
	//int maxRequests = 10;

	double interRequestDelay = global_parameters[1]/1000.0;
	double csExeTime = global_parameters[2]/1000.0;
	int maxRequests = global_parameters[3];

	double csExeTimeRand = 0.0;
	double interRequestDelayRand = 0.0;

	//gsl_rng* r = gsl_rng_alloc (gsl_rng_taus);
	//gsl_rng_set(r, time(NULL));

	std::chrono::time_point<std::chrono::system_clock> start_request, end_request;
	std::chrono::time_point<std::chrono::system_clock> start_cs, end_cs;

	while (maxRequests > 0) {
		//interRequestDelayRand = gsl_ran_exponential(r, interRequestDelay);
		//csExeTimeRand = gsl_ran_exponential(r, csExeTime);		
		//rand() % (sockets.size());

		csExeTimeRand = csExeTime + ((double)(rand() % 50)) / 1000.0;
		interRequestDelayRand = interRequestDelay + ((double)(rand() % 50)) / 1000.0;

		std::cout << "Trying request." << std::endl;
		maxRequests = maxRequests - 1;

		*cs_enter = true;

		while (!*cs_ready) { usleep(300); }

		start_cs = std::chrono::system_clock::now();
		end_cs = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds_cs = end_cs - start_cs;

		while (elapsed_seconds_cs.count() < csExeTimeRand) {
			usleep(100);
			end_cs = std::chrono::system_clock::now();
			elapsed_seconds_cs = end_cs - start_cs;
		}
		std::cout << "Time executing: " << elapsed_seconds_cs.count() << std::endl;
		std::cout << "Time should execute: " << csExeTimeRand << std::endl;
		std::cout << "Done executing." << std::endl;

		*cs_leave = true;

		start_request = std::chrono::system_clock::now();
		end_request = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds_request = end_request - start_request;

		while (elapsed_seconds_request.count() < interRequestDelayRand) {
			usleep(100);
			end_request = std::chrono::system_clock::now();
			elapsed_seconds_request = end_request - start_request;
		}
		std::cout << "Time waited: " << elapsed_seconds_request.count() << std::endl;
		std::cout << "Time should wait: " << interRequestDelayRand << std::endl;

	}

  	delete cs_enter;
	delete cs_ready;
	delete cs_leave;
}

