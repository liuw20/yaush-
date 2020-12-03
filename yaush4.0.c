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
#define NORMAL 0                                                             /* 一般的命令 */
#define OUT_FILE 1                                                           /* 输出重定向 */
#define IN_FILE 2                                                            /* 输入重定向 */
#define PIPE 3                                                               /* 命令中有管道 */
#define PIPE_RE 4                                                            /*管道后面存在重定向操作*/
void show_shell(char *prompt);                                               //打印命令提示符
char *get_order(char *prompt);                                               //获取用户输入命令
void explain_order(char *prompt, int *ordercount, char orderlist[100][256]); //对用户输入命令进行分解
void exec_order(int ordercount, char orderlist[100][256]);
int find_command(char *command);
int main()
{
    int ordercount = 6;
    char orderlist[100][256] = {"ls", "-l", "|", "wc", ">", "liuw.txt"};
    exec_order(ordercount, orderlist);
    return 0;
}

void exec_order(int ordercount, char orderlist[100][256])
{
    /*该函数用来执行命令*/
    int flag = 0;      //用来判断输入命令是否符合要求
    int how = 0;       //用来指示命令中是否含有重定向、管道符号
    int flag_back = 0; //用来指示命令中是否含有后台运行符号
    int status;
    int i;
    int fp;
    int fd[2];
    char *arg[ordercount + 1];
    char *argnext[ordercount + 1];
    char *file;
    pid_t pid;
    int flag_re; //用来记录是否在|后面的指令是否存在重定向操作
    /*将输入的命令取出*/
    for (int i = 0; i < ordercount; i++)
    {
        arg[i] = (char *)orderlist[i];
    }
    arg[ordercount] = NULL;
    /*对输入的命令进行语法分析，判断是否存在重定向、管道、后台等符号*/
    //查找是否存在后台运行符
    for (int i = 0; i < ordercount; i++)
    {
        if (strncmp(arg[i], "&", 1) == 0)
        {
            if (i == ordercount - 1)
            {
                flag_back = 1; //如果检索出存在&后台运行程序的符号，则置1
                arg[ordercount - 1] = NULL;
                break;
            }
            else //如果没有出现在命令末尾则提示出错
            {
                printf(" back exec command  error!");
                return;
            }
        }
    }
    for (int i = 0; arg[i] != NULL; i++)
    {
        /*检索是否存在管道、重定向操作,flag用来记录不符合的命令输入*/
        if (strcmp(arg[i], ">") == 0)
        {
            flag++;
            how = OUT_FILE;
            if (arg[i + 1] == NULL) //输入格式错误
                flag++;
        }
        if (strcmp(arg[i], "<") == 0)
        {
            flag++;
            how = IN_FILE;
            if (i == 0) //输入格式错误
                flag++;
        }
        if (strcmp(arg[i], "|") == 0)
        {
            flag++;
            how = PIPE;
            if (arg[i + 1] == NULL)
                flag++;
            if (i == 0)
                flag++;
            break;
        }
    }
    if (flag > 1)
    {
        printf("input command error!");
        return;
    }
    switch (how)
    {
    case OUT_FILE:
        for (int i = 0; arg[i] != NULL; i++)
        {
            if (strcmp(arg[i], ">") == 0)
            {
                file = arg[i + 1]; //记录输出文件
                arg[i] = NULL;
            }
        }
        break;
    case IN_FILE:
        for (int i = 0; arg[i] != NULL; i++)
        {
            if (strcmp(arg[i], "<") == 0)
            {
                file = arg[i + 1]; //记录输出文件
                arg[i] = NULL;
            }
        }
        break;
    case PIPE: //把管道后面的部分存入到argnext中
      if (pipe(fd) < 0) //创建管道
        {
            printf("pipe error\n"); //创建失败则提示
            exit(0);
        }
        for (int i = 0; arg[i] != NULL; i++)
        {
            if (strcmp(arg[i], "|") == 0)
            {
                arg[i] = NULL; //传递NULL
                int j;
                for (j = i + 1; arg[j] != NULL; j++)
                {
                    argnext[j - i - 1] = arg[j];
                }
                argnext[j - i - 1] = arg[j]; //传递NULL

                for (int k = 0; argnext[k] != NULL; k++)
                {
                    if (strcmp(argnext[k], ">") == 0)
                    {
                        flag_re = PIPE_RE; //如果在管道后的指令出现了>指令，则记录
                        argnext[k] = NULL;
                        file = argnext[k + 1]; //记录输出文件
                    }
                }

                break;
            }
        }
    default:
        break;
    }
    if ((pid = fork()) < 0) //创建子进程，用来处理用户指令
    {
        printf("fork error!\n"); //创建子进程失败则报错
        return;
    }
    /*利用子进程对不同的命令分开处理*/
    switch (how)
    {
    case 0:
        if (pid == 0)
        {
            if (!(find_command(arg[0])))
            {
                printf("%s : command not found\n", arg[0]); //找不到命令则报错
                exit(0);
            }
            execvp(arg[0], arg); //执行命令
            exit(0);
        }
        break;
    case 1:
        /* 输入的命令中含有输出重定向符> */
        if (pid == 0)
        {
            if (!(find_command(arg[0])))
            {
                printf("%s : command not found\n", arg[0]); //找不到命令则报错
                exit(0);
            }
            fp = open(file, O_RDWR | O_CREAT | O_TRUNC, 0644);
            close(STDOUT_FILENO);
            dup2(fp, 1); //利用fp覆盖标准输出
            execvp(arg[0], arg);
            exit(0);
        }
        break;
    case 2:
        /* 输入的命令中含有输入重定向符< */
        if (pid == 0)
        {
            if (!(find_command(arg[0])))
            {
                printf("%s : command not found\n", arg[0]); //找不到命令则报错
                exit(0);
            }
            fp = open(file, O_RDONLY);
            close(STDIN_FILENO); //关闭标准输入
            dup2(fp, 0);         //利用该文件描述符覆盖掉标准输出
            execvp(arg[0], arg);
            exit(0);
        }
        break;
    case 3:
        /* 输入的命令中含有管道符| */
        if (pid > 0)
        {//对父进程进行操作
            int pid2;
            int status2;
            int fd2;
            if ((pid2 = fork()) < 0)
            { //再次创建子进程用来处理管道符号后面的指令
                printf("fork2 error\n");
                return;
            }
            else if (pid2 == 0)
            {//对创建的子进程进行操作
                close(fd[1]);              //关闭写操作
                close(STDIN_FILENO);       //关闭标准输入
                dup2(fd[0], STDIN_FILENO); //将fd[0]读操作覆盖标准输入
                close(fd[0]);
                if (flag_re )
                {
                    int out_fd;
                    out_fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    close(STDOUT_FILENO);
                    dup2(out_fd, STDOUT_FILENO); //将该文件描述符覆盖标准输出
                    close(out_fd);
                }

                if (!(find_command(argnext[0])))
                {
                    printf("%s : pipe later command not found\n", argnext[0]);
                    exit(0);
                }
                execvp(argnext[0], argnext);
            }
            else
            {//pid2的父进程
                close(fd[0]);
                close(fd[1]);
                wait(pid2, NULL, 0);
            }
        }
        else if (pid == 0)
        {
            close(fd[0]);
            close(STDOUT_FILENO);       //关闭标准输出
            dup2(fd[1], STDOUT_FILENO); //将管道fd[1]写操作覆盖标准输出
            close(fd[1]);
            if (!(find_command(arg[0])))
            {
                printf("%s : pipe front command not found\n", arg[0]);
                exit(0);
            }
            i=execvp(arg[0], arg); //该子进程执行前面的指令
            if (i==-1)
            {
                printf("未找到该命令！");
            }
            exit(0);
        }
        break;

        //     if (waitpid(pid2, &status2, 0) == -1)
        //         printf("wait for child process error\n");

        //     if (!(find_command(argnext[0])))
        //     {
        //         printf("%s : command not found\n", argnext[0]);
        //         exit(0);
        //     }
        //     fd2 = open("/tmp/youdonotknowfile", O_RDONLY);
        //     dup2(fd2, 0);
        //     execvp(argnext[0], argnext);

        //     if (remove("/tmp/youdonotknowfile"))
        //         printf("remove error\n");
        //     exit(0);
        // }
        // break;
    default:
        break;
    }
    if (flag_back == 1)
    { //如果有“&”则后台执行
        printf("[process id %d]\n", pid);
        return;
    }

    // /* 父进程等待子进程结束 */
    // if (waitpid(pid, &status, 0) == -1)
    //     printf("wait for child process error\n");
}

int find_command(char *command)
{
    DIR *dp;
    struct dirent *dirp;
    char *path[] = {"./", "/bin", "/usr/bin", NULL};

    /* 使当前目录下的程序可以被运行，如命令"./fork"可以被正确解释和执行 */
    if (strncmp(command, "./", 2) == 0)
        command = command + 2;

    /* 分别在当前目录、/bin和/usr/bin目录查找要可执行程序 */
    int i = 0;
    while (path[i] != NULL)
    {
        if ((dp = opendir(path[i])) == NULL)
            printf("can not open /bin \n");
        while ((dirp = readdir(dp)) != NULL)
        {
            if (strcmp(dirp->d_name, command) == 0)
            {
                closedir(dp);
                return 1;
            }
        }
        closedir(dp);
        i++;
    }
    return 0;
}