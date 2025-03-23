#include "threadpool.h"
#include <arpa/inet.h>
#include <cstring>
#include <sys/epoll.h>
#define SERVER_IP "10,30,1,227"
#define SERVER_PORT 2100
class ftp
{
public:
    int create_listen_socket(int port, struct sockaddr_in &addr);
    void handle_command(int cfd);
    void passive_connect(int cfd);
    void log(struct sockaddr_in connect_addr, char *event);
    void epoll();
};