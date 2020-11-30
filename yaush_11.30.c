/*打印终端命令提示符的函数*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define max 1024
void show_shell()
{
	uid_t user_id;
	char *ret;
	struct passwd *user;
	char hostname[100];
	char cwd[120];
	//获取用户id
	user_id=getuid();
	// 根据用户uid获得用户相关信息
	user=getpwuid(user_id);//获得用户id
	gethostname(hostname, 100);//获得主机名称
	getcwd(cwd, 120);//获取当前工作目录
	if (strcmp(cwd,user->pw_dir) == 0)
	{
		printf("~"); 			  // 家目录特殊处理
	}
	else
	{
		ret = strrchr(cwd,'/');   // 获取路径的最后一个目录
		if (ret[1] == '\0')
		{
			printf("/]");		  // 根目录
		}
		else
		{
			printf("%s]",ret+1);//获得该字符下一个位置的字符
		}
		/* 打印用户提示符 */
		if (0 == user_id)//对普通用户和超级用户进行区分
		{
		printf("# ");		// 超级用户
		}
		else
		{
		printf("$ ");		// 普通用户
		}

		fflush(stdout);			// 刷新终端提示符
		}

		}




	
}





