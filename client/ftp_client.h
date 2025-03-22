#include <iostream>
#include <arpa/inet.h>
#include <string>
class ftp
{
    int create_socket(int port);
    void send_command();
};