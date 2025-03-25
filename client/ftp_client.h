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
        bool is_passive;
    };
    int create_connect_socket(int port, string ip);
    void send_command();
    int passive_connect(int command_cfd);
    vector<string> splite_argv(const string &strp);
    void handle_stor(FD cfd, string &file_name);
};