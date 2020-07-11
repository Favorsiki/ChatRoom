/*************************************************************************
	> File Name: thread_pool.c
	> Author: suyelu 
	> Mail: suyelu@126.com
	> Created Time: Thu 09 Jul 2020 02:50:28 PM CST
 ************************************************************************/

#include "head.h"
extern int bepollfd, repollfd;
extern struct User *bteam, *rteam;
extern pthread_mutex_t bmutex, rmutex;


void send_all(struct ChatMsg *msg) {
    for (int i = 0; i < MAX;i++ ) {
        if (bteam[i].online) send(bteam[i].fd, (void*)msg, sizeof(struct ChatMsg), 0);
        if (rteam[i].online) send(rteam[i].fd, (void*)msg, sizeof(struct ChatMsg), 0);
    }
}


void send_to(char * to, struct ChatMsg *msg, int fd) {
    int flag = 0;
    for (int i = 0;i < MAX; i++) {
        if (rteam[i].online && (!strcmp(to, rteam[i].name))) {
            send(rteam[i].fd, msg, sizeof(struct ChatMsg), 0);
            flag = 1;
            break;
        }
        if (bteam[i].online && (!strcmp(to, bteam[i].name))) {
            send(bteam[i].fd, msg, sizeof(struct ChatMsg), 0);
            flag = 1;
            break;
        }
    }
    if (!flag) {
        memset(msg->msg, 0, sizeof(msg->msg));
        sprintf(msg->msg, "用户< %s> 不在线，或用户名错误!", to);
        msg->type = CHAT_SYS;
        send(fd, msg, sizeof(struct ChatMsg), 0);
    }
}

void do_work(struct User* user) {
    //收到一条信息，并打印
    struct ChatMsg r_msg, msg;
    bzero(&msg, sizeof(msg));
    bzero(&r_msg, sizeof(r_msg));
    recv(user->fd, (void*)&msg, sizeof(msg), 0);
    if (msg.type & CHAT_WALL) {
        printf("<"BLUE"%s"NONE">对所有人说: %s \n", user->name, msg.msg);
        strcpy(msg.name, user->name);
        send_all(&msg);
    } else if (msg.type & CHAT_MSG) {
        char to[20] = {0};
        int i = 1;
        for (;i <= 21;i++) {
            if (msg.msg[i] == ' ')break;
        }
        if (msg.msg[i] != ' ' ||msg.msg[0] != '@') {
            memset(&r_msg, 0, sizeof(r_msg));
            r_msg.type = CHAT_SYS;
            sprintf(r_msg.msg, "私聊格式错误!");
            send(user->fd, (void*)&r_msg, sizeof(r_msg), 0);
        } else {
            msg.type = CHAT_MSG;
            strcpy(msg.name, user->name);
            int j;
            for (j = 0;j + 1 < i;j++) {
                to[j] = msg.msg[j + 1];
            }
            to[j] = '\0';
            DBG(RED"len %d, %s : %s\n"NONE,j, to, msg.msg);
            send_to(to, &msg, user->fd);
        }
    } else if (msg.type & CHAT_FIN) {
        bzero(msg.msg, sizeof(msg.msg));
        msg.type = CHAT_SYS;
        sprintf(msg.msg,"注意：我们的好朋友"BLUE" %s"NONE" 要下线了!\n", user->name);
        strcpy(msg.name, user->name);
        DBG("CHAT_FIN: %s : %s\n",msg.name, msg.msg );
        if (user->team)
            pthread_mutex_lock(&bmutex);
        else 
            pthread_mutex_lock(&rmutex);
        user->online = 0;
        int epollfd = user->team ? bepollfd : repollfd;
        send_all(&msg);
        del_event(epollfd,user->fd);
        if (user->team)
            pthread_mutex_unlock(&bmutex);
        else 
            pthread_mutex_unlock(&rmutex);
        printf(RED"Server Info"NONE" :"BLUE" %s"BLUE" 下线了!\n", user->name);
        close(user->fd);
    } else if (msg.type & CHAT_FUNC) {
        DBG(YELLOW"%s\n", msg.msg);
        if (msg.msg[1] == '2') {
            FILE* fp = fopen("/home/favorsiki/football/common/photo.jpg", "r");
            r_msg.type = CHAT_FUNC;
            strcpy(r_msg.name, msg.name);
            while (fgets(r_msg.msg,512,fp) != NULL) {
                DBG(GREEN"%s"NONE, r_msg.msg);
                send(user->fd, &r_msg, sizeof(struct ChatMsg), 0);
            }
            fclose(fp);
        } else {
            for (int i = 0; i < MAX;i++ ) {
                if (bteam[i].online) {
                    msg.type = CHAT_FUNC;
                    strcpy(msg.msg, bteam[i].name);
                    send(user->fd, &msg, sizeof(struct ChatMsg), 0);
                }
                if (rteam[i].online) {
                    msg.type = CHAT_FUNC;
                    strcpy(msg.msg, rteam[i].name);
                    send(user->fd, &msg, sizeof(struct ChatMsg), 0);
                }
            }
        }
    }
}

void task_queue_init(struct task_queue *taskQueue, int sum, int epollfd) {
    taskQueue->sum = sum;
    taskQueue->epollfd = epollfd;
    taskQueue->team = calloc(sum, sizeof(void *));
    taskQueue->head = taskQueue->tail = 0;
    pthread_mutex_init(&taskQueue->mutex, NULL);
    pthread_cond_init(&taskQueue->cond, NULL);
}

void task_queue_push(struct task_queue *taskQueue, struct User *user) {
    pthread_mutex_lock(&taskQueue->mutex);
    taskQueue->team[taskQueue->tail] = user;
    if (++taskQueue->tail == taskQueue->sum) {
        taskQueue->tail = 0;
    }
    pthread_cond_signal(&taskQueue->cond);
    pthread_mutex_unlock(&taskQueue->mutex);
}


struct User *task_queue_pop(struct task_queue *taskQueue) {
    pthread_mutex_lock(&taskQueue->mutex);
    while (taskQueue->tail == taskQueue->head) {
        pthread_cond_wait(&taskQueue->cond, &taskQueue->mutex);
    }
    struct User *user = taskQueue->team[taskQueue->head];
    if (++taskQueue->head == taskQueue->sum) {
        taskQueue->head = 0;
    }
    pthread_mutex_unlock(&taskQueue->mutex);
    return user;
}

void *thread_run(void *arg) {
    pthread_detach(pthread_self());
    struct task_queue *taskQueue = (struct task_queue *)arg;
    while (1) {
        struct User *user = task_queue_pop(taskQueue);
        do_work(user);
    }
}

