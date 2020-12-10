#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include "readline.h"
#include "history.h"
#define NORMAL 0   /* 一般的命令 */
#define OUT_FILE 1 /* 输出重定向 */
#define IN_FILE 2  /* 输入重定向 */
#define PIPE 3     /* 命令中有管道 */
#define PIPE_RE 4  /*管道后面存在重定向操作*/
struct passwd *user;
pid_t PID[1024]; //后台执行的进程，默认上限为1024
char *HIS[20];
int built_in(int ordercount, char orderlist[100][256]);
void show_history(int hisnum);
int main(int argc, char **argv)
{
    int count = 1;
    char order[100][256] = {"history"};
    built_in(count, order);
    return 0;
}

//该文件用来完善内建命令
int built_in(int ordercount, char orderlist[100][256])
{
    extern struct passwd *user;
    user = getpwuid(getuid());
    int his_num;                           //用来记录历史命令的数量
    if (strcmp(orderlist[0], "exit") == 0) //如果用户输入了exit，则退出该程序
        exit(0);
    //实现内建命令cd
    if (strcmp(orderlist[0], "cd") == 0)
    {
        char *cd_path = NULL;
        if (orderlist[1][0] == '~')
        { //对家目录进行处理
            cd_path = malloc(strlen(user->pw_dir) + strlen(orderlist[1]));
            if (cd_path == NULL)
            {
                printf("cd : malloc failed!");
            }
            strcpy(cd_path, user->pw_dir); //将当前目录复制到当前路径
            strncpy(cd_path + strlen(user->pw_dir), orderlist[1] + 1, strlen(orderlist[1]));
            return 1;
        }
        else
        {
            cd_path = malloc(strlen(orderlist[1] + 1));
            if (cd_path == NULL)
            {
                printf("cd:  malloc failed!");
            }
            strcpy(cd_path, orderlist[1]);
        }
        if (chdir(cd_path) != 0)
        {
            printf("cd:%s:%s\n", cd_path, strerror(errno));
        }
        return 1;
    }
    if (strcmp(orderlist[0], "history") == 0)
    {
        if (orderlist[1] != NULL)
        {
            his_num = atoi(orderlist[1]);
            show_history(his_num);
        }
        else
        {
            his_num = 0;
            show_history(his_num);
        }
        return 1;
    }
    //实现作业查看
    if (strcmp(orderlist[0], "jobs") == 0) 
    {       int jobs_last=1;
            printf("[%d]+  	运行中		%s\n", jobs_last, HIS[jobs_last - 1]);
        return 1;
    }
    //实现前台控制
    if (strcmp(orderlist[0], "fg") == 0)
    {
        pid_t pid;
        if (orderlist[1] != NULL)
        {
            pid = atoi(orderlist[1]);
        }
        else
        {
            printf("fg: no job assigned\n");
            return 1;
        }
        //该命令实现前台运行程序
        setpgid(pid, pid);
        if ((tcsetpgrp(1, pid)) == 0) //将前台进程组设置为
        {
            kill(pid, SIGCONT); //向进程发送信号
            waitpid(pid, NULL, WUNTRACED);
        }
        else
        {
            printf("fg: no such job!");
        }
        return 1;
    }
    //实现后台运行程序
    if (strcmp(orderlist[0], "bg") == 0)
    {
        pid_t pid;
        if (orderlist[1] != NULL)
        {
            pid = atoi(orderlist[1]); //用atoi转换，获取pid
        }
        else
        {                                             //只有一个bg
            printf("myshell: bg: no job assigned\n"); //打印提示信息
            return 1;
        }
        //启动在后台挂起的进程
        if (kill(pid, SIGCONT) < 0)
        {
            printf("bg: no such job！");
        }
        else
        {
            waitpid(pid, NULL, WUNTRACED);
        }
        return 1;
    }
        return 0;
}
