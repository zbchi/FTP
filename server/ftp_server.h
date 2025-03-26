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
    struct client_info
    {
        int fd;
        sockaddr_in addr;
        bool is_passive;
        string active_port;
    };

    ftp();
    int create_listen_socket(int port, struct sockaddr_in &addr);
    int create_connect_socket(int port, string ip);

    void handle_command(ftp::client_info *client);
    void handle_stor(ftp::client_info *client, string &file_name);
    void handle_retr(ftp::client_info *client, string &file_name);
    void handle_list(ftp::client_info *client);

    int passive_connect(int command_cfd);
    int active_connect(ftp::client_info *client);

    void log(struct sockaddr_in connect_addr, char *event);

    void epoll();

    vector<string> splite_argv(const string &strp);

private:
    threadPool pool;
};