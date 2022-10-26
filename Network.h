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
using namespace std;

class Network
{
public:
    // Declare fields
    int my_node_id;
    priority_queue<Request, vector<Request>, prioQ_compare> prioQ;
    vector<int> lastTimeStamp; // vector of most recent time stamps from other nodes
    bool* applicationRequest; // Flag to let know there is an application request
    bool* CS_ready; // Flag to let application class know critical section can be entered
    bool* releaseFlag; // Flag to release the current head of prioq.
    vector<string> node_ips;// FIXME: implement vector of node information
    vector<int> node_ports;
    vector<int> sockets;
    

   Network(vector<string> node_ips, vector<int> node_port, int my_node_id, bool * applicationRequest, bool * CS_ready, bool * releaseFlag);

    // 
    /**
     * @brief 
     * Override the () operator for the purpose of threading
     * Will be called by passing it in to a thread object
     * i.e. thread t1(Network(), params)
     * Will implement Chandry Lamport's Mutual Exclusion Algorithm
     */
    void execute_protocol();


};

/**
 * @brief Prints the contents of a Request Priority Queue
 * 
 * @param prioQ 
 */
void showpq(priority_queue<Request, vector<Request>, prioQ_compare> prioQ);
