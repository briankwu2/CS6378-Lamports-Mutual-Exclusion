#include <thread>
#include <queue>
#include <random>

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
    priority_queue<Request, vector<Request>, prioQ_compare> prioQ;
    vector<int> lastTimeStamp; // Let numNodes = to size of node vector
    bool applicationRequest; // Flag to let know there is an application request
    bool CSReady; // Flag to let application class know critical section can be entered
    bool releaseFlag; // Flag to release the current head of prioq.

    // Constructors
    Network()
    {
        this->lastTimeStamp.assign(numNodes,-1); // Creates vector with size numNodes, and fills with -1.

    }

    // 
    /**
     * @brief 
     * Override the () operator for the purpose of threading
       Will be called by passing it in to a thread object
       i.e. thread t1(Network(), params)
     *
     */
    void operator()()
    {
        // Do something
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
