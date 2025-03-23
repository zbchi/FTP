#include "threadpool.h"
#include <arpa/inet.h>
#include <cstring>
#define SERVER_IP "10,30,1,227"
class ftp
{
public:
    int create_listen_socket(int port, struct sockaddr_in &addr);
    void recv_command();
    void passive_connect(int cfd);
    void log(struct sockaddr_in connect_addr, char *event);
};