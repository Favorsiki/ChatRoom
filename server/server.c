/*************************************************************************
:Q
:q
	> File Name: server.c
	> Author: suyelu 
	> Mail: suyelu@126.com
	> Created Time: Thu 09 Jul 2020 10:51:49 AM CST
 ************************************************************************/

#include "head.h"
char *conf = "./footballd.conf";
struct Map court;
struct Bpoint ball; //球的位置
struct BallStatus ball_status; //球的状态
struct Score score;
int repollfd, bepollfd;
struct User *rteam, *bteam;
int port = 0;
pthread_mutex_t rmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bmutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {
    int opt, listener, epollfd;
    pthread_t red_t, blue_t;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage : %s -p port\n", argv[0]);
                exit(1);
        }
    }

    //判断一配置文件合法性

    if (!port) port = atoi(get_conf_value(conf, "PORT"));

    if ((listener = socket_create_udp(port)) < 0) {
        perror("socket_create_udp()");
        exit(1);
    }
    DBG(GREEN"INFO"NONE" : Server start On port %d.\n", port);

    rteam = (struct User *)calloc(MAX, sizeof(struct User));
    bteam = (struct User *)calloc(MAX, sizeof(struct User));

    epollfd = epoll_create(MAX * 2);
    repollfd = epoll_create(MAX);
    bepollfd = epoll_create(MAX);

    if (epollfd < 0 || repollfd < 0 || bepollfd < 0) {
        perror("epoll_create()");
        exit(1);
    }
    
    struct task_queue redQueue;
    struct task_queue blueQueue;
    task_queue_init(&redQueue, MAX, repollfd);
    task_queue_init(&blueQueue, MAX, bepollfd);
    
    pthread_create(&red_t, NULL, sub_reactor, (void *)&redQueue);
    pthread_create(&blue_t, NULL, sub_reactor, (void *)&blueQueue);
    
    struct epoll_event ev, events[MAX * 2];
    ev.events = EPOLLIN;
    ev.data.fd = listener;
    
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listener, &ev) < 0) {
        perror("epoll_ctl");
        exit(1);
    }
 
    struct sockaddr_in client;
    bzero(&client, sizeof(client));
    socklen_t len = sizeof(client);

    while (1){
    int nfds = epoll_wait(epollfd, events, MAX * 2, -1); 
        if (nfds < 0) {
            perror("epoll_wait()");
            exit(1);
        }
        for (int i = 0; i < nfds; i++) {
            struct User user;
            bzero(&user, sizeof(user));
            char buff[512] = {0};
            if (events[i].data.fd == listener) {
                int new_fd = udp_accept(listener, &user);
                if (new_fd > 0) {
                    add_to_sub_reactor(&user);
                }
            }
        }
    }

    return 0;
}
