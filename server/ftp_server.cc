#include "ftp_server.h"
ftp::ftp() : pool(16)
{
}

int ftp::create_listen_socket(int port, struct sockaddr_in &addr)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    int ret = bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
        perror("bind");
    listen(lfd, 128);
    return lfd;
}

int ftp::active_connect(int command_cfd)
{
}

int ftp::passive_connect(int command_cfd)
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
    return cfd;
}

void ftp::handle_command(FD &cfd)
{
    char buf[1024];

    memset(buf, 0, sizeof(buf));
    int read_len = recv(cfd.fd, buf, sizeof(buf), 0);
    if (read_len <= 0)
        return;

    cout << buf << endl;

    string command(buf, read_len);
    vector<string> commands = splite_argv(command);

    if (strcmp(commands[0].c_str(), "PASV") == 0)
        cfd.is_passive = true;
    else if (strcmp(commands[0].c_str(), "PORT") == 0)
        cfd.is_passive = false;
    else if (strcmp(commands[0].c_str(), "STOR") == 0)
        pool.add_task([this, &cfd, &commands]()
                      { handle_stor(cfd, commands[1]); });
    else if (strcmp(commands[0].c_str(), "RETR") == 0)
        pool.add_task([this, &cfd, &commands]()
                      { handle_retr(cfd, commands[1]); });
    else if (strcmp(commands[0].c_str(), "LIST") == 0)
        pool.add_task([this, &cfd]
                      { handle_list(cfd); });
}

void ftp::handle_stor(FD cfd, string &file_name)
{

    int data_fd;
    if (cfd.is_passive)
        data_fd = passive_connect(cfd.fd);

    string file_path = SERVER_DIR;
    file_path += file_name;
    ofstream file(file_path, ios::binary);

    if (!file.is_open())
    {
        cerr << "无法打开文件: " << file_path << endl;
        close(data_fd);
        return;
    }

    char buffer[4096];
    ssize_t bytes_recved;
    while ((bytes_recved = recv(data_fd, buffer, sizeof(buffer), 0)) > 0)

        file.write(buffer, bytes_recved);

    file.close();
    close(data_fd);
}

void ftp::handle_retr(FD cfd, string &file_name)
{
    int data_fd;
    if (cfd.is_passive)
        data_fd = passive_connect(cfd.fd);

    string file_path = SERVER_DIR;
    file_path += file_name;
    ifstream file(file_path, ios::binary);
    if (!file.is_open())
    {
        cerr << "无法打开文件: " << file_path << endl;
        close(data_fd);
        return;
    }

    char buffer[4096];
    while (1)
    {
        file.read(buffer, sizeof(buffer));
        if (file.gcount() <= 0)
            break;

        send(data_fd, buffer, file.gcount(), 0);
    }
    file.close();
    close(data_fd);
}

void ftp::handle_list(FD cfd)
{
    int data_fd;
    if (cfd.is_passive)
        data_fd = passive_connect(cfd.fd);

    pid_t pid = fork();

    if (pid == 0)
    {
        dup2(data_fd, STDOUT_FILENO);
        execlp("ls", "ls", "-ls", SERVER_DIR, (char *)NULL);
        close(data_fd);
    }
    close(data_fd);
}

void ftp::log(struct sockaddr_in connect_addr, char *event)
{
    int port = ntohs(connect_addr.sin_port);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &connect_addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN);
    if (strcmp("connect", event) == 0)
        cout << "Connected  IP:" << ip << "  port:" << port << endl;
}

void ftp::epoll()
{
    struct sockaddr_in addr;
    int lfd = create_listen_socket(SERVER_PORT, addr);

    int epfd = epoll_create(1024);
    struct epoll_event ev;
    ev.data.fd = lfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(evs[0]);
    while (1)
    {
        int num = epoll_wait(epfd, evs, size, -1);
        for (int i = 0; i < num; i++)
        {
            int curfd = evs[i].data.fd;
            FD cfd;
            if (curfd == lfd)
            {
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);

                cfd.fd = accept(lfd, (struct sockaddr *)&client_addr, &len);
                log(client_addr, "connect");

                ev.data.fd = cfd.fd;
                ev.events = EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd.fd, &ev);
            }
            else
            {
                cfd.fd = curfd;
                handle_command(cfd);
            }
        }
    }
    close(lfd);
}

vector<string> ftp::splite_argv(const string &strp)
{
    vector<string> args;
    istringstream stream(strp);
    string arg;
    while (stream >> arg)
    {
        args.push_back(arg);
    }

    return args;
}