//Main program
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <climits>
#include <errno.h>
#include <sys/epoll.h>
#include "server.h"

//List of Host, used for maintaining all the relevant info
host_info* HostList;
char *topo_file;
//update interval
int update_interval;
//my Ip address
char myIPaddr[INET_ADDRSTRLEN];

int main(int argc, char* argv []){
	int c;


	//Copied form http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
	while ((c = getopt (argc, argv, "t:i:")) != -1)
		switch (c)
		{
		case 't':
			if(!optarg){
				fprintf(stderr,"Usage server -t [topology-file] -i[routing-update-interval]");
				exit(1);
			}
			topo_file = optarg;
			break;
		case 'i':
			if(!optarg){
				fprintf(stderr,"Usage server -t [topology-file] -i [routing-update-interval]");
				exit(1);
			}
			update_interval = atoi(optarg);
			break;
		case '?':
			fprintf(stderr,"Usage server -t [topology-file] -i [routing-update-interval]");
			return 1;
		default:
			fprintf(stderr,"Usage server -t [topology-file] -i [routing-update-interval]");
			exit(1);
		}

	if(argc!=5){
		fprintf(stderr,"Usage server -t [topology-file] -i [routing-update-interval]");
		exit(1);
	}
	//extract info from topology file to Host List
	ExtractfromTopo(topo_file);
	my_ip(myIPaddr);

	//get Port to listen to form HostList data structure which stores all the host-info
	uint16_t myPort=getPort_from_ip(myIPaddr);

	char char_MyPort[10];
	sprintf(char_MyPort,"%d",myPort);
	//Open socket at the port
	int serverSock=start_ServerSock(char_MyPort);

	//creat Epoll file descriptor
	int efd=epoll_create(10);
	createTimer_and_EpollEntries(efd);

	make_entry_to_epoll(serverSock,efd);
	make_entry_to_epoll(0,efd);

	//get this Server's Id
	int myId=myIdx();

	//Update cost table
	UpdateCosts(myId);

	//Send Distance vec
	sendDistanceVec();

	//Wait for events loop
	WaitforEvent(efd,serverSock,update_interval);

	return 0;
}

