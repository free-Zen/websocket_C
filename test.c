# include <stdio.h> 
# include <string.h> 
# include <unistd.h>
# include <stdlib.h>
# include <pthread.h>
# include <sys/socket.h> 
# include <arpa/inet.h>
# include <netinet/in.h> 
# include "webSocket.h"


# define SERVERIP "127.0.0.1" 
# define SERVERPORT 9002
# define MAXBUFFSIZE 1024
# define DEFEULT_SERVER_PORT 9001


int main(int argc,char *argv[])
{
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddr_len;
	int listenfd, connfd;
	int ret;
	int port= DEFEULT_SERVER_PORT;
	char ** ppRecvdata;
	char pSend[] = "I Get Your Message!!";
	ppRecvdata = (char **)malloc(sizeof(char *));
	if(ppRecvdata == NULL) return -1;
	if(argc>1)
	{
		port=atoi(argv[1]);
	}
	if(port<=0||port>0xFFFF)
	{
		printf(" Port(%d) is out of range(1-%d)\n",port,0xFFFF);
		return -1;
	}
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd < 0){
		printf("Create Server error with initiing\n");
		return -1;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr =inet_addr( SERVERIP);
	servaddr.sin_port = htons(port);

	if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
	{
		printf(" Create Server error with binding\n");
		close(listenfd);
		return -1;
	}

	if(listen(listenfd, 5) == -1)
	{
		printf("Create Server error with listening\n");
		close(listenfd);
		return -1;
	}

	printf("Server is running,Listen %d\n",port);
	cliaddr_len = sizeof(cliaddr);
	while(1)
	{
		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
		printf("Accept a connect,From  PORT %d\n",ntohs(cliaddr.sin_port));
		if (TCH_WebSocket_ServerHandshake(connfd)!=0)
		{
			printf(" handshake error \n");
			return -1;
		}
		while (1)
		{
			ret = TCH_WebSocket_ServerRead(connfd,ppRecvdata); 
			if(ret <=0)
				break;
			printf("######### Recv: \n%s\n",*ppRecvdata);
			free(*ppRecvdata);
			*ppRecvdata = NULL;
			TCH_WebSocket_ServerWrite(connfd,pSend,20);
		}
		close(connfd);
	}
	return 0;
}
	
	
