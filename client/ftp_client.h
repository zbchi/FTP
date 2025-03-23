#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <cstring>
using namespace std;
class ftp
{
public:
    int create_connect_socket(int port, string ip);
    void send_command();
    int passive_connect(int command_cfd);
};