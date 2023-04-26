

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "client.h"


#define SERVER_PKGPATH "/home/xiaoluo/ftp"




int reply_ls(int conn)
{
	//打开目录
	DIR *mytftp = opendir(SERVER_PKGPATH);
	if(mytftp == NULL)
	{
		perror("opendir failed:");
		return -1;
	}
	//读取目录
	struct dirent *dirp = NULL;
	unsigned char filename[1024] = {0};
	unsigned char re_pkg[2048] = {0};
	int pkg_len,r=0,i=0,ret;
	while(dirp = readdir(mytftp))
	{
		if(!strcmp(dirp->d_name , ".") || !strcmp(dirp->d_name , ".."))
		{
			continue;
		}
		//以空格分隔
		r+=sprintf( filename + r , "%s " , dirp->d_name);		
	}
	//包头
	re_pkg[i++] = 0x88;

	//包长度
	pkg_len = 4 + 4 + 4 +r + 1;
	re_pkg[i++] = (pkg_len) >> 0  & 0xff;
	re_pkg[i++] = (pkg_len) >> 8  & 0xff;
	re_pkg[i++] = (pkg_len) >> 16 & 0xff;
	re_pkg[i++] = (pkg_len) >> 24 & 0xff;
	
	//命令号
	re_pkg[i++] = (server_ls) >> 0  & 0xff;
	re_pkg[i++] = (server_ls) >> 8  & 0xff;
	re_pkg[i++] = (server_ls) >> 16 & 0xff;
	re_pkg[i++] = (server_ls) >> 24 & 0xff;

	//回复文件名总长度
	re_pkg[i++] = (r + 1) >> 0  & 0xff;
	re_pkg[i++] = (r + 1) >> 8  & 0xff;
	re_pkg[i++] = (r + 1) >> 16 & 0xff;
	re_pkg[i++] = (r + 1) >> 24 & 0xff;

	//总的文件名
	for( int j = 0 ; j < r + 1; j++ )
	{
		re_pkg[i++] = filename[j];
	}
	
	//包尾
	re_pkg[i++] = 0x88;					
	ret = send(conn , re_pkg , i , 0);
	if(ret <= 0)
	{
		perror("ls_send failed:");
		return -1;
	}
	else
	{
		printf("成功发送服务器文件列表给用户\n");
	}
	return 0;
}


int reply_get(int conn , char *filename)
{
	unsigned char filepath[1024];
	unsigned char re_pkg[4096] = {0};
	unsigned char flag , Isfinsh;
	int pkg_len = 4 + 4 + 1,size = 0,i = 0,ret;
	sprintf(filepath , "%s/%s" , SERVER_PKGPATH , filename);
	//打开用户需求的文件
	int fd = open(filepath , O_RDONLY);
	if(fd == -1)
	{
		printf("用户需求的文件不存在\n");
		flag = 0;
	}
	else
	{
		flag = 1;
		size = lseek(fd, 0, SEEK_END);
		printf("文件大小 %d Betys\n" , size);
		//将光标定在文件开头
		lseek(fd, 0 , SEEK_SET);		
	}
	
	//包头
	re_pkg[i++] = 0x88;

	//包长度		4
	if(flag == 1)
	{
		pkg_len += 4;
	}
	re_pkg[i++] = (pkg_len) >> 0  & 0xff;
	re_pkg[i++] = (pkg_len) >> 8  & 0xff;
	re_pkg[i++] = (pkg_len) >> 16 & 0xff;
	re_pkg[i++] = (pkg_len) >> 24 & 0xff;
	
	//命令号		4
	re_pkg[i++] = (server_ls) >> 0  & 0xff;
	re_pkg[i++] = (server_ls) >> 8  & 0xff;
	re_pkg[i++] = (server_ls) >> 16 & 0xff;
	re_pkg[i++] = (server_ls) >> 24 & 0xff;

	//文件是否存在	1
	re_pkg[i++] = flag;
	if(flag == 1)
	{
		//回复文件内容总长度	4
		printf("file_len = %d\n",size);
		re_pkg[i++] = (size) >> 0	& 0xff;
		re_pkg[i++] = (size) >> 8	& 0xff;
		re_pkg[i++] = (size) >> 16 & 0xff;
		re_pkg[i++] = (size) >> 24 & 0xff;
	}
	
	//包尾
	re_pkg[i++] = 0x88;	

	//预发送
	ret = send(conn , re_pkg , i , 0);

	//文件存在则发送
	if(flag == 1)
	{
		unsigned char ch;
		while(read(fd , &ch , 1) == 1)
		{
			ret = send(conn , &ch , 1 , 0);
		}
		close(fd);
	}
	
	if(ret <= 0)
	{
		perror("ls_send failed:");
		return -1;
	}
	else if(flag == 1)
	{
		printf("成功发送服务器文件给用户\n");
	}
	else if(flag == 0)
	{
		printf("用户需求的文件不存在,发送失败\n");
	}
	return 0;
}


int reply_put(int conn , char *filename)
{
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

	unsigned char buf[1024];
	int i = 0;
	while(ch != 0x88)	//读取数据
	{
		buf[i++] = ch;
		recv(conn , &ch , 1 , 0);
	}
	
	//解析数据

	//包长度
	int pkg_len = (buf[3] << 24) | 
				  (buf[2] << 16) |
				  (buf[1] << 8)  |
				  (buf[0] << 0);
	//命令
	int cmd = (buf[7] << 24) | 
			  (buf[6] << 16) |
			  (buf[5] << 8)  |
			  (buf[4] << 0);
	//文件出错标志
	unsigned char flag = buf[8];
	if(flag == 0)
	{
		printf("用户上传失败...\n");
		printf("等待用户重新输入命令...\n");
		return -1;
	}

	//文件内容长度
	int str_len = (buf[12] << 24) | 
				  (buf[11] << 16) |
				  (buf[10] << 8)  |
				  (buf[9]  << 0);

	//保存在服务器文件夹
	char filepath[128];
	sprintf(filepath , "%s/%s" , SERVER_PKGPATH ,filename);
	int fd = open(filepath , O_WRONLY | O_TRUNC | O_CREAT , 0664);
	if(fd == -1)
	{
		perror("open failed");
		return -1;
	}
	//写入
	printf("文件大小 %d Betys\n" , str_len);
	for(int j = 0; j < str_len ; j++)
	{
		recv(conn , &ch , 1 , 0);
		write(fd , &ch , 1 );
	}
	close(fd);
	return 0;
}

int reply_bye(int conn)
{
	close(conn);
	return 0;
}


int ls_recv(int sock)
{
	//准备接收服务器发送的数据
	int i = 0;
	unsigned char ch = 0;
	unsigned char recv_buf[2048];
	int pkg_len , cmd , filename_len;
	do	//防止读到的第一个数据不是帧的包头或包尾
	{
		recv(sock , &ch , 1 , 0);
	}
	while(ch != 0x88);
	
	while(ch == 0x88)	//防止读到的第一个数据是上一帧的包尾
	{
		recv(sock , &ch , 1 , 0);
	}
	
	i = 0;
	while(ch != 0x88)	//读取数据
	{
		recv_buf[i++] = ch;
		recv(sock , &ch , 1 , 0);
	}
	pkg_len = (recv_buf[3] << 24) | 
			  (recv_buf[2] << 16) |
			  (recv_buf[1] << 8)  |
			  (recv_buf[0] << 0);
	cmd = (recv_buf[7] << 24) | 
		  (recv_buf[6] << 16) |
		  (recv_buf[5] << 8)  |
		  (recv_buf[4] << 0);
	filename_len = 	(recv_buf[11] << 24) | 
					(recv_buf[10] << 16) |
			 		(recv_buf[9]  << 8)  |
			 		(recv_buf[8]  << 0);
	//printf("pkg_len = %d , cmd = %d , filename_len = %d\n", pkg_len , cmd , filename_len);
	printf("The server side has the following files:\n");
	for(int j = 0 ; j < filename_len ; j++ )
	{
		if( recv_buf[12+j] == ' ' )
		{
			putchar('\n');
		}
		else
		{
			printf("%c",recv_buf[12+j]);
		}
	}
}


int get_recv(int sock , char * filename)
{
	unsigned char ch;
	do	//防止读到的第一个数据不是帧的包头或包尾
	{
		recv(sock , &ch , 1 , 0);
	}
	while(ch != 0x88);

	while(ch == 0x88)	//防止读到的第一个数据是上一帧的包尾
	{
		recv(sock , &ch , 1 , 0);
	}

	unsigned char buf[1024];
	int i = 0;
	while(ch != 0x88)	//读取数据
	{
		buf[i++] = ch;
		recv(sock , &ch , 1 , 0);
	}
	
	//解析数据

	//包长度
	int pkg_len = (buf[3] << 24) | 
				  (buf[2] << 16) |
				  (buf[1] << 8)  |
				  (buf[0] << 0);
	//命令
	int cmd = (buf[7] << 24) | 
			  (buf[6] << 16) |
			  (buf[5] << 8)  |
			  (buf[4] << 0);
	//文件出错标志
	unsigned char flag = buf[8];
	if(flag == 0)
	{
		printf("No such file\n");
		return -1;
	}

	//文件内容长度
	int str_len = (buf[12] << 24) | 
				  (buf[11] << 16) |
				  (buf[10] << 8)  |
				  (buf[9]  << 0);
	char filepath[128];
	sprintf(filepath , "./%s" , filename);
	int fd = open(filepath , O_WRONLY | O_CREAT | O_TRUNC , 0664);
	
	//写入
	printf("File size %d Betys\n" , str_len);
	int k = 0 ,count = 0;
	for(int j = 0; j < str_len ; )
	{
		
		if(recv_check(sock , &ch , 1) != -1)
		{
			int ret = write(fd , &ch , 1 );
			if(ret == -1)perror("failed to write");
			j++;
			k++;
			if(k/1024)
			{
				k = 0;
				count++;
				printf("%d kb\n",count);
			}
		}
		if(j==str_len)printf("Receive completion\n");
	}
	close(fd);
}






int put_recv(int sock , char *filename)
{
	unsigned char filepath[1024];
	unsigned char re_pkg[4096] = {0};
	unsigned char flag , Isfinsh;
	int pkg_len = 4 + 4 + 1,size = 0,i = 0,ret;
	sprintf(filepath , "./%s" , filename);
	//打开用户需上传的文件
	int fd = open(filepath , O_RDONLY);
	if(fd == -1)
	{
		perror("open failed:");
		flag = 0;
	}
	else
	{
		flag = 1;
		size = lseek(fd, 0, SEEK_END);
		printf("File size %d Betys\n" , size);
		//将光标定在文件开头
		lseek(fd, 0 , SEEK_SET);		
	}
	
	//包头
	re_pkg[i++] = 0x88;

	//包长度		4
	if(flag == 1)
	{
		pkg_len += 4;
	}
	re_pkg[i++] = (pkg_len) >> 0  & 0xff;
	re_pkg[i++] = (pkg_len) >> 8  & 0xff;
	re_pkg[i++] = (pkg_len) >> 16 & 0xff;
	re_pkg[i++] = (pkg_len) >> 24 & 0xff;
	
	//命令号		4
	re_pkg[i++] = (server_ls) >> 0  & 0xff;
	re_pkg[i++] = (server_ls) >> 8  & 0xff;
	re_pkg[i++] = (server_ls) >> 16 & 0xff;
	re_pkg[i++] = (server_ls) >> 24 & 0xff;

	//文件是否存在	1
	re_pkg[i++] = flag;
	if(flag == 1)
	{
		//回复文件内容总长度	4
		printf("file_len = %d\n",size);
		re_pkg[i++] = (size) >> 0	& 0xff;
		re_pkg[i++] = (size) >> 8	& 0xff;
		re_pkg[i++] = (size) >> 16 & 0xff;
		re_pkg[i++] = (size) >> 24 & 0xff;
	}
	
	//包尾
	re_pkg[i++] = 0x88;	

	//预发送
	ret = send(sock , re_pkg , i , 0);

	//文件存在则发送
	if(flag == 1)
	{
		unsigned char ch;
		while(read(fd , &ch , 1) == 1)
		{
			ret = send(sock , &ch , 1 , 0);
		}
		close(fd);
	}
	
	if(ret <= 0)
	{
		perror("ls_send failed:");
		return -1;
	}
	else if(flag == 1)
	{
		printf(" %s Successful upload server\n" , filename);
	}
	else if(flag == 0)
	{
		printf("The user uploads the %s does not exist, and sends the failure\n" , filename);
	}
	return 0;
}

int bye_recv(int sock)
{}




int cmd_send(int sock , int snd_cmd , char *filename)
{
	unsigned char buf[1024];
	int filename_len , pkg_len = 8;
	if(snd_cmd == server_get || snd_cmd == server_put)
	{
		filename_len = strlen(filename) + 1;
		pkg_len+=filename_len;
	}
	int i = 0, j = 0;

	buf[i++] = 0x88;
	buf[i++] = (pkg_len) >> 0  & 0xff;
	buf[i++] = (pkg_len) >> 8  & 0xff;
	buf[i++] = (pkg_len) >> 16 & 0xff;
	buf[i++] = (pkg_len) >> 24 & 0xff;
	buf[i++] = (snd_cmd) >> 0  & 0xff;
	buf[i++] = (snd_cmd) >> 8  & 0xff;
	buf[i++] = (snd_cmd) >> 16 & 0xff;
	buf[i++] = (snd_cmd) >> 24 & 0xff;
	if(snd_cmd == server_get || snd_cmd == server_put)
	{
		printf("The file to download or upload is %s\n",filename);
		buf[i++] = (filename_len) >> 0  & 0xff;
		buf[i++] = (filename_len) >> 8  & 0xff;
		buf[i++] = (filename_len) >> 16 & 0xff;
		buf[i++] = (filename_len) >> 24 & 0xff;
		for( ; filename[j] != '\0' ; j++)
		{
			buf[i++] = filename[j];
		}
		buf[i++] = filename[j];
	}
	buf[i++] = 0x88;
	int ret = send(sock , &buf, i , 0);
	if(ret == -1)
	{
		perror("cmd_send failed:");
		return -1;
	}
	else 
	{
		printf("Command to send success!\n");
	}
	return 0;
}

int recv_check(int sock , unsigned char *ch , int size)
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
	
    return recv(sock , &ch , size , 0);
}

