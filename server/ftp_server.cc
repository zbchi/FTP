#include "ftp_server.h"
int ftp::create_socket(int port)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(lfd, 128);
    return lfd;
}

void ftp::recv_command()
{
    int first_lfd = create_socket(2100);
    int first_cfd = accept(first_lfd, NULL, NULL);
    if (first_cfd == -1)
        perror("accept");
    string buf;
    while (1)
    {
        buf.clear();
        cin >> buf;
        send(first_cfd, buf.c_str(), buf.size(), 0);
    }
}