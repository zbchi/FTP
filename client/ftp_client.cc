#include "ftp_client.h"
using namespace std;
int ftp::create_connect_socket(int port)
{
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(port);

    connect(cfd, (struct sockaddr *)&addr, sizeof(addr));
    return cfd;
}
void ftp::send_command()
{
    int cfd = create_connect_socket(2100);
    char buf[1024];
    while (1)
    {
        std::cin >> buf;
        send(cfd, buf, strlen(buf) + 1, 0);

        memset(buf, 0, sizeof(buf));
        int ret = recv(cfd, buf, sizeof(buf), 0);
        if (ret == -1)
            perror("recv");
        if (ret == 0)
            cout << "断开连接" << endl;
        cout << "-------------------------" << endl;
        std::cout << buf << std::endl;
    }
}