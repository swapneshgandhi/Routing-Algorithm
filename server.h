#ifndef SERVER_H
#define SERVER_H
#include <arpa/inet.h>

//defined as costs have to be 2 byets max.
#define uint16_t_max 65535
#define uint16_t_min 0

//Host info
struct host_info{
	int id;
	char server_ip[INET_ADDRSTRLEN];
	uint16_t port;
	bool neighbor;
	time_t start_time;
	int nextHop;
};

int ** alloc_cost_mat(int numServers);
void ExtractfromTopo(const char * topo_file);
void createTimer_and_EpollEntries(int eventfd);
void intialize_cost_mat();
void remove_entry_from_epoll(int fd,int eventid);
void make_entry_to_epoll(int fd, int eventid);
int StartTimer(int timer, int idx);
int getTimerfd_for_id(int idx);
host_info* getHostInfo_for_id(int idx);
int getIdx_from_IP(const char *ip);
char * getIP_from_idx(int id, char* ip);
char *  my_ip (char *ip_addr);
int myIdx();
void TimeoutHandle();
void testfun(void *Message);
void sendDistanceVec();
void Sentto(void* srcdata,const char* destip,const char *destport);
void recvMessagefrom(int ServerSock);
void HandleStdin();
void tolowercase(char *str);
int start_ServerSock(const char *port);
void Sentto(void* srcdata,const char* destip,const char *destport, size_t total);
int min (int,int);
void initialize_my_cost_mat(int idx);
uint16_t getPort_from_ip(char *ip);
void detect_neighbor_timeout();
bool check_if_timeout(int id);
void WaitforEvent(int eventfd, int serverSock, int timeout);
void updateStartTimer(int id);
void UpdateCosts(int myIdx);
int getId_for_timer(int timer);
#endif

