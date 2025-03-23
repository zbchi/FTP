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
void ftp::passive_connect(int command_cfd)
{
    struct sockaddr_in getport_addr;
    socklen_t getport_len = sizeof(getport_addr);
    int lfd = create_listen_socket(0, getport_addr);
    getsockname(lfd, (struct sockaddr *)&getport_addr, &getport_len);

    int new_port = ntohs(getport_addr.sin_port);
    int p1 = new_port / 256;
    int p2 = new_port % 256;

    string response_str;
    response_str = "227 Entering Passive Mode(" + string(SERVER_IP) + "," + to_string(p1) + "," + to_string(p2) + ").";
    cout << response_str << endl;

    int ret = send(command_cfd, response_str.c_str(), response_str.size(), 0);
    if (ret == -1)
        perror("send");

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int cfd = accept(lfd, (struct sockaddr *)&client_addr, &len);

    if (cfd == -1)
        perror("accept");
    else
        log(client_addr, "connect");
}

void ftp::recv_command()
{
    struct sockaddr_in addr;
    int lfd = create_listen_socket(2100, addr);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int cfd = accept(lfd, (struct sockaddr *)&client_addr, &len);

    if (cfd == -1)
        perror("accept");
    else
        log(client_addr, "connect");

    char buf[1024];
    while (1)
    {
        memset(buf, 0, sizeof(buf));
        recv(cfd, buf, sizeof(buf), 0);
        if (strcmp(buf, "PASV") == 0)
            passive_connect(cfd);
    }
}

void ftp::log(struct sockaddr_in connect_addr, char *event)
{
    int port = ntohs(connect_addr.sin_port);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &connect_addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN);
    if (strcmp("connect", event) == 0)
        cout << "Connected  IP:" << ip << "  port:" << port << endl;
}