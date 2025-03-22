#include "ftp_client.h"
#include <string>
int ftp::create_socket(int port)
{
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(port);

    accept(cfd, (struct sockaddr *)&addr.sin_addr.s_addr, (socklen_t *)sizeof(addr));
    return cfd;
}
void ftp::send_command()
{
    int cfd = create_socket(2100);
    string buf;
    while (1)
    {
    }
}