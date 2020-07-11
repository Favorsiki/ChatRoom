/*************************************************************************
	> File Name: client_recv.c
	> Author: 
	> Mail: 
	> Created Time: Fri 10 Jul 2020 10:29:02 PM CST
 ************************************************************************/

#include "head.h"
extern int sockfd;
void * do_recv(void*arg) {
    while (1) {
        struct ChatMsg msg;
        bzero(&msg, sizeof(msg));
        int ret = recv(sockfd, (void*)&msg, sizeof(msg), 0);
        if (msg.type & CHAT_WALL) {
            printf("<"BLUE"%s"NONE "> 对所有人说: %s \n", msg.name, msg.msg);
        } else if (msg.type & CHAT_MSG) {
            printf("<"YELLOW"%s"NONE"> 对你说: %s \n", msg.name, msg.msg);
        } else if (msg.type & CHAT_SYS){
            printf(YELLOW"Server Info"NONE" : %s \n", msg.msg);
        } else if (msg.type & CHAT_FIN) {
            printf(L_RED"Server Info"NONE"Server Down!\n");
            exit(1);
        } else if (msg.type & CHAT_FUNC) {
            printf("%s\n", msg.msg);
        }
    }
}
