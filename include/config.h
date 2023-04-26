

#ifndef __CONFIG_H__
#define __CONFIG_H__



enum cmd_list
{
	server_ls = 11,
	server_get = 22,
	server_put = 33,
	server_bye = 88
};



int reply_ls(int conn);
int reply_get(int conn , char *filename);
int reply_put(int conn , char *filename);
int reply_bye(int conn);


int ls_recv(int sock);
int get_recv(int sock , char *filename);
int put_recv(int sock , char *filename);
int bye_recv(int sock);



int cmd_send(int sock , int snd_cmd , char *filename);
int recv_check(int sock , unsigned char *ch , int size);
#endif