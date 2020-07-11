/*************************************************************************
	> File Name: client.c
	> Author: suyelu 
	> Mail: suyelu@126.com
	> Created Time: Wed 08 Jul 2020 04:32:12 PM CST
 ************************************************************************/

#include "head.h"

int server_port = 0;
char server_ip[20] = {0};
int team = -1;
char name[20] = {0};
char log_msg[512] = {0};
char *conf = "./football.conf";
int sockfd = -1;


void logout(int signum) {
    struct ChatMsg msg;
    msg.type = CHAT_FIN;
    send(sockfd, (void*)&msg, sizeof(msg), 0);
    close(sockfd);
    exit(1);
}

int main(int argc, char **argv) {
    int opt;
    struct LogRequest request;
    struct LogResponse response;
    struct User user;
    while ((opt = getopt(argc, argv, "h:p:t:m:n:")) != -1) {
        switch (opt) {
            case 't':
                team = atoi(optarg);
                break;
            case 'h':
                strcpy(server_ip, optarg);
                break;
            case 'p':
                server_port = atoi(optarg);
                break;
            case 'm':
                strcpy(log_msg, optarg);
                break;
            case 'n':
                strcpy(name, optarg);
                break;
            default:
                fprintf(stderr, "Usage : %s [-hptmn]!\n", argv[0]);
                exit(1);
        }
    }
    

    if (!server_port) server_port = atoi(get_conf_value(conf, "SERVERPORT"));
    if (!team) team = atoi(get_conf_value(conf, "TEAM"));
    if (!strlen(server_ip)) strcpy(server_ip, get_conf_value(conf, "SERVERIP"));
    if (!strlen(name)) strcpy(name, get_conf_value(conf, "NAME"));
    if (!strlen(log_msg)) strcpy(log_msg, get_conf_value(conf, "LOGMSG"));


    DBG("<"GREEN"Conf Show"NONE"> : server_ip = %s, port = %d, team = %s, name = %s\n%s\n",\
        server_ip, server_port, team ? "BLUE": "RED", name, log_msg);

    request.team = team;
    strcpy(request.msg, log_msg);
    strcpy(request.name, name);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = inet_addr(server_ip);

    socklen_t len = sizeof(server);

    if ((sockfd = socket_udp()) < 0) {
        perror("socket_udp()\n");
        exit(1);
    }

    sendto(sockfd, (void*)&request, sizeof(request), 0, (struct sockaddr *)&server, len);
    DBG(L_RED"已经连接上了服务器： %s\n", server_ip);
    fd_set rfds;   
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    int retval;
  
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    retval = select(sockfd + 1, &rfds, NULL, NULL, &tv);
    if (retval < 0) {
        perror("select()\n");
        exit(1);
    } else if (retval) {
        int ret = recvfrom(sockfd, (void*)&response, sizeof(response), 0, (struct sockaddr *)&server, &len);
        if (ret != sizeof(response) || response.type) {
            DBG(RED"ERROR"NONE"Server refused your login\n\tThis may be helpful: %s", response.msg);
            exit(1);
        }
    } else {
        DBG(RED"ERROR"NONE"Time is over 5s\n");
        exit(1);
    }
    DBG(RED"Server"NONE" : %s\n", response.msg);
    connect(sockfd, (struct sockaddr*)&server, len);

    pthread_t recv_t;
    pthread_create(&recv_t, NULL, do_recv, NULL);

    signal(SIGINT, logout);
    struct ChatMsg msg;
    while (1) {
        bzero(&msg, sizeof(msg));
        strcpy(msg.name, name);
        msg.type = CHAT_WALL;
        printf("<"BLUE"%s"NONE">"RED"Please input:\n"NONE, name);
        scanf("%[^\n]s", msg.msg);
        getchar();
        if (strlen(msg.msg) == 0) continue;
        if (msg.msg[0] == '@') {
            msg.type = CHAT_MSG;
        }
        if (msg.msg[0] == '#') {
            msg.type = CHAT_FUNC;
        }
        send(sockfd, (void*)&msg, sizeof(msg), 0);
    }
    
    return 0;
}
