/*实现打印终端提示符和读取用户输入*/

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
#define NORMAL 0   /* 一般的命令 */
#define OUT_FILE 1 /* 输出重定向 */
#define IN_FILE 2  /* 输入重定向 */
#define PIPE 3     /* 命令中有管道 */

void show_shell(char *prompt);                                               //打印命令提示符
char *get_order(char *prompt);                                               //获取用户输入命令
void explain_order(char *prompt, int *ordercount, char orderlist[100][256]); //对用户输入命令进行分解
void exec_order(int ordercount, char orderlist[100][256]);

int main()
{
    char *prompt = "ls -r tep";
    int count = 0;
    char order[100][256];
    explain_order(prompt, &count, order);

    // show_shell(prompt);
    // get_order(prompt);

    return 0;
}
void exec_order(int ordercount, char orderlist[100][256])
{
    /*该函数用来执行命令*/
    int flag = 0;
    int how = 0;       //用来指示命令中是否含有重定向、管道符号
    int flag_back = 0; //用来指示命令中是否含有后台运行符号
    int status;
    int i;
    int fd;
    char *arg[ordercount + 1];
    char *argnext[ordercount + 1];
    char *file;
    pid_t pid;
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
                printf("input error!");
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
            if (arg[i + 1] == NULL)
                flag++;
        }
        if (strcmp(arg[i], "<") == 0)
        {
            flag++;
            how = IN_FILE;
            if (i == 0)
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
        }
    }
    if (flag > 1)
    {
        printf("input error!");
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
                break;
            }
        }
    default:
        break;
    }
    if ((pid = fork()) < 0) //创建子进程
    {
        printf("fork error!\n"); //创建子进程失败则报错
        return;
    }
    /*对不同的命令分开处理*/
    switch (how)
    {
    case 0:
        if (pid == 0)
        { //子进程处理内建命令
            if (!(find_command(arg[0])))
            {
                printf("%s : command not found\n", arg[0]);
                exit(0);
            }
            execvp(arg[0], arg);
            exit(0);
        }
        break;
    case 1:
        /* 输入的命令中含有输出重定向符> */
        if (pid == 0)
        {
            if (!(find_command(arg[0])))
            {
                printf("%s : command not found\n", arg[0]);
                exit(0);
            }
            fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0644);
            dup2(fd, 1); //标准输出覆盖原先的文件描述符
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
                printf("%s : command not found\n", arg[0]);
                exit(0);
            }
            fd = open(file, O_RDONLY);
            // close(0);
            dup2(fd, 0); //标准输入覆盖原先的文件描述符
            execvp(arg[0], arg);
            exit(0);
        }
        break;
    case 3:
        /* 输入的命令中含有管道符| */
        if (pid == 0)
        {
            int pid2;
            int status2;
            int fd2;

            if ((pid2 = fork()) < 0)
            {
                printf("fork2 error\n");
                return;
            }
            else if (pid2 == 0)
            {
                if (!(find_command(arg[0])))
                {
                    printf("%s : command not found\n", arg[0]);
                    exit(0);
                }
                fd2 = open("/tmp/youdonotknowfile",
                           O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd2, 1);
                execvp(arg[0], arg);
                exit(0);
            }

            if (waitpid(pid2, &status2, 0) == -1)
                printf("wait for child process error\n");

            if (!(find_command(argnext[0])))
            {
                printf("%s : command not found\n", argnext[0]);
                exit(0);
            }
            fd2 = open("/tmp/youdonotknowfile", O_RDONLY);
            dup2(fd2, 0);
            execvp(argnext[0], argnext);

            if (remove("/tmp/youdonotknowfile"))
                printf("remove error\n");
            exit(0);
        }
        break;
    default:
        break;
    }
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
}

// char  *get_order(char *prompt)
// {
//     /*该函数获得用户的指令，并实现TAB键补全,返回输入的指令指针*/
// 	char *buffer;//该指针用于存放用户输入的字符
// 	buffer=readline(prompt);
// 	if (buffer[0]!='\0')
// 	{
// 		add_history(buffer);//该功能实现将用户输入的命令记录下来
// 		write_history(NULL);//将命令写入到~/.history文件当中
// 	}
// 	else
// 	{
// 		exit(-1);
// 	}
//     return prompt;
// }

// void show_shell(char *prompt)
// {
// 	/*实现功能将命令提示符字符串写入到字符数组prompt当中*/

// 	uid_t user_id;
// 	char *ret;
// 	struct passwd *user;
// 	char hostname[100];
// 	char cwd[120];
// 	//获取用户id
// 	user_id=getuid();
// 	// 根据用户uid获得用户相关信息
// 	user=getpwuid(user_id);//获得用户id

// 	gethostname(hostname, 100);//获得主机名称
// 	sprintf(prompt,"%s@%s:",user->pw_name,hostname);
// 	getcwd(cwd, 120);//获取当前工作目录
// 	int len_pro;
// 	len_pro=strlen(prompt);
// 	if (strcmp(cwd,user->pw_dir) == 0)
// 	{
// 		sprintf(prompt+len_pro,"%s",cwd);  // 家目录特殊处理
// 	}
// 	else
// 	{
// 		sprintf(prompt+len_pro,"~%s",cwd+strlen(user->pw_dir));
// 	}

// 	len_pro=strlen(prompt);
// 		/* 打印用户提示符 */
// 	if (0 == user_id)//对普通用户和超级用户进行区分
// 	{
// 	sprintf(prompt+len_pro,"# ");		// 超级用户

// 	}
// 	else
// 	{
// 	sprintf(prompt+len_pro,"$ ");		// 普通用户
// 	}
// 	// puts(prompt);

// 	// fflush(stdout);			// 刷新终端提示符
// }
