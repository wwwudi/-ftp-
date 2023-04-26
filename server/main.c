




#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"


int terminate = 0;





void sighandler(int sig)
{
	switch(sig)
    {
	    case SIGINT: terminate = 1;
	        		 break;
	    case SIGALRM: terminate = 1;
	        		  break;
    }
}

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		usage();
		return 0;
	}
	signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
	
	int sock = create_tcp_listen_socket(argv[1],argv[2]);
	if(sock == -1)
	{
		return 0;
	}
    int online_client = 0;
	while(!terminate)
	{
		printf("目前在线客户数为: %d\n",online_client);
		struct sockaddr_in client;
	    socklen_t  addrlen = sizeof(client);
		if(online_client < 128)
		{
			int conn = accept_client_conntion(sock , &client , &addrlen);
			if (conn > 0)
		    {
		       online_client++;
		   	   pid_t pid = fork();
		   	   if(pid > 0)
		   	   {
					close(conn);
		   	   }
		   	   else if(pid == 0)
		   	   {
					printf("成功与 %s 建立连接 端口号为: %d\n",inet_ntoa(client.sin_addr),client.sin_port);
					printf("连接号: conn = %d\n",conn);
					client_handler(conn , client);
					raise(SIGINT);
		   	   } 
			}
			while(waitpid( -1 , NULL , WNOHANG) > 0)
		    {
				online_client--;
		    }
	    }
	}
    close(sock);
    return 0;
}





