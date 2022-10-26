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
//#include <gsl/gsl_randist.h>
//#include <gsl/gsl_rng.h>

int main()
{

	// Temporary Random Value
	
	srand(time(NULL));

	// Calls Parser or is given the values

	double interRequestDelay = 20.0/1000.0;
	double csExeTime = 10.0/1000.0;
	int maxRequests = 1000;

	//double interRequestDelay = global_parameters[1]/1000.0;
	//double csExeTime = global_parameters[2]/1000.0;
	//int maxRequests = global_parameters[3];
	
	double csExeTimeRand = 0.0;
	double interRequestDelayRand = 0.0;

	//gsl_rng* r = gsl_rng_alloc (gsl_rng_taus);
	//gsl_rng_set(r, time(NULL));

	std::chrono::time_point<std::chrono::system_clock> start_request, end_request;
	std::chrono::time_point<std::chrono::system_clock> start_cs, end_cs;

	while(maxRequests > 0){
		//interRequestDelayRand = gsl_ran_exponential(r, interRequestDelay);
		//csExeTimeRand = gsl_ran_exponential(r, csExeTime);		
		//rand() % (sockets.size());

		csExeTimeRand = csExeTime+((double)(rand() % 50))/1000.0;

		interRequestDelayRand = interRequestDelay+((double)(rand() % 50))/1000.0;

		std::cout << "Trying request." << std::endl;
		maxRequests = maxRequests - 1;

		//cs-enter();

		//while (!cs_ready){}
		
		start_cs = std::chrono::system_clock::now();
		end_cs = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds_cs = end_cs - start_cs;

		while(elapsed_seconds_cs.count() < csExeTime){
			usleep(100);
			end_cs = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds_cs = end_cs - start_cs;
		}
		std::cout << "Time executing: " << elapsed_seconds_cs.count() << std::endl;
		std::cout << "Done executing." << std::endl;

		//cs-leave();

		start_request = std::chrono::system_clock::now();
		end_request = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds_request = end_request - start_request;
		
		while(elapsed_seconds_request.count() < interRequestDelay){
			usleep(100);
			end_request = std::chrono::system_clock::now();
			elapsed_seconds_request = end_request - start_request;
		}
		std::cout << "Time waited: " << elapsed_seconds_request.count() << std::endl;

	}

	//gsl_rng_free(r);

}