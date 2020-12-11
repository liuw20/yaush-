//该文档为最终的程序调试版
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
#include"readline.h"
#include"history.h"
#define NORMAL 0   /* 一般的命令 */
#define OUT_FILE 1 /* 输出重定向 */
#define IN_FILE 2  /* 输入重定向 */
#define PIPE 3     /* 命令中有管道 */
#define PIPE_RE 4  /*管道后面存在重定向操作*/
struct passwd *user;
pid_t  PID[1024]; //后台执行的进程，默认上限为1024
char *HIS[20];

void show_shell(char *prompt);                                               //打印命令提示符
char *get_order(char *prompt);                                               //获取用户输入命令
void explain_order(char *prompt, int *ordercount, char orderlist[100][256]); //对用户输入命令进行分解
void exec_order(int ordercount, char orderlist[100][256]);//执行分解后的命令
int find_command(char *command);//在环境变量中寻找非内建命令
int built_in(int ordercount, char orderlist[100][256]);//对部分内建命令进行执行
void show_history(int hisnum);//历史命令查询

int main(int argc, char **argv)
{
    //主程序
	int    i;
	int    ordercount = 0;
	char   orderlist[100][256];
	char   **arg = NULL;
	char   shell[1024];//用户提示符
    char *buffer=NULL;//存储命令

    buffer=malloc(sizeof(char)*256);
    for (i = 0; i < 1024; i++)
	{ //初始化后台运行进程表
		PID[i] = 0;
	}
    read_history(NULL);//读取历史命令
	while(1) {
        memset(buffer,0,256);
		show_shell(shell);//显示前缀
		buffer=get_order(shell);//获得命令输入
	
        if (strcmp(buffer,"")==0)
        {
            continue;//输入空字符串跳过
        }
		if( strcmp(buffer,"exit\n") == 0 || strcmp(buffer,"logout\n") == 0 )
			break;	// 若输入的命令为exit或logout则退出本程序 
		for (i=0; i < 100; i++)
		{
			orderlist[i][0]='\0';
		}
		ordercount = 0;
		explain_order(buffer, &ordercount, orderlist); //对输入的命令进行拆分
        if(built_in(ordercount,orderlist))
        {
            //如果是内建命令则直接执行
            continue;
        }
		exec_order(ordercount,orderlist);//执行命令
	}
 
	if(buffer != NULL) {
		free(buffer);
		buffer= NULL;
	}
 
	exit(0);
}
void show_shell(char *prompt)
{	/*实现功能将命令提示符字符串写入到字符数组prompt当中*/
    extern struct passwd *user;
	char hostname[256];
	char pathname[1024];
	int l=0;
	user = getpwuid(getuid()); //用户名
	getcwd(pathname, 1024);	  //路径
	sprintf(prompt,"\001\033[49;31;1m\002[liuw20]\001\033[0m\002");
	l = strlen(prompt);
	if (gethostname(hostname, 256) == 0)														   //获得用户名和主机名
		sprintf(prompt + l, "%s@%s:", user->pw_name, hostname); 
	else
		sprintf(prompt + l, "%s@unknown", user->pw_name);

	l = strlen(prompt);

	if (strlen(pathname) < strlen(user->pw_dir) || strncmp(pathname, user->pw_dir,
														  strlen(user->pw_dir)) != 0) //获得路径
		sprintf(prompt + l, "%s", pathname);	
	else
		sprintf(prompt + l, "~%s", pathname + strlen(user->pw_dir)); //省略home路径

	l = strlen(prompt);

	if (geteuid() == 0)			   //检查是否是root
		sprintf(prompt + l, "# "); //root
	else
		sprintf(prompt + l, "$ "); //comm

	return;
	
}
char  *get_order(char *prompt)
{
    /*该函数获得用户的指令，并实现TAB键补全,返回输入的指令指针*/
	char *buffer;//该指针用于存放用户输入的字符
	buffer=readline(prompt);
	if (buffer[0]!='\0')
	{
		add_history(buffer);//该功能实现将用户输入的命令记录下来
		write_history(NULL);//将命令写入到~/.history文件当中
	}
    else 
	{
       return buffer;
	}
    int l=strlen(buffer);
    buffer[l]='\n';
    l++;
    buffer[l]='\0';  
    return buffer;
}
void explain_order(char *prompt, int *ordercount, char orderlist[100][256])
{
	/*该函数实现对命令的分解，并将以空格分隔的字符串存入到字符数组当中来*/
	char *p = prompt;
	char *q = prompt;
	int number = 0;
	while (1)
	{
		if (p[0] == '\n') //检测到输入回车则退出
			break;

		if (p[0] == ' ') //跳过空格，向后读取
			p++;
		else
		{ //对非空格字符进行处理
			q = p;
			number = 0;
			while ((q[0] != ' ') && (q[0] != '\n'))
			{
				number++; //记录两空格之间的命令的长度
				q++;
			}
			strncpy(orderlist[*ordercount], p, number + 1); //将number+1个字符放到orderlist当中
			orderlist[*ordercount][number] = '\0';//命令后面添加0字符
			*ordercount = *ordercount + 1;
			p = q;
		}
	}
    
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
                printf("%s : normal command not found\n", arg[0]); //找不到命令则报错
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
                printf("%s : output < command not found\n", arg[0]); //找不到命令则报错
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
                printf("%s : input > command not found\n", arg[0]); //找不到命令则报错
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
            waitpid(pid,NULL,0);
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

      
    default:
        break;
    }
    HIST_ENTRY **his_t;
	his_t = history_list();
    if (flag_back == 1)
    { //如果有“&”则后台执行，并将后台执行的命令放到历史命令数组当中
        		int k;
				for (k = 0; k <1024; k++)
				{
					if (PID[k] == 0)
					{
						PID[k] = pid;
						printf("[%d] %u\n", k + 1, pid);//返回进程号

						int l = 0;
						while (his_t[l] != 0)
						{
							l++;
						}
						l = l - 1;
						HIS[k] = his_t[l]->line;
						break;
					}
				}
				if (k == 1024)
				{
					printf("后台进程数达到上限");
				}
			

        return;
    }

    /* 父进程等待子进程结束 */
    if (waitpid(pid, &status, 0) == -1&&how!=3)
        printf("wait for child process error\n");
}

int find_command(char *command)
{
    DIR *dp;
    struct dirent *dirp;
    char *path[] = {"./", "/bin", "/usr/bin",NULL};

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
        if (strcmp(orderlist[1],"~")==0)
        { //对家目录进行处理
            cd_path = malloc(strlen(user->pw_dir) + strlen(orderlist[1]));
            if (cd_path == NULL)
            {
                printf("cd : malloc failed!");
            }
            strcpy(cd_path, user->pw_dir); //将家目录复制到当前路径
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
        if (orderlist[1][0] != '\0')
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
        if ((tcsetpgrp(1, pid)) == 0) //将前台进程组设置为1
        {
            printf("%s",HIS[0]);
            kill(pid, SIGCONT); //向进程发送信号
            
            waitpid(pid, NULL, WUNTRACED);
        }
        else
        {
            printf("fg: no such job!\n");
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
            printf("bg: no job assigned\n"); //打印提示信息
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

void show_history(int hisnum)
{ //显示历史命令
	int i = 0;
	HIST_ENTRY **his;
	his = history_list();
	if (hisnum == 0)
	{
		while (his[i] != 0)
		{
			printf(" %d  %s\n", i, his[i]->line);
			i++;
		}
	}
	else
	{
		while (his[i] != 0)
		{
			i++;
		}
		i = i - hisnum;
		while (hisnum)
		{
			printf(" %d  %s\n", i, his[i]->line);
			i++;
			hisnum--;
		}
	}
}