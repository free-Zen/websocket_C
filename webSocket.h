#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__


#ifdef __cplusplus
extern "C"
{
#endif



#define MAXDATASIZE 4096

/*Return 1:handshake successful 0:handshake failed*/
int webSocket_ServerHandshake(int socketfd);
int webSocket_ClientHandshake(int socetfd);


int webSocket_ServerRead(int socketfd, char ** ppBuf);
int webSocket_ServerWrite(int sockdtfd, char * pBuf  , int iBufLens);

int webSocket_ClientRead(int socketfd, char * *ppBuf);
int webSocket_ClientWrite(int sockdtfd, char * pBuf  , int iBufLens);




#ifdef __cplusplus
}
#endif

#endif
