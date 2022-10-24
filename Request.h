#ifndef REQUEST_H_
#define REQUEST_H_

/**
 * @brief Request struct that is to be stored inside a priority queue.
 * 
 */
struct Request
{
    // Fields
    int time_stamp;
    int node_id;

    //Functions
    bool compare(const Request &compare);
};

/**
 * @brief Compare class to compare Request structures.
 * Provides a > than comparator to make sure the priority queue is a min heap in respect
 * to time stamp.
 * Includes a tie-breaker for equal time_stamps.
 */
class prioQ_compare
{
public:
    // Defined in order to make the priority queue a min heap in respect to time_stamp
    bool operator()(const Request &left, const Request &right);
    
};

#endif // REQUEST_H_