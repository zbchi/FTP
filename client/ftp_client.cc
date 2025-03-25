#include "ftp_client.h"
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
void ftp::send_command()
{
    string ip;
    cin >> ip;

    FD cfd;
    cfd.fd = create_connect_socket(2100, ip);
    cin.ignore();
    char buf[1024];

    string command;
    while (1)
    {
        command.clear();

        getline(cin, command);

        cout << "cin command:" << command << endl;
        send(cfd.fd, command.c_str(), command.size(), 0);
        vector<string> commands = splite_argv(command);
        if (strcmp(commands[0].c_str(), "PASV") == 0)
            cfd.is_passive = true;
        else if (strcmp(commands[0].c_str(), "PORT") == 0)
            cfd.is_passive = false;
        else if (strcmp(commands[0].c_str(), "STOR") == 0)
            handle_stor(cfd, commands[1]);
        else if (strcmp(commands[0].c_str(), "RETR") == 0)
            handle_retr(cfd, commands[1]);
        else if (strcmp(commands[0].c_str(), "LIST") == 0)
            handle_list(cfd);
    }
}

int ftp::active_connect(int command_cfd)
{
}

int ftp::passive_connect(int command_cfd)
{
    char buf[64];
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

void ftp::handle_stor(FD cfd, string &file_name)
{
    int data_fd;
    if (cfd.is_passive)
        data_fd = passive_connect(cfd.fd);

    string file_path = CLIENT_DIR;
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

void ftp::handle_retr(FD cfd, string &file_name)
{
    int data_fd;
    if (cfd.is_passive)
        data_fd = passive_connect(cfd.fd);

    string file_path = CLIENT_DIR;
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

void ftp::handle_list(FD cfd)
{
    int data_fd;
    if (cfd.is_passive)
        data_fd = passive_connect(cfd.fd);

    char buffer[4096];
    ssize_t bytes_recved;
    while ((bytes_recved = recv(data_fd, buffer, sizeof(buffer), 0)) > 0)
        write(STDOUT_FILENO, buffer, bytes_recved);
    close(data_fd);
}