# DDyLoadBalancer

Design and Implementation

We decide to implement this system using C++ since we were familiar with all the necessary tools. 

We build the core code for both client and server in the common.h file, and only left a few implementation details to be defined in the server.cpp and client.cpp file. This way, the design is modular, and changing in the common.h file will perform enhancement in both server and client. In this file, all the shared structures are defined. 

We have 3 main phases. The first is the bootstrap phase, which equally divide the load into both devices. Then, a working phase starts, where both the server and the client can initiate a transfer, in a work-stealing scheme. Finally, when everything is done in both places, a final transfer is initiated by the server, back to the client, which received the data, and perform a correctness check. 

We test several schemes, and ended up using a symmetrical one to initiate transfer, since this scheme was the best in terms of latency of the whole system. In other case, the client or the server may become idle for a long time until the one in charge of the communication perform a check, incurring in possible unbalance. 

For compression and decompression, we use zlib, which is a highly used library. There are different levels of compression. After testing each level, we realized that because the bandwidth of the connection is very high, compressing and decompressing is very costly, and slow down the entire system. We set a compilation flag for activate or deactivate the the compression because of this. 

Our hardware monitor is distributed into several of the method of our application, collecting information in different steps.

We used QT to implement an interface, showing both throatling and progress of the whole system. 

For the working threads, we use the phtreads library, which allow us to create as much as threads as the system can allow very easily. 



Choosing symmetric initiated transfer

We test several schemes, and ended up using a symmetrical one to initiate transfer, since this scheme was the best in terms of latency of the whole system. In other case, the client or the server may become idle for a long time until the one in charge of the communication perform a check, incurring in possible unbalance. We obtain enhancement of 20% in the total execution time by having this schemes, compared to the one using the client as the only initiator of the communication. This is because, since the application consumes a lot of time in processing, the eventual packet sent to the network takes a negligible time. 

Consider the Bandwidth and delay
We use this formula to calculate how many jobs worth sending in case of the other part issue a request:

/* CODE USED TO ANALIZE THRESHOLD
    float msecs_send = JOB_SIZE * 4 * 8 * 2 / 10e6 * 1000 + 54; 
    threshold =  msecs_send / (latency / queueready.size() ) ;
*/

This is, we calculate when sending the job back and forth will take less time than actually processing in the node. This give us the threshold value that will tell if we should send or not jobs to the other part upon request.



How to run our program
The server should run:
./server port_server

The client should run
./client server_ip server_port


In the case of running with GUI

The server should run:
./server port_server gui_ip gui_port

The client should run
./client server_ip server_port gui_ip gui_port

