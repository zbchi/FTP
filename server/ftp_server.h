#include "threadpool.h"
#include <arpa/inet.h>
#include <cstring>
#include <sys/epoll.h>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>

#define SERVER_IP "10,30,1,227"
#define SERVER_PORT 2100
#define SERVER_DIR "../SERVER_FILES/"
class ftp
{
public:
    class FD
    {
    public:
        int fd;
        bool is_passive;
    };
    ftp();
    int create_listen_socket(int port, struct sockaddr_in &addr);

    void handle_command(FD &cfd);
    void handle_stor(FD cfd, string &file_name);
    void handle_retr(FD cfd, string &file_name);
    void handle_list(FD cfd);

    int passive_connect(int command_cfd);
    int active_connect(int command_cfd);

    void log(struct sockaddr_in connect_addr, char *event);

    void epoll();

    vector<string> splite_argv(const string &strp);

private:
    threadPool pool;
};