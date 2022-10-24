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
#include <chrono>  
#include <map>
#include <fstream>
#include <array>


using namespace std;

/**
 * @brief Request struct that is to be stored inside a priority queue.
 * 
 */
struct Request
{
    int time_stamp = -1;
    int node_id;
    /* data */
};

/**
 * @brief Compare class to compare Request structures.
 * Provides a > than comparator to make sure the priority queue is a min heap in respect
 * to time stamp.
 * Includes a tie-breaker for equal time_stamps.
 * Does not include tie-breaker for node_id as that means something went wrong.
 */
class prioQ_compare
{
public:
    // Defined in order to make the priority queue a min heap in respect to time_stamp
    bool operator()(const Request &left, const Request &right)
    {
        // Tie breaker if time_stamps are equal
        if (left.time_stamp == right.time_stamp)
        {
            return left.node_id >= right.node_id;
        }
        else // Otherwise, if the left value is larger than the right value, return true
        {
            return left.time_stamp > right.time_stamp;
        }
    }
};

class Network
{
public:
    // Declare fields
    int numNodes = 5; // placeholder;
    int my_node_id;
    priority_queue<Request, vector<Request>, prioQ_compare> prioQ;
    vector<int> lastTimeStamp; // Let numNodes = to size of node vector
    bool applicationRequest; // Flag to let know there is an application request
    bool CS_ready; // Flag to let application class know critical section can be entered
    bool releaseFlag; // Flag to release the current head of prioq.
    vector<string> node_ips;// FIXME: implement vector of node information
    vector<int> node_port;
    vector<int> sockets;


    // Constructors
    Network(vector<string> node_ips, vector<int> node_port)
    {
        this->node_ips = node_ips;
        this->node_port = node_port;
        lastTimeStamp.assign(numNodes,-1); // Creates vector with size numNodes, and fills with -1.
        applicationRequest = false;
        CS_ready = false;
        releaseFlag = false;


        // Establish Socket Connections

    }

    // 
    /**
     * @brief 
     * Override the () operator for the purpose of threading
     * Will be called by passing it in to a thread object
     * i.e. thread t1(Network(), params)
     * Will implement Chandry Lamport's Mutual Exclusion Algorithm
     * 
     *
     */
    void operator()()
    {
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


        std::chrono::time_point<std::chrono::system_clock> start, end;
        fd_set readfds;

        start = chrono::system_clock::now();
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

            // end = std::chrono::system_clock::now();
            // std::chrono::duration<double> elapsed_seconds = end - start;

            // if (isActive && elapsed_seconds.count() > minSendDelaySecs) {
            //     start = std::chrono::system_clock::now();

            //     send(sockets[nodeMsg], sentVectorClock.c_str(), strlen(sentVectorClock.c_str()), 0);
                
            //     sentMsgs = sentMsgs + 1;

            //     //printf("Sent message to %d. Messages Sent: %d. Elapsed Time: %f. Delay: %f.\n", nodeCons.at(node).at(nodeMsg), sentMsgs, elapsed_seconds.count(), minSendDelaySecs);

            //     //printf("Current vector clock: %s\n\n", vectorToString(vectorClock).c_str());

            // }

            // Check every socket for if there is data. (Receive Portion)
            for (int i = 0; i < sockets.size(); i++) {
                sd = sockets[i];

                if (FD_ISSET(sd, &readfds)) {
                    //FIXME: Unsure what to delete/remove from this section -----------------
                    printf("Reading Message from %d\n", nodeCons.at(node).at(i));
                    if ((valread = read(sd, buffer, 1024)) == 0) {
                        //printf("Closing Socket1: %d\n", sd);
                        getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

                        //printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                        //printf("Closing Socket: %d\n", sd);

                        close(sd);
                        sockets.erase(sockets.begin() + i);
                    }
                    // -----------------------------------------------------------------------
                    else {
                        if (valread < 0)
                            //printf("Something went wrong! %s\n", strerror(errno));

                        buffer[valread] = '\0';

                        // Just converting the c_str() from socket into a string.
                        string message(buffer);
                        int time_stamp = -1;
                        int node_id = -1;

                        // Message should be structured in a "name <time_stamp> <node_id>"
                        message.erase(0,message.find(' ') + 1); // Erase the first token
                        time_stamp = stoi(message.substr(0,message.find(' '))); // Find time_stamp
                        message.erase(0,message.find(' ') + 1); // Erase the second token "<time_stamp>"
                        node_id = stoi(message.substr(0,message.find(' '))); // Find node_id
                        
                        // Picks the larger time stamp and adds to it
                        lastTimeStamp.at(my_node_id) = max(time_stamp, lastTimeStamp.at(my_node_id)) + 1;

                        // Check for request, reply, or release message
                        if(message.substr(0,7).compare("request"))
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
                        else if (message.substr(0,7).compare("release"))
                        {
                            prioQ.pop();
                        }
                        else // For reply message
                        {
                            // Do nothing (you can remove this!)
                        }

                    }
                }
            }//End of recieve loop

            if (applicationRequest)
            {
                Request my_request;
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

            }

        }//End of while loop
    }



    // Declare Functions


};

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

/**
 * @brief Tests the PriorityQueue
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char const *argv[])
{
    priority_queue<Request, vector<Request>, prioQ_compare> prioq;
    srand(time(NULL));

    // Tests prioQ
    for (int i=0; i < 10; i++)
    {
        Request temp;
        temp.node_id = rand() % 100;
        temp.time_stamp = rand() % 100;
        prioq.push(temp);
    }

    // Tests equivalent time_stamp case

    Request temp;
    temp.node_id = 30;
    temp.time_stamp = 50;

    Request temp2;
    temp2.node_id = 62;
    temp2.time_stamp = 50;

    prioq.push(temp);
    prioq.push(temp2);

    showpq(prioq);
    return 0;
}
