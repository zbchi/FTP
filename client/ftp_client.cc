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
        if (strcmp(commands[0].c_str(), "STOR") == 0)
            handle_stor(cfd, commands[1]);
    }
}
int ftp::passive_connect(int command_cfd)
{
    char buf[64];
    strcpy(buf, "PASV");
    // send(command_cfd, buf, strlen(buf) + 1, 0);

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