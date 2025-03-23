#include "ftp_server.h"
int ftp::create_listen_socket(int port, struct sockaddr_in &addr)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    int ret = bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
        perror("bind");
    listen(lfd, 128);
    return lfd;
}
void ftp::passive_connect(int commond_cfd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int lfd = create_listen_socket(0, addr);
    getsockname(lfd, (struct sockaddr *)&addr, &len);

    int new_port = ntohs(addr.sin_port);
    int p1 = new_port / 256;
    int p2 = new_port % 256;

    string response_str;
    response_str = "227 Entering Passive Mode(" + string(SERVER_IP) + "," + to_string(p1) + "," + to_string(p2) + ").";
    cout << response_str << endl;

    int ret = send(commond_cfd, response_str.c_str(), response_str.size(), 0);
    if (ret == -1)
        perror("send");
}

void ftp::recv_command()
{
    struct sockaddr_in addr;
    int lfd = create_listen_socket(2100, addr);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int cfd = accept(lfd, NULL, NULL);
    if (cfd == -1)
        perror("accept");

    char buf[1024];
    while (1)
    {
        memset(buf, 0, sizeof(buf));
        recv(cfd, buf, sizeof(buf), 0);
        if (strcmp(buf, "PASV") == 0)
            passive_connect(cfd);
    }
}