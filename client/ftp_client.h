#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <fstream>
#include <unistd.h>
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
        bool is_passive;
        string active_port;
        int listen_fd;
    };

    int create_connect_socket(int port, string ip);
    int create_listen_socket(int port, struct sockaddr_in &addr);

    void send_command();
    void handle_stor(FD cfd, string &file_name);
    void handle_retr(FD cfd, string &file_name);
    void handle_list(FD cfd);

    int passive_connect(int command_cfd);
    int active_listen(FD &cfd);

    vector<string> splite_argv(const string &strp);
};