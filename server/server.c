


#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>

#include "config.h"
#include "server.h"


/* 
    create_tcp_listen_socket: 创建一个tcp类型的“监听套接字”
    @ip : 指定一个本地ip
    @port: 指定一个tcp的端口号
    返回值：
        成功  返回创建好的监听套接字描述符(> 0)
        失败  返回-1.
*/
int create_tcp_listen_socket(const char * ip,const char * port)
{
	//1. 创建一个socket套接字
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("failed to socket");
        return -1;
   }


	//当一个网络进程退出后，内核会为它的 地址(ip+端口号) 保存一段时间(几分钟内)
    //如果在这个 几分钟的保存 期间，你想利用这个 ip+端口号， 则需要设置 选项
    //   SO_REUSEADDR 和  SO_REUSEPORT

    //设置选项，允许重用地址
    int on = 1;
    int ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,  (const void*) &on, sizeof(on));
    if (ret == -1)
    {
        perror("failed to setsockopt");
        return -1;
    }

    //设置选项，允许重用端口
    on = 1;
    ret = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT,  (const void*) &on, sizeof(on));
    if (ret == -1)
    {
        perror("failed to setsockopt");
        return -1;
    }

	
    // 2. 指定本机的地址
    struct sockaddr_in  addr;
    memset(&addr, 0, sizeof(addr));
    // bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port =  htons(atoi(port));
    addr.sin_addr.s_addr = inet_addr(ip);

    // 3. bind
    ret = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("bind error");
        return -1;
    }

    // 4. listen
    ret = listen(sock, 5);
    if (ret == -1)
    {
        perror("listen error");
        return -1;
    }
    return sock;
}

/*
    accept_client_conntion: 从“监听套接字”上接收一个客户端的连接请求。
    @sock: 监听套接字
    返回值：
        成功  返回一个与客户端的连接套接字(>0)
        失败  返回-1, errno被设置
*/
int accept_client_conntion(int sock , struct sockaddr_in *client , socklen_t  *addrlen)
{
    struct timeval timeout;
    timeout.tv_sec = 5;
	timeout.tv_usec = 0;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);

    int ret = select(sock + 1, &rfds, NULL, NULL, &timeout);
    if (ret <= 0)
    {
        return -1;
    }
	
    return accept(sock, (struct sockaddr*)client , addrlen);
}


int client_handler(int conn , struct sockaddr_in client)
{
	int flag = 1;
	while(flag)
	{
		//处理:1、client发送了什么命令: ls get 协商好
		//约定数据包有:包头 长度 命令 + ... 包尾 
		//包头包尾:0x88 
		unsigned char ch;
		do	//防止读到的第一个数据不是帧的包头或包尾
		{
			recv(conn , &ch , 1 , 0);
		}
		while(ch != 0x88);

		while(ch == 0x88)	//防止读到的第一个数据是上一帧的包尾
		{
			recv(conn , &ch , 1 , 0);
		}

		unsigned char buf[1024],filename[32];
		int i = 0;
		while(ch != 0x88)	//读取数据
		{
			buf[i++] = ch;
			recv(conn , &ch , 1 , 0);
		}

		int pkg_len = (buf[3] << 24) | 
					  (buf[2] << 16) |
					  (buf[1] << 8)  |
					  (buf[0] << 0);

		//解析数据
		int cmd = (buf[7] << 24) | 
				  (buf[6] << 16) |
				  (buf[5] << 8)  |
				  (buf[4] << 0);
		int str_len;
		switch(cmd)
		{
			case server_ls:	printf("用户 %s 需要查看服务器文件\n",inet_ntoa(client.sin_addr));
							reply_ls(conn);
							break;
			case server_get: str_len = (buf[11] << 24) | 
									   (buf[10] << 16) |
									   (buf[9]  << 8)  |
									   (buf[8]  << 0);
							 for( int j = 0 , i = 12 ; j < str_len ; j++)
							 {
								filename[j] = buf[i++];
							 }
							 printf("用户 %s 需要获取服务器文件 %s\n",inet_ntoa(client.sin_addr) , filename);
							 reply_get(conn , filename);
							 break;
			case server_put: str_len = (buf[11] << 24) | 
									   (buf[10] << 16) |
									   (buf[9]  << 8)  |
									   (buf[8]  << 0);
							 for( int j = 0 , i = 12 ; j < str_len ; j++)
							 {
								filename[j] = buf[i++];
							 }
							 printf("用户 %s 需要上传服务器文件 %s\n",inet_ntoa(client.sin_addr) , filename);
							 reply_put(conn , filename);
							 break;
			case server_bye: printf("用户 %s 准备断开连接\n",inet_ntoa(client.sin_addr));
							 reply_bye(conn);
							 flag = 0;
							 break;
			default:break;
		}
	}
	return 0;
}







void usage(void)
{
	printf("-------------------------------------------------\n");
	printf("----------本服务器运行时需带两个参数-_-!---------\n");
	printf("-------------并且需使用管理员身份运行------------\n");
	printf("-----------格式如./server __ip__ __port__--------\n");
	printf("-------------感谢使用小罗的服务器^_^-------------\n");
	printf("-------------------------------------------------\n");
}








