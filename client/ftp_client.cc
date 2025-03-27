#include "ftp_client.h"
ftp::ftp() : pool(4)
{
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

int ftp::create_listen_socket(int port, struct sockaddr_in &addr)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 0;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    int ret = bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        cout << "[Error][Invalid port]";
        return -1;
    }
    listen(lfd, 128);
    return lfd;
}

void ftp::send_command()
{
    create_dir();

    string ip;
    cin >> ip;

    client_info cfd;
    cfd.is_passive = -1;
    cfd.fd = create_connect_socket(2100, ip);
    log(&cfd, "connect");

    cin.ignore();
    char buf[1024];

    string command;
    while (1)
    {
        command.clear();

        getline(cin, command);
        vector<string> commands = splite_argv(command);
        if (commands.size() == 0)
            continue;

        send(cfd.fd, command.c_str(), command.size(), 0);

        if (strcmp(commands[0].c_str(), "PASV") == 0)
        {
            cfd.is_passive = true;
            passive_getinfo(&cfd);
        }
        else if (strcmp(commands[0].c_str(), "PORT") == 0 && commands.size() == 2)
        {
            cfd.active_port = commands[1];
            cfd.listen_fd = active_listen(cfd);
            if (cfd.listen_fd == -1)
            {
                cfd.is_passive = -1;
                continue;
            }
            cfd.is_passive = false;
        }
        else if (strcmp(commands[0].c_str(), "STOR") == 0 && commands.size() == 2)
            pool.add_task([this, &cfd, &commands]
                          { handle_stor(&cfd, commands[1]); });
        else if (strcmp(commands[0].c_str(), "RETR") == 0 && commands.size() == 2)
            pool.add_task([this, &cfd, &commands]
                          { handle_retr(&cfd, commands[1]); });
        else if (strcmp(commands[0].c_str(), "LIST") == 0)
            handle_list(&cfd);

        log(&cfd, "connect");
    }
}

int ftp::active_listen(client_info &cfd)
{
    struct sockaddr_in local_addr;
    int port;
    char *end;
    try
    {
        port = strtol(cfd.active_port.c_str(), &end, 10);
    }
    catch (const exception &e)
    {
        cout << "[Error]Invalid port:";
        return -1;
    }
    if (port <= 0)
    {
        cout << "[Error][Invalid port]";
        return -1;
    }
    int lfd = create_listen_socket(port, local_addr);
    return lfd;
}

void ftp::passive_getinfo(client_info *cfd)
{
    char buf[64];
    memset(buf, 0, sizeof(buf));
    int ret = recv(cfd->fd, buf, sizeof(buf), 0);
    if (ret == -1)
        perror("recv");
    if (ret == 0)
        cout << "Disconnect" << endl;
    cout << buf << endl;
    int a, b, c, d, e, f;
    sscanf(buf, "227 Entering Passive Mode(%d,%d,%d,%d,%d,%d).", &a, &b, &c, &d, &e, &f);
    cfd->passive_serverip = to_string(a) + "." + to_string(b) + "." + to_string(c) + "." + to_string(d);
    cfd->passive_serverport = e * 256 + f;
}

int ftp::passive_connect(client_info *cfd)
{
    int data_fd = create_connect_socket(cfd->passive_serverport, cfd->passive_serverip);
    return data_fd;
}

int ftp::select_mode_connect(client_info *cfd)
{
    if (cfd->is_passive == -1)
    {
        log(cfd, "unconnected");
        return -1;
    }
    int data_fd;
    if (cfd->is_passive)
        data_fd = passive_connect(cfd);
    else
        data_fd = accept(cfd->listen_fd, NULL, NULL);

    if (data_fd < 0)

        log(cfd, "disconnect");

    return data_fd;
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

void ftp::handle_stor(client_info *cfd, string &file_name)
{
    int data_fd = select_mode_connect(cfd);
    if (data_fd < 0)
        return;

    string file_path = CLIENT_DIR;
    file_path += file_name;
    ifstream file(file_path, ios::binary);
    if (!file.is_open())
    {
        cerr << "[Failed to open file:" << file_path << "]";
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

void ftp::handle_retr(client_info *cfd, string &file_name)
{
    int data_fd = select_mode_connect(cfd);
    if (data_fd < 0)
        return;

    char buffer[4096];
    ssize_t bytes_recved = recv(data_fd, buffer, sizeof(buffer), 0);
    if (bytes_recved <= 0)
    {
        cerr << "[Failde to recevie data]";
        close(data_fd);
        return;
    }

    string file_path = CLIENT_DIR;
    file_path += file_name;
    ofstream file(file_path, ios::binary);

    file.write(buffer, bytes_recved);

    if (!file.is_open())
    {
        cerr << "[Failed to open file:" << file_path << "]";
        close(data_fd);
        return;
    }

    while ((bytes_recved = recv(data_fd, buffer, sizeof(buffer), 0)) > 0)
        file.write(buffer, bytes_recved);

    file.close();
    close(data_fd);
}

void ftp::handle_list(client_info *cfd)
{
    int data_fd = select_mode_connect(cfd);
    if (data_fd < 0)
        return;

    char buffer[4096];
    ssize_t bytes_recved;
    while ((bytes_recved = recv(data_fd, buffer, sizeof(buffer), 0)) > 0)
        write(STDOUT_FILENO, buffer, bytes_recved);
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

    if (strcmp("unconnected", event) == 0)
    {
        cout << "[Error][Unconnected but transfer data]" << endl;
    }
}

void ftp::create_dir()
{
    string dirPath = CLIENT_DIR;
    if (!filesystem::exists(dirPath))
    {
        if (!(filesystem::create_directories(dirPath)))
            cout << "[Error]Failed to create directories" << endl;
    }
}
