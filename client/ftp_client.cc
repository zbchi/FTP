#include "ftp_client.h"
int ftp::create_connect_socket(int port, string ip)
{
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
    addr.sin_port = htons(port);

    connect(cfd, (struct sockaddr *)&addr, sizeof(addr));
    return cfd;
}
void ftp::send_command()
{
    string ip;
    cin >> ip;

    int cfd = create_connect_socket(2100, ip);
    char buf[1024];
    while (1)
    {
        std::cin >> buf;
        if (strcmp(buf, "PASV") == 0)
            passive_connect(cfd);
    }
}
int ftp::passive_connect(int command_cfd)
{
    char buf[64];
    strcpy(buf, "PASV");
    send(command_cfd, buf, strlen(buf) + 1, 0);

    memset(buf, 0, sizeof(buf));
    int ret = recv(command_cfd, buf, sizeof(buf), 0);
    if (ret == -1)
        perror("recv");
    if (ret == 0)
        cout << "Disconnect" << endl;
    cout << buf << endl;
    int a, b, c, d, e, f;
    sscanf(buf, "227 Entering Passive Mode(%d,%d,%d,%d,%d,%d).", &a, &b, &c, &d, &e, &f);
    string server_ip = to_string(a) + "." + to_string(b) + "." + to_string(c) + "." + to_string(d);
    int port = e * 256 + f;
    int cfd = create_connect_socket(port, server_ip);
    return cfd;
}