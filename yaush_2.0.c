/*实现打印终端提示符和读取用户输入*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include  <readline/readline.h>
#include <readline/history.h>
#define SIZE 1024
void show_shell(char *prompt);//打印命令提示符
char  *get_order(char *prompt);//获取用户输入命令
void explain_order(char *prompt,int *ordercount,char orderlist[100][256]);//对用户输入命令进行分解

int  main() 
{
    char *prompt="ls -r tep";
	int  count=0;
	char order[100][256];
	explain_order(prompt,&count,order);
	
	
	// show_shell(prompt);
    // get_order(prompt);
	
	return 0;
}

void explain_order(char *prompt,int *ordercount,char orderlist[100][256])
{
	/*该函数实现对命令的分解，并将以空格分隔的字符串存入到字符数组当中来*/
	char	*p	= prompt;
	char	*q	= prompt;
	int	number	= 0;
	while (1) {
		if ( p[0] == '\n' )//检测到输入回车则退出
			break;
 
		if ( p[0] == ' '  )//跳过空格，向后读取
			p++;
		else {//对非空格字符进行处理
			q = p;
			number = 0;
			while( (q[0]!=' ') && (q[0]!='\n') ) {
				number++;//记录两空格之间的命令的长度
				q++;
			}
			strncpy(orderlist[*ordercount], p, number+1);//将number+1个字符放到orderlist当中
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





