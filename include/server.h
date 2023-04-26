












#ifndef __SERVER_H__
#define __SERVER_H__

/* 
    create_tcp_listen_socket: 创建一个tcp类型的“监听套接字”
    @ip : 指定一个本地ip
    @port: 指定一个tcp的端口号
    返回值：
        成功  返回创建好的监听套接字描述符(> 0)
        失败  返回-1.
*/
int create_tcp_listen_socket(const char * ip,const char * port);


/*
    accept_client_conntion: 从“监听套接字”上接收一个客户端的连接请求。
    @sock: 监听套接字
    返回值：
        成功  返回一个与客户端的连接套接字(>0)
        失败  返回-1, errno被设置
*/
int accept_client_conntion(int sock , struct sockaddr_in *client , socklen_t  *addrlen);


int client_handler(int conn , struct sockaddr_in client);

void usage(void);

#endif








