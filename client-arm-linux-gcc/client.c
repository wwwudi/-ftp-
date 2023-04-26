
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"




/* 
    create_tcp_connection_socket: 创建一个tcp类型的“套接字”
    @ip : 指定一个本地ip
    @port: 指定一个tcp的端口号
    返回值：
        成功  返回创建好的套接字描述符(> 0)
        失败  返回-1.
*/
int init_connection(const char * ip,const char * port)
{
	//1、指定套接字描述符
	int socket_fd = socket(AF_INET,SOCK_STREAM,0);
	if(socket_fd == -1)
	{
		perror("failed to socket");
		return -1;
	}

	//2、connect
	struct sockaddr_in Serveraddr;
	memset(&Serveraddr , 0 ,sizeof(Serveraddr));
	Serveraddr.sin_family = AF_INET;
	Serveraddr.sin_port = htons(atoi(port));
	Serveraddr.sin_addr.s_addr = inet_addr(ip);
	int ret = connect(socket_fd , (struct sockaddr *)&Serveraddr , sizeof(Serveraddr));
	if(ret == -1)
	{
		perror("failed to connect");
		return -1;
	}

	return socket_fd;
}


void send_cmd_server(int socket_fd)
{
	unsigned char cmd[32];
	while(1)
	{
		int snd_cmd;
		
		printf("Please enter command:");
		setbuf(stdin,NULL);
		scanf("%s",cmd);
		setbuf(stdin,NULL);
		putchar('\n');
		
		printf("The %s operation is being requested...\n",cmd);
		if( strcmp(cmd , "ls") == 0 )
		{
			snd_cmd = server_ls;
			if(!cmd_send(socket_fd , snd_cmd , NULL))
			{
				ls_recv(socket_fd);
			}
		}
		else if( strcmp(cmd , "get") == 0 )
		{
			unsigned char filename[1024];
			snd_cmd = server_get;
			scanf("%s",filename);
			if(!cmd_send(socket_fd , snd_cmd , filename))
			{
				get_recv(socket_fd ,filename);
			}
		}
		else if( strcmp(cmd , "put") == 0 )
		{
			unsigned char filename[1024];
			snd_cmd = server_put;
			scanf("%s",filename);
			if(!cmd_send(socket_fd , snd_cmd , filename))
			{
				put_recv(socket_fd ,filename);
			}
		}
		else if( strcmp(cmd , "bye") == 0 )
		{
			snd_cmd = server_bye;
			cmd_send(socket_fd , snd_cmd , NULL);
		}
		else 
		{
			printf("The input command command is incorrect please reenter\n");
			printf("The currently supported orders are ls , get , put and bye\n");
			continue;
		}

		
		if(snd_cmd == server_bye)break;
	}
	close(socket_fd);
}

void usage(void)
{
	printf("------------------------------------------------------------------------------\n");
	printf("-----------The two parameters are required when the program runs-_-!----------\n");
	printf("----------------------format: ./client __ip__ __port__------------------------\n");
	printf("-------------------Thank you for using xiaoluo's program^_^-------------------\n");
	printf("------------------------------------------------------------------------------\n");
}


