#include "Request.h"
/**
 * @brief Default Constructor, instantiates request as a null request.
 * 
 * @return Request:: 
 */
Request:: Request()
{
    this->time_stamp = -1;
    this->node_id = -1;
}

/**
 * @brief Equality function for Request, the overloaded prioQ_compare operator() does the other comparisons.
 * 
 * @param compare  Request object to be compared with
 * @return true if the object has the same node_id and time_stamp
 * @return false if the object does not have the same node_id and time_stamp
 */
bool Request::compare(const Request &compare)
{
    return (this->time_stamp == compare.time_stamp) && (this->node_id == compare.node_id);
}

/**
 * @brief Class specific for comparing Requests that can be fed into the priority queue data structure.
 *        Specifically compares so that the priority queue structure is a min heap (default max heap).
 *        Therefore, left > right returns true.
 * 
 * @param left Left operand in form of Request struct
 * @param right Right operand in form of Request struct
 * @return true Returns true if primarily left time_stamp is > right_stamp, and tie breaks for left_node_id > right_node_id
 * @return false Returns false if any of the above is false
 */
bool prioQ_compare::operator()(const Request &left, const Request &right)
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
