# WARNING
---
This project is non-functional. Issues arose where multi-threading would not work well and so this project was scrapped
and instead re-built in Java to allow ease for Sockets and Threading and concurrency.
See CS6378_Project2_Java instead for the implementation of the CL Mutual Exclusion protocol.


# Group Members
Brian Wu bkw180001
Nick Herzberg ndh180001

# Compilation
In order to run the current implementation, compile using the command
```cpp
g++ -std=c++11 *.cpp -pthread
```
Then run:
```cpp
./a.out configBase.txt 
```
on dcXX.utdallas.edu machines 1 to 4, or alternatively change the config file
to match however many machines desired and change up the parameters as necessary.

## Current Issues
- Currently, concurrency may be an issue due to threading. Atomic Booleans will need to be implemented to attempt to solve this issue.
- "Reply" messages that are sent are sometimes being sent with an extra value that is not supposed to be there. Currently unknown why, but may be related to concurrency issues.


# CS6378-Lamports-Mutual-Exclusion
Advanced Operating Systems Project that implements Lamport's Mutual Exclusion algorithm.

## Implementation
Lamport's Mutual Exclusion Algorithm is the following.
Consider Pi has a general process, and there are n amount of processes in the system.
Each process has a priority queue, and each request message has a **Logical Scalar Clock**.
Process Pi, on needing to enter it's critical section (C.S.) will put its own request into it's priority queue in the format of (<time_stamp>, <process_id>).
  The time stamp will lead the priority queue, lower time stamp having the higher priority. The tie breaker will be the process_id. Lower   
  process ID being the higher priority.

When Process Pi requests a C.S. access, it will put its own request into its priority queue and broadcast a request message to every other process.
If Process Pi receives a request message, it will put that request into it's priority queue and then send a reply message back to the process.

There are 2 conditions for a process to enter it's C.S.
  1. The process must have received **ANY** message (request, reply, or app) from all of the other processes with a time stamp larger than it's own request.
  2. It's request must be the first in the priority queue.

Upon completion of it's critical section, it will broadcast a release message to free up its request from the other processes priority queue.
