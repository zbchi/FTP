#include "threadpool.h"
#include <arpa/inet.h>
#include <cstring>
class ftp
{
    int create_socket(int port);
    void recv_command();
};