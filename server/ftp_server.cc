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

int ftp::create_connect_socket(int port, string ip)
{
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
    addr.sin_port = htons(port);

    int ret = connect(cfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
        perror("connect");
    return cfd;
}

int ftp::passive_listen(client_info *client)
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
    // cout << response_str << endl;

    int ret = send(client->fd, response_str.c_str(), response_str.size(), 0);
    if (ret == -1)
        perror("send");

    client->passive_lfd = lfd;
    return lfd;
}
int ftp::passive_connect(ftp::client_info *client)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int data_fd = accept(client->passive_lfd, (struct sockaddr *)&addr, &len);
    if (data_fd < 0)
        return -1;
    client_info tmp = *client;
    tmp.addr = addr;
    log(&tmp, "connect");

    return data_fd;
}

int ftp::active_connect(client_info *client)
{
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr = client->addr;

    char *end;
    addr.sin_port = htons(strtol(client->active_port.c_str(), &end, 10));
    int ret = connect(cfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("connect");
        return -1;
    }

    client_info tmp;
    tmp = *(client);
    tmp.addr = addr;
    log(&tmp, "connect");
    return cfd;
}

int ftp::select_mode_connect(client_info *client)
{
    if (client->is_passive == -1)
    {
        log(client, "unconnected");
        return -1;
    }
    int data_fd;
    if (client->is_passive)
        data_fd = passive_connect(client);
    else
        data_fd = active_connect(client);

    return data_fd;
}

void ftp::handle_command(client_info *client)
{
    char buf[1024];

    memset(buf, 0, sizeof(buf));
    int read_len = recv(client->fd, buf, sizeof(buf), 0);
    if (read_len <= 0)
        return;

    string command(buf, read_len);
    vector<string> commands = splite_argv(command);

    if (strcmp(commands[0].c_str(), "PASV") == 0)
    {
        client->is_passive = true;
        pool.add_task([this, client, &commands]()
                      { passive_listen(client); });
    }
    else if (strcmp(commands[0].c_str(), "PORT") == 0 && commands.size() == 2)
    {
        client->is_passive = false;
        client->active_port = commands[1];
    }
    else if (strcmp(commands[0].c_str(), "STOR") == 0 && commands.size() == 2)
        pool.add_task([this, client, &commands]()
                      { handle_stor(client, commands[1]); });
    else if (strcmp(commands[0].c_str(), "RETR") == 0 && commands.size() == 2)
        pool.add_task([this, client, &commands]()
                      { handle_retr(client, commands[1]); });
    else if (strcmp(commands[0].c_str(), "LIST") == 0)
        pool.add_task([this, client]
                      { handle_list(client); });
}

void ftp::handle_stor(client_info *client, string &file_name)
{
    int data_fd = select_mode_connect(client);
    if (data_fd < 0)
        return;

    string file_path = SERVER_DIR;
    file_path += file_name;
    ofstream file(file_path, ios::binary);

    if (!file.is_open())
    {
        cerr << "[Failed to open file:]" << file_path << endl;
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

void ftp::handle_retr(client_info *client, string &file_name)
{
    int data_fd = select_mode_connect(client);
    if (data_fd < 0)
        return;

    string file_path = SERVER_DIR;
    file_path += file_name;
    ifstream file(file_path, ios::binary);
    if (!file.is_open())
    {
        cerr << "[Failed to open file:" << file_path << "]" << endl;
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

void ftp::handle_list(client_info *client)
{
    int data_fd = select_mode_connect(client);
    if (data_fd < 0)
        return;

    pid_t pid = fork();

    if (pid == 0)
    {
        dup2(data_fd, STDOUT_FILENO);
        execlp("ls", "ls", "-ls", SERVER_DIR, (char *)NULL);
        close(data_fd);
    }
    close(data_fd);
}

void ftp::log(client_info *client, char *event)
{
    if (client->is_passive == 1)
        cout << "[Passive Mode]";
    else if (client->is_passive == 0)
        cout << "[Active Mode]";
    else if (client->is_passive == -1)
        cout << "[Unusable Mode]";

    int port = ntohs(client->addr.sin_port);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client->addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN);
    if (strcmp("connect", event) == 0)
        cout << "Connected  IP:" << ip << "  port:" << port << endl;
    else if (strcmp("disconnect", event) == 0)
        cout << "Disconnected  IP:" << ip << "  port:" << port << endl;
    else if (strcmp("unconnected", event) == 0)
        cout << "[Error]Unconnected but transfer data" << endl;
}

void ftp::epoll()
{
    create_dir();

    struct sockaddr_in addr;
    int lfd = create_listen_socket(SERVER_PORT, addr);

    int epfd = epoll_create(1024);
    struct epoll_event ev;

    client_info listen_socket{lfd, NULL, NULL, NULL};

    ev.data.ptr = &listen_socket;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(evs[0]);
    while (1)
    {
        int num = epoll_wait(epfd, evs, size, -1);
        for (int i = 0; i < num; i++)
        {
            client_info *current_client = (client_info *)evs[i].data.ptr;

            int curfd = current_client->fd;
            if (curfd == lfd)
            {

                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);

                int cfd = accept(lfd, (struct sockaddr *)&client_addr, &len);

                client_info *client = new client_info{cfd, client_addr, -1, ""};
                ev.data.ptr = client;
                ev.events = EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);

                log(client, "connect");
            }
            else
            {
                char buf[1024];
                int ret = recv(current_client->fd, buf, sizeof(buf), MSG_PEEK);
                if (ret == 0 || (ret == -1 && errno != EAGAIN))
                {
                    log(current_client, "disconnect");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, current_client->fd, NULL);
                    close(current_client->fd);
                    delete current_client;
                    continue;
                }
                handle_command(current_client);
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

void ftp::create_dir()
{
    string dirPath = SERVER_DIR;
    if (!filesystem::exists(dirPath))
    {
        if (!(filesystem::create_directories(dirPath)))
            cout << "[Error]Failed to create directories" << endl;
    }
}