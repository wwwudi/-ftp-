#ifndef __SERVER_H__
#define __SERVER_H__






void usage(void);


/* 
    create_tcp_connection_socket: 创建一个tcp类型的“套接字”
    @ip : 指定一个本地ip
    @port: 指定一个tcp的端口号
    返回值：
        成功  返回创建好的套接字描述符(> 0)
        失败  返回-1.
*/
int init_connection(const char * ip,const char * port);



void send_cmd_server(int socket_fd);

void sighandler_alrm(int sig);




#endif


