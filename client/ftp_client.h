#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <filesystem>
#include "threadpool.h"
#define CLIENT_DIR "../CLIENT_FILES/"
using namespace std;
class ftp
{
public:
    class FD
    {
    public:
        int fd;
        sockaddr_in addr;
        int is_passive;
        string active_port;
        int listen_fd;
    };

    struct client_info
    {
        int fd;
        sockaddr_in addr;
        int is_passive;
        string active_port;
        int listen_fd;
        string passive_serverip;
        int passive_serverport;
    };
    ftp();
    void send_command();

private:
    threadPool pool;
    int create_connect_socket(int port, string ip);
    int create_listen_socket(int port, struct sockaddr_in &addr);

    void handle_stor(client_info *cfd, string &file_name);
    void handle_retr(client_info *cfd, string &file_name);
    void handle_list(client_info *cfd);

    void passive_getinfo(client_info *cfd);
    int passive_connect(client_info *cfd);
    int active_listen(client_info &cfd);
    int select_mode_connect(ftp::client_info *cfd);

    void log(client_info *client, char *event);

    vector<string> splite_argv(const string &strp);

    void create_dir();
};