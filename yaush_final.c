/*该文件用来整合各个模块
author: liuwei
data: 2020.12.12*/
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
pid_t PID[1024]; //后台执行的进程，默认上限为1024
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
        HIST_ENTRY **his_t;
        his_t=history_list();
        memset(buffer,0,256);
		show_shell(shell);//显示前缀
		buffer=get_order(shell);//获得命令输入
		/* 若输入的命令为exit或logout则退出本程序 */
		if( strcmp(buffer,"exit\n") == 0 || strcmp(buffer,"logout\n") == 0 )
			break;
		for (i=0; i < 100; i++)
		{
			orderlist[i][0]='\0';
		}
		ordercount = 0;
        //  char *tem="ls -l tep";
        // puts(tem);
		explain_order(buffer, &ordercount, orderlist); //对输入的命令进行拆分
        //  printf("%d",ordercount);
        // for (int j = 0; j< ordercount;j++)
        // {
        //     printf("%s\n",orderlist[i]);
        // }
        // ordercount=6;
        //  char orderlist[100][256] = {"ls", "-l", "|", "wc", ">", "liuw.txt"};
        if(built_in(ordercount,orderlist))
        {
            //如果是内建命令则直接执行
            continue;
        }
		exec_order(ordercount,orderlist);//执行命令
	}
 
	if(buffer != NULL||shell!=NULL) {
		free(buffer);
        free(shell);
		buffer= NULL;
        // shell=NULL;
	}
 
	exit(0);
}
void show_shell(char *prompt)
{	/*实现功能将命令提示符字符串写入到字符数组prompt当中*/
    extern struct passwd *user;
	char hostname[256];
	char pathname[1024];
	int l;
	user = getpwuid(getuid()); //用户名
	getcwd(pathname, 1024);	  //路径

	sprintf(prompt, "\001\033[49;31;1m\002[liuw20]\001\033[0m\002"); 
	l = strlen(prompt);

	if (gethostname(hostname, 256) == 0)														   //获得用户名和主机名
		sprintf(prompt + l, "\001\033[49;32;1m\002%s@%s:\001\033[0m\002", user->pw_name, hostname); //green 32
	else
		sprintf(prompt + l, "\001\033[49;32;1m\002%s@unknown:\001\033[0m\002", user->pw_name);

	l = strlen(prompt);

	if (strlen(pathname) < strlen(user->pw_dir) || strncmp(pathname, user->pw_dir,
														  strlen(user->pw_dir)) != 0) //获得路径
		sprintf(prompt + l, "\001\033[49;34;1m\002%s\001\033[0m\002", pathname);	 //此目录为home目录，home路径不省略,34 blu
	else
		sprintf(prompt + l, "\001\033[49;34;1m\002~%s\001\033[0m\002", pathname + strlen(user->pw_dir)); //省略home路径

	l = strlen(prompt);

	if (geteuid() == 0)			   //检查是否是root
		sprintf(prompt + l, "# "); //root
	else
		sprintf(prompt + l, "$ "); //comm

	// uid_t user_id;
	// char *ret;
	// extern  struct passwd *user;
	// char hostname[100];
	// char cwd[120];
	// //获取用户id
	// user_id=getuid();
	// // 根据用户uid获得用户相关信息
	// user=getpwuid(user_id);//获得用户id

	// gethostname(hostname, 100);//获得主机名称
	// sprintf(prompt,"%s@%s:",user->pw_name,hostname); 
	// getcwd(cwd, 120);//获取当前工作目录
	// int len_pro;
	// len_pro=strlen(prompt);
	// if (strcmp(cwd,user->pw_dir) == 0)
	// {
	// 	sprintf(prompt+len_pro,"%s",cwd);  // 家目录特殊处理
	// }
	// else
	// {
	// 	sprintf(prompt+len_pro,"~%s",cwd+strlen(user->pw_dir));
	// }

	// len_pro=strlen(prompt);
	// 	/* 打印用户提示符 */
	// if (0 == user_id)//对普通用户和超级用户进行区分
	// {
	// sprintf(prompt+len_pro,"# ");		// 超级用户
	// }
	// else
	// {
	// sprintf(prompt+len_pro,"$ ");		// 普通用户
	// }

	return;
	
	

	// fflush(stdout);			// 刷新终端提示符
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
		exit(-1);
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
			orderlist[*ordercount][number] = '\0';
			*ordercount = *ordercount + 1;
			p = q;
		}
	}
    // printf("%d\n",*ordercount);
    // for (int i = 0; i < *ordercount; i++)
    // {
    //     printf("%s\n",orderlist[i]);
    // }
    
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
        return 1;
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
        return 1;
    }
    // else if (strcmp(orderlist[0],"fg"))
    // {
    //     //该命令实现前台运行程序
    //     setpgid(orderlist[1],orderlist[1]);
    //     if ((tcsetpgrp(1,orderlist[1]))==0)//将前台进程组设置为
    //     {
    //        kill(orderlist[1],SIGCONT);//向进程发送信号
    //        waitpid(orderlist[1],NULL,WUNTRACED);

    //     }
    //     else
    //     {
    //         printf("fg: no such job!");
    //     }
        
        
    // }
    // else if (strcmp(orderlist[0],"bg"))
    // {//启动在后台挂起的进程
    //     if( kill(orderlist[1],SIGCONT)<0)
    //     {
    //         printf("bg: no such job！");
    //     }
    //     else
    //     {
    //         waitpid(orderlist[1],NULL,WUNTRACED);
    //     }
        
    // }
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