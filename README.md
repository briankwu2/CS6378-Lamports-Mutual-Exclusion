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
