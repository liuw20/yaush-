/*该文件用来实现内建命令例如作业控制、历史命令查询等*/
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
int main()
{
    int ordercount = 3;
    char orderlist[100][256] = {"history", "10"};
    built_in(ordercount, orderlist);
    return 0;
}
int built_in(int ordercount, char orderlist[100][256])
{
    extern struct passwd *user;
    user = getpwuid(getuid());
    int his_num;                           //用来记录历史命令的数量
    if (strcmp(orderlist[0], "exit") == 0) //如果用户输入了exit，则退出该程序
        exit(0);
    //实现内建命令cd
    else if (strcmp(orderlist[0], "cd") == 0)
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
    }
    else if (strcmp(orderlist[0], "history") == 0)
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
    }
    else if (strcmp(orderlist[0], "jobs") == 0) //实现查看作业功能
    {
        pid_t pid;
        pid = fork();
        if (pid < 0)
        {
            printf(" jobs fork error!");
        }
        else if (pid == 0)
        {
            if (ordercount > 1)
            {
                printf("jobs: input error!");
            }
            else
            {
                execlp("ps", "ps", "ax", NULL);
            }
        }
        else
        { //父进程等待子进程完成
            waitpid(pid, NULL, 0);
        }
    }
    else if (strcmp(orderlist[0],"fg"))
    {
        //该命令实现前台运行程序
        setpgid(orderlist[1],orderlist[1]);
        if ((tcsetpgrp(1,orderlist[1]))==0)//将前台进程组设置为
        {
           kill(orderlist[1],SIGCONT);//向进程发送信号
           waitpid(orderlist[1],NULL,WUNTRACED);

        }
        else
        {
            printf("fg: no such job!");
        }
        
        
    }
    else if (strcmp(orderlist[0],"bg"))
    {//启动在后台挂起的进程
        if( kill(orderlist[1],SIGCONT)<0)
        {
            printf("bg: no such job！");
        }
        else
        {
            waitpid(orderlist[1],NULL,WUNTRACED);
        }
        
    }
}