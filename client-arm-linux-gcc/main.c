




#include <stdio.h>
#include "client.h"
#include <signal.h>



void sighandler(int sig)
{
	printf("Please say bye to the server ^_^\n");
}




//TCP
int main(int argc , char *argv[])
{
	if(argc < 3)
	{
		usage();
		return 0;
	}
	signal(SIGINT, sighandler);
    signal(SIGCONT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGQUIT, sighandler);
    signal(SIGSTOP, sighandler);
    signal(SIGTSTP, sighandler);
    
	int socket_fd = init_connection(argv[1],argv[2]);
	if(socket_fd == -1)
	{
		return 0;
	}
	send_cmd_server(socket_fd);
	return 0;
}













