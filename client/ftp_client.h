#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <cstring>
class ftp
{
public:
    int create_connect_socket(int port);
    void send_command();
};