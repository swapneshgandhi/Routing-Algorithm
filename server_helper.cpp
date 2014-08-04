#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <climits>
#include <fstream>
#include <sys/epoll.h>
#include "server.h"

using std::ifstream;
//maximum characters per line defined for buffer size for handling stdin
#define MAX_CHARS_PER_LINE 128

extern host_info* HostList;

//Cost Matrix maintained at Host
int** CostMatrix;
//Maintain Cost to each neighbor in the array
int* CostNeighbor;

extern int update_interval;
//number of servers
int numServers;
extern char myIPaddr[INET_ADDRSTRLEN];
static int packets=0;
bool crash=false;

//Allocate memory for Cost Matrix
int ** alloc_cost_mat(int numServers){
	CostMatrix = new int *[numServers+1];

	for (int i=0;i<numServers+1;i++){
		CostMatrix[i]=new int [numServers+1];
	}
	intialize_cost_mat();
	return CostMatrix;
}

//Find this servers Id
int myIdx(){
	int myid=getIdx_from_IP(myIPaddr);
	return myid;
}

//Allocate cost neighbor array
int * alloc_cost_nghbr(int numServers){
	CostNeighbor = new int [numServers+1];

	for (int i=1;i<numServers+1;i++){
		CostNeighbor[i]=uint16_t_max;
	}
	CostNeighbor[myIdx()]=0;
	return CostNeighbor;
}

//Allocate memory for Host List
void alloc_HostList(int numServers){
	HostList = new host_info [numServers+1];
}

//Initialize cost Matrix
void intialize_cost_mat(){

	for (int i=1;i<numServers+1;i++){
		for (int j=1;j<numServers+1;j++){
			if(i==j){
				(CostMatrix[i][j])=0;
			}
			else{
				(CostMatrix[i][j])=uint16_t_max;
			}
		}
	}
}

//Initialize a particular row in the cost
void initialize_my_cost_mat(int idx){

	for (int i=1;i<numServers+1;i++){

		if(idx==i){
			(CostMatrix[idx][i])=0;
		}
		else{
			(CostMatrix[idx][i])=uint16_t_max;
		}

	}
}

//make entry to epoll fd
void make_entry_to_epoll(int fd, int eventid){
	int s;
	struct epoll_event event;

	event.data.fd=fd;
	event.events = EPOLLIN;

	s = epoll_ctl (eventid, EPOLL_CTL_ADD, fd, &event);
	if (s == -1)
	{
		perror ("epoll_ctl");
		abort ();
	}
	/* Buffer where events are returned */
}


void ExtractfromTopo(const char* topo_file){
	ifstream fin;
	char *endptr;
	char buf[MAX_CHARS_PER_LINE];

	fin.open(topo_file); // open a file
	if (!fin.good()){
		fprintf(stderr,"Can't open topology file specified file");
		exit(1);
	}
	// read each line of the file

	//Copied from http://cs.dvc.edu/HowTo_Cparse.html
	fin.getline(buf, MAX_CHARS_PER_LINE);
	//convert firstarg to integer

	numServers = strtol(buf, &endptr, 10);									//taken this from man page of strtol

	if ((errno == ERANGE && (numServers == uint16_t_max || numServers == uint16_t_min))
			|| (errno != 0 && numServers == 0)) {
		perror("strtol");
		fprintf(stderr,"Invalid topology file.");
		exit(1);
	}

	fin.getline(buf, MAX_CHARS_PER_LINE);
	int numNeighbor = strtol(buf, &endptr, 10);									//taken this from man page of strtol

	if ((errno == ERANGE && (numNeighbor == uint16_t_max || numNeighbor == uint16_t_min))
			|| (errno != 0 && numNeighbor == 0)) {
		perror("strtol");
		fprintf(stderr,"Invalid topology file.");
		exit(1);
	}
	alloc_HostList(numServers);
	int i=0;
	while (i<numServers)
	{
		// read an entire line into memory

		fin.getline(buf, MAX_CHARS_PER_LINE);
		int id;
		char ip_addr [16];
		char *ip_ptr=ip_addr;
		uint16_t port;
		id = strtol(strtok(buf," "), &endptr, 10);
		if ((errno == ERANGE && (id == uint16_t_max || id == uint16_t_min))
				|| (errno != 0 && id == 0)) {
			perror("strtol");
			fprintf(stderr,"Invalid topology file.");
			exit(1);
		}

		ip_ptr=strtok(NULL," ");

		if(!strchr(ip_ptr,'.')){
			fprintf(stderr,"Invalid topology file IP address is not where if should be.");
			exit(1);
		}

		port = strtol(strtok(NULL," "), &endptr, 10);
		if ((errno == ERANGE && (port == uint16_t_max|| port == uint16_t_min))
				|| (errno != 0 && port == 0)) {
			perror("strtol");
			fprintf(stderr,"Invalid topology file.");
			exit(1);
		}
		//make entry of the host to Host-List
		host_info host;
		host.id=id;
		strcpy(host.server_ip,ip_ptr);
		host.port=port;
		HostList[id]=host;
		i++;
	}

	i=0;
	CostMatrix=alloc_cost_mat(numServers);
	alloc_cost_nghbr(numServers);
//making entries of various costs in cost matrix
	while (i<numNeighbor){
		int idx1;
		int idx2;
		int cost;
		fin.getline(buf, MAX_CHARS_PER_LINE);

		idx1 = strtol(strtok(buf," "), &endptr, 10);
		if ((errno == ERANGE && (idx1 == uint16_t_max|| idx1 == uint16_t_min))
				|| (errno != 0 && idx1 == 0)) {
			perror("strtol");
			fprintf(stderr,"Invalid topology file.");
			exit(1);
		}

		idx2 = strtol(strtok(NULL," "), &endptr, 10);
		if ((errno == ERANGE && (idx2 == uint16_t_max|| idx2 == uint16_t_min))
				|| (errno != 0 && idx2 == 0)) {
			perror("strtol");
			fprintf(stderr,"Invalid topology file.");
			exit(1);
		}

		cost = strtol(strtok(NULL," "), &endptr, 10);
		if ((errno == ERANGE && (cost == uint16_t_max|| cost == uint16_t_min))
				|| (errno != 0 && cost == 0)) {
			perror("strtol");
			fprintf(stderr,"Invalid topology file.");
			exit(1);
		}
		HostList[idx2].neighbor=true;
		CostNeighbor[idx2]=cost;
		i++;
	}
}

//Create start timer entries
void createTimer_and_EpollEntries(int eventfd){

	for (int i=1;i<numServers;i++){
		HostList[i].start_time=time(NULL);
	}
}

//get server Id for given IP
int getIdx_from_IP(const char *ip){
	for(int i=1;i<numServers+1;i++){
		if(!strcmp(HostList[i].server_ip,ip)){
			return i;
		}
	}
	return -1;
}

uint16_t  getPort_from_ip(char *ip){
	for(int i=1;i<numServers+1;i++){
		if(!strcmp(HostList[i].server_ip,ip)){
			return HostList[i].port;
		}
	}
	return 0;
}


//returns the ip address of the host machine
//copied most of the code form here http://jhshi.me/2013/11/02/how-to-get-hosts-ip-address/
char *  my_ip (char *ip_addr){
	char ip[256];

	//DNS server
	char target_name[8] = "8.8.8.8";
	// DNS port
	char target_port[3] = "53";
	char port[20];
	strcpy(ip_addr,"error");
	/* get peer server */
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo* info;
	int ret = 0;
	if ((ret = getaddrinfo(target_name, target_port, &hints, &info)) != 0) {
		fprintf(stderr, "[ERROR]: getaddrinfo error: %s\n", gai_strerror(ret));
		return ip_addr;
	}

	if (info->ai_family == AF_INET6) {
		fprintf(stderr, "[ERROR]: do not support IPv6 yet.\n");
		return ip_addr;
	}

	/* create socket */
	int sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (sock <= 0) {
		perror("socket");
		return ip_addr;
	}

	/* connect to server */
	if (connect(sock, info->ai_addr, info->ai_addrlen) < 0) {
		perror("connect");
		close(sock);
		return ip_addr;
	}

	/* get local socket info */
	struct sockaddr_storage local_addr;
	socklen_t addr_len = sizeof(local_addr);
	if (getsockname(sock, (struct sockaddr*)&local_addr, &addr_len) < 0) {
		perror("getsockname");
		close(sock);
		return ip_addr;
	}

	/* get peer ip addr */
	if (local_addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&local_addr;

		inet_ntop(AF_INET, &s->sin_addr, ip, sizeof ip);
	} else { // AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&local_addr;

		inet_ntop(AF_INET6, &s->sin6_addr, ip, sizeof ip);
	}
	strcpy(ip_addr,ip);

	return ip_addr;
}

//remove entry from epoll, used in crash simulation
void remove_entry_from_epoll(int fd,int eventid){
	struct epoll_event event;
	if(fd){
		int	s = epoll_ctl (eventid, EPOLL_CTL_DEL, fd, &event);
		if (s == -1)
		{
			perror ("epoll_ctl");
			abort ();
		}
	}
}

//detect if neighbor didn't send messages for 3 successive intervals
void detect_neighbor_timeout(){
	double timout_rem=3*update_interval;
	time_t currtime=time(NULL);
	for (int i=1;i<numServers+1;i++){
		if(!HostList[i].neighbor || CostNeighbor[i]==uint16_t_max){
			continue;
		}

		if(HostList[i].start_time && timout_rem<=difftime(currtime,HostList[i].start_time)){
			fprintf(stderr,"Didn't receive updates in last 3 intervals; assuming server %d has crashed.\n",HostList[i].id);

			CostNeighbor[i]=uint16_t_max;
			UpdateCosts(myIdx());
		}
	}
}

//handle various tasks after a timeout has occurred
void TimeoutHandle(){
	sendDistanceVec();
	detect_neighbor_timeout();
}

//check if timeout has occurred
bool check_if_timeout(int id){
	time_t currtime=time(NULL);
	if(difftime(currtime,HostList[id].start_time)>=update_interval){
		return true;
	}
	else{
		return false;
	}
}

/*Epoll waiting loop
 *
 *
 * Referred to https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/
 * on how to use Epoll and have copied part of the code from this tutorial
 */
void WaitforEvent(int eventfd, int serverSock, int timeout){

	struct epoll_event* events = new epoll_event [numServers*2];
	int myid=myIdx();
	HostList[myid].start_time=time(NULL);
	while (1)
	{
		int n, i;
		int infd;
		n = epoll_wait (eventfd, events, numServers*2, (float)timeout*1000);

		//if n =0 means timeout has occurred
		if(!n){
			fprintf(stderr,"timeout over..\n");
			TimeoutHandle();
			HostList[myid].start_time=time(NULL);
			fflush(stdout);
		}

		for (i = 0; i < n; i++)
		{
			if(crash){
				//server has crashed the remove the epoll entries
				remove_entry_from_epoll(events[i].data.fd,eventfd);
				n=0;
				timeout=-1;
				continue;
			}

			//if event occurred on stdin
			if (events[i].data.fd==0){
				HandleStdin();
			}

			//if event occurred on seversocket
			else if(events[i].data.fd==serverSock){
				recvMessagefrom(events[i].data.fd);
				UpdateCosts(myid);
			}

			//check if timeout has occurred since last time
			if(check_if_timeout(myid)){
				fprintf(stderr,"timeout over..\n");
				TimeoutHandle();
				HostList[myid].start_time=time(NULL);
			}
			fflush(stdout);
		}
	}
}

uint16_t myPort(){
	return getPort_from_ip(myIPaddr);
}

//send distance vector
void sendDistanceVec(){
	typedef unsigned char bytes;
	//message field according to standars 2 or 4 bytes
	uint16_t NumUpdateFields;
	uint16_t ServerPort;
	uint32_t ServerIP;
	uint16_t Filler=0;
	uint16_t Id;
	uint16_t Cost;

	//allocate memory for messgae
	bytes *Message = new bytes[8+12*numServers];
	bytes *tmpptr=(bytes *)Message;
	//header
	NumUpdateFields=numServers;
	htons(NumUpdateFields);

	ServerPort=myPort();
	htons(ServerPort);

	struct sockaddr_in sa;
	// store this IP address in sa:
	inet_pton(AF_INET, myIPaddr, &(sa.sin_addr));
	ServerIP=sa.sin_addr.s_addr;
	htonl(ServerIP);

	memcpy(tmpptr,&NumUpdateFields,sizeof(NumUpdateFields));
	tmpptr+=sizeof(NumUpdateFields);

	memcpy(tmpptr,&ServerPort,sizeof(ServerPort));
	tmpptr+=sizeof(ServerPort);

	memcpy(tmpptr,&ServerIP,sizeof(ServerIP));
	tmpptr+=sizeof(ServerIP);

	for (int i=1;i<numServers+1;i++){
		struct sockaddr_in sa;
		// store this IP address in sa:
		inet_pton(AF_INET, HostList[i].server_ip, &(sa.sin_addr));
		ServerIP=sa.sin_addr.s_addr;
		htonl(ServerIP);
		ServerPort=HostList[i].port;
		htons(ServerPort);
		htons(Filler);
		Id=HostList[i].id;
		htons(Id);
		Cost=CostMatrix[myIdx()][Id];
		htons(Cost);
		memcpy(tmpptr,&ServerIP,sizeof(ServerIP));
		tmpptr+=sizeof(ServerIP);

		memcpy(tmpptr,&ServerPort,sizeof(ServerPort));
		tmpptr+=sizeof(ServerPort);

		memcpy(tmpptr,&Filler,sizeof(Filler));
		tmpptr+=sizeof(Filler);

		memcpy(tmpptr,&Id,sizeof(Id));
		tmpptr+=sizeof(Id);

		memcpy(tmpptr,&Cost,sizeof(Cost));
		tmpptr+=sizeof(Cost);

	}

	//send messages to neighbor
	for (int i=1;i<numServers+1;i++){

		if(HostList[i].neighbor){
			char Port[10];

			sprintf(Port,"%d",HostList[i].port);

			Sentto((void *)Message, HostList[i].server_ip,Port,8+12*(numServers));

		}
	}
}
//send the source data to destIP at dest port
//Copied for beej.us almost all of the network programming part
void Sentto(void* srcdata,const char* destip,const char *destport, size_t total){

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(destip, destport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));

	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to bind socket\n");
	}

	size_t left=total;
	size_t sent= 0;

	while (sent<total){
		numbytes = sendto(sockfd, (unsigned char*)srcdata+sent, left, 0,p->ai_addr, p->ai_addrlen);
		if (numbytes==-1){
			break;
			perror("talker: sendto");
			exit(1);

		}
		sent+=numbytes;
		left-=numbytes;
	}
	freeaddrinfo(servinfo);
	close(sockfd);
}

static void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//update the timeout start time of the Host
void updateStartTimer(int id){
	HostList[id].start_time=time(NULL);
}

void recvMessagefrom(int ServerSock){
	typedef unsigned char bytes;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	addr_len = sizeof their_addr;
	int numbytes;
	char senderIP[INET_ADDRSTRLEN];
	bytes *Message= new bytes[1024];
	bytes *tmpptr;
	int i=0;
	uint16_t numUpdateFields;
	uint16_t ServerPort;
	uint32_t ServerIP;
	uint16_t Filler;
	uint16_t Id;
	uint16_t Cost;

	//set timeout for receive blocking
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	if (setsockopt(ServerSock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
		return;
	}

	if ((numbytes = recvfrom(ServerSock, Message, 8+12*numServers , 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1) {

		//if timeout occurred
		if(errno==EAGAIN){
			fprintf(stderr,"false positive");
			return;
		}
		perror("recvfrom");
		exit(1);
	}

	//unpack data received from the neighbor
	tmpptr=Message;
	inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),senderIP, sizeof senderIP);
	//
	memcpy(&numUpdateFields,tmpptr,sizeof(numUpdateFields));
	ntohs(numUpdateFields);
	tmpptr+=sizeof(numUpdateFields);
	memcpy(&ServerPort,tmpptr,sizeof(ServerPort));
	ntohs(ServerPort);
	tmpptr+=sizeof(ServerPort);
	memcpy(&ServerIP,tmpptr,sizeof(ServerIP));
	ntohl(ServerIP);
	tmpptr+=sizeof(ServerIP);

	struct sockaddr_in sa;
	sa.sin_addr.s_addr=ServerIP;
	inet_ntop(AF_INET,&(sa.sin_addr),senderIP,INET_ADDRSTRLEN);
	int senderidx=getIdx_from_IP(senderIP);

	if(!HostList[senderidx].neighbor){
		return;
	}
	packets++;
	updateStartTimer(senderidx);

	fprintf(stderr,"RECEIVED A MESSAGE FROM SERVER %d\n",senderidx);

	while(i<numUpdateFields){
		memcpy(&ServerIP,tmpptr,sizeof(ServerIP));
		ntohl(ServerIP);
		char ip[16];
		struct sockaddr_in sa;
		sa.sin_addr.s_addr=ServerIP;
		inet_ntop(AF_INET,&(sa.sin_addr),ip,INET_ADDRSTRLEN);
		tmpptr+=sizeof(ServerIP);

		memcpy(&ServerPort,tmpptr,sizeof(ServerPort));
		ntohs(ServerPort);
		tmpptr+=sizeof(ServerPort);

		memcpy(&Filler,tmpptr,sizeof(Filler));
		ntohs(Filler);
		tmpptr+=sizeof(Filler);

		memcpy(&Id,tmpptr,sizeof(Id));
		ntohs(Id);
		tmpptr+=sizeof(Id);

		memcpy(&Cost,tmpptr,sizeof(Cost));
		ntohs(Cost);
		tmpptr+=sizeof(Cost);
		CostMatrix[senderidx][Id]=Cost;
		i++;
	}
	delete [] Message;
}

void tolowercase(char *str){
	int i=0;
	while(str[i]!='\0'){
		str[i]=std::tolower(str[i]);
		i++;
	}
}

//handle stdin event
void HandleStdin(){

	char buf[MAX_CHARS_PER_LINE];
	char *firstarg=buf;
	int id=myIdx();

	int bytes=read(0,buf,MAX_CHARS_PER_LINE);
	buf[bytes-1]='\0';

	firstarg=strtok(buf," ");
	//cpy_ptr=strtok(cpy_buf,"\n");
	if(!firstarg){
		return;
	}
	tolowercase(firstarg);

	if(!strcmp(firstarg,"step")){
		UpdateCosts(id);
		sendDistanceVec();
		fprintf(stderr,"%s SUCCESS\n",firstarg);
	}

	else if(!strcmp(firstarg,"packets")){
		fprintf(stderr,"Number of packets received %d\n",packets);
		fprintf(stderr,"%s SUCCESS\n",firstarg);
	}

	else if(!strcmp(firstarg,"display")){
		char nextHop[10]="";
		char cost[10]="";
		UpdateCosts(id);

		for (int i=1;i<numServers+1;i++){

			if(HostList[i].nextHop==uint16_t_max){
				strcpy(nextHop,"N/A");
			}

			else if(CostMatrix[id][i]==uint16_t_max){
				HostList[i].nextHop=uint16_t_max;
				strcpy(nextHop,"N/A");
			}

			else{
				strcpy(nextHop,"");
				sprintf(nextHop,"%d",HostList[i].nextHop);
			}

			if(CostMatrix[id][i]==uint16_t_max){
				strcpy(cost,"inf");
			}

			else{
				strcpy(cost,"");
				sprintf(cost,"%d",CostMatrix[id][i]);
			}

			fprintf(stderr,"%d %s %s\n",i,nextHop,cost);

		}
		fprintf(stderr,"%s SUCCESS\n",firstarg);
	}

	else if(!strcmp(firstarg,"disable")){
		char *secondarg=strtok(NULL," ");

		if(!(secondarg)){
			fprintf(stderr,"%s Error: the second argument is not valid\n",firstarg);
			return;
		}
		CostMatrix[id][atoi(secondarg)]=uint16_t_max;
		CostNeighbor[atoi(secondarg)]=uint16_t_max;
		HostList[atoi(secondarg)].neighbor=false;
		fprintf(stderr,"%s SUCCESS\n",firstarg);
		UpdateCosts(id);
	}

	else if(!strcmp(firstarg,"update")){
		char *secondarg=strtok(NULL," ");
		char *thirdarg=strtok(NULL," ");
		char *fourtharg=strtok(NULL," ");
		int cost;

		if(!secondarg || !thirdarg || !fourtharg){
			fprintf(stderr,"%s Error:Invalid command:Wrong parameters\n",firstarg);
			return;
		}

		if(!strcmp(fourtharg,"inf")){
			cost=uint16_t_max;
		}
		else{
			cost=atoi(fourtharg);
		}

		if(!atoi(thirdarg) || !(atoi(thirdarg) < numServers+1) || !(atoi(thirdarg) > 0) || (atoi(thirdarg) == id) || !(HostList[atoi(thirdarg)].neighbor)){
			fprintf(stderr,"%s Error:The second index provided to update is not valid.\n",firstarg);
			return;
		}

		if(atoi(secondarg)!=id){
			fprintf(stderr,"%s Error:The first argument to update must be this server's Id\n",firstarg);
			return;
		}
		else{
			CostMatrix[id][atoi(thirdarg)]=cost;
			CostNeighbor[atoi(thirdarg)]=cost;
			fprintf(stderr,"%s SUCCESS\n",firstarg);
		}
	}

	else if(!strcmp(firstarg,"crash")){
		crash=true;
		fprintf(stderr,"%s SUCCESS\n",firstarg);
	}

	else{
		fprintf(stderr,"%s Error: Invalid command\n",firstarg);
	}
}

int min (int num1,int num2){
	return num1<num2?num1:num2;
}

//update costs in the matrix
void UpdateCosts(int idx){
	int prev=uint16_t_max;
	//Initialize the cost matrix of this index so that correct values be calculated
	initialize_my_cost_mat(idx);
	for (int i=1;i<numServers+1;i++){
		for (int j=1;j<numServers+1;j++){

			if(i==idx){
				HostList[i].nextHop=uint16_t_max;
				break;
			}
			prev=CostMatrix[idx][i];

			if(prev > min(CostNeighbor[i],CostMatrix[idx][j]+CostMatrix[j][i])){

				if(CostNeighbor[i] < CostMatrix[idx][j]+CostMatrix[j][i]){

					CostMatrix[idx][i]=CostNeighbor[i];
					HostList[i].nextHop=i;
				}
				else if(CostNeighbor[i]>=(CostMatrix[idx][j]+CostMatrix[j][i])){

					CostMatrix[idx][i]=CostMatrix[idx][j]+CostMatrix[j][i];
					HostList[i].nextHop=j;
				}
				//update in value start from begining.
				if(CostMatrix[idx][i]>100){
					CostMatrix[idx][i]=uint16_t_max;
					HostList[i].nextHop=uint16_t_max;
				}
			}
		}
	}
}

//start server socket copied from beej.us
int start_ServerSock(const char *port){
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;

	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
			exit(1);
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}
	freeaddrinfo(servinfo);
	fprintf(stderr, "Server: Listening at port %s...\n",port);
	return sockfd;
}
