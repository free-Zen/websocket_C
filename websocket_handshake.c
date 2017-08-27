#include <stdio.h>
#include <arpa/inet.h>//for htonl and sockaddr_in
#include <unistd.h>
#include <stdlib.h>//for atoi
#include <string.h>//for memset and memcpy
#include <sys/types.h>
#include <sys/socket.h>
#include "webSocket.h"
#include"openssl/sha.h"

#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_GUID_LEN 36
#define SECURITY_KEY_LEN 24
#define ACCEPT_KEY_LEN 29



int Fetch_SecurityKey(char *pRequestHeader,char *pFetchKey)
{
	char *pGetKeysBegin = NULL;
	char *pFlag = "Sec-WebSocket-Key: ";
	int iKeyIndex = 0;
	
	if(pFetchKey == NULL || pRequestHeader == NULL) 
		return -1;
	pGetKeysBegin = strstr(pRequestHeader,pFlag);
	
	if(pGetKeysBegin == NULL)
	{
		return -1 ;
	}
	
	pGetKeysBegin += strlen(pFlag);
	while(pGetKeysBegin[iKeyIndex]!='\r' && pGetKeysBegin[iKeyIndex]!='\n' && iKeyIndex < SECURITY_KEY_LEN)
	{
		pFetchKey[iKeyIndex] = pGetKeysBegin[iKeyIndex];
		iKeyIndex++;
	}	
	return 0;
}

int Fecth_AcceptKey(char *pRespondHeader, char *pAcceptKey)
{
	char *pGetKeysBegin = NULL;
	char *pFlag = "Sec-WebSocket-Accept: ";
	int iKeyIndex = 0;
	
	if(pAcceptKey == NULL || pRespondHeader == NULL) 
		return -1;
	pGetKeysBegin = strstr(pRespondHeader,pFlag);
	
	if(pGetKeysBegin == NULL)
	{
		return -1 ;
	}
	
	pGetKeysBegin += strlen(pFlag);
	while(pGetKeysBegin[iKeyIndex]!='\r' && pGetKeysBegin[iKeyIndex]!='\n' && iKeyIndex < ACCEPT_KEY_LEN)
	{
		pAcceptKey[iKeyIndex] = pGetKeysBegin[iKeyIndex];
		iKeyIndex++;
	}	
	return 0;


}


int Create_AcceptKey(char * pInputData,char * pOutputData )
{
	#define SHA1_BUF_LEN 20
	char sha1input[WS_GUID_LEN+SECURITY_KEY_LEN];
	char sha1output[SHA1_BUF_LEN];
	
	if(pInputData == NULL || pOutputData == NULL ) 
	{
		return -1;
	}	
	
	memcpy(sha1input,pInputData,SECURITY_KEY_LEN);
	memcpy(sha1input+SECURITY_KEY_LEN,WS_GUID,WS_GUID_LEN);
	
	/***** use securekey and GUID data to create a 20 bytes data with sha1 function ***/
	SHA1(sha1input, sizeof(sha1input), sha1output);
	/****** a  text format data transform to base-64 format data (20+1)/3*4+1=29  ****/
	EVP_EncodeBlock(pOutputData, sha1output, SHA1_BUF_LEN);
	
	return 0;
}

int webSocket_ServerHandshake(int socketfd)
{
	
	char RequestHeader[MAXDATASIZE];
	char ShakeHandData[MAXDATASIZE];
	char KeyValue[SECURITY_KEY_LEN];
	int BuffLens = 0;
	char Accept_Key[ACCEPT_KEY_LEN];
	
	memset(RequestHeader,0,MAXDATASIZE);
	memset(ShakeHandData,0,MAXDATASIZE);
	
	if( socketfd <= 0)
	{
		return -1 ;
	}
	
	BuffLens = recv(socketfd,RequestHeader,MAXDATASIZE,0);
	if(BuffLens <= 0) return -1 ;
	
	if (Fetch_SecurityKey(RequestHeader,KeyValue) != 0) return -1;
		
	if (Create_AcceptKey(KeyValue,Accept_Key) != 0) return -1;
	
	sprintf(ShakeHandData, 
           "HTTP/1.1 101 Switching Protocols\r\n"
           "Upgrade: websocket\r\n"
           "Connection: Upgrade\r\n"
           "Sec-WebSocket-Accept: %s\r\n"
           "\r\n", Accept_Key);
		   
	BuffLens = send(socketfd,ShakeHandData,strlen(ShakeHandData),0);
	if(BuffLens != strlen(ShakeHandData))
	{
		return -1 ;
	}
	printf("\n%s\n",ShakeHandData);
	return 0 ;
}



int create_security_key(char *pSecurityKey)
{
	char randstr[16];
	char raw_Key[25];
	memset(raw_Key,0,25);
	int i;
	if(pSecurityKey == NULL ) return -1;
	srand((unsigned)time(NULL));
	for (i = 0; i < 16; i++)    
	{    
		randstr[i] =  rand() % 94 +33; 
	}    
	EVP_EncodeBlock(raw_Key,randstr,16);
	memcpy(pSecurityKey,raw_Key,24);
	return 0;
}

/*
INT create_security_key(CHAR *pSecurityKey)
{
	CHAR tempSecurityKey[] = "UmF5DnJ5UXhHeDdxYm54NQ==";

	return memcpy(pSecurityKey, tempSecurityKey, sizeof(tempSecurityKey));
}*/

int webSocket_ClientHandshake(int socketfd)
{
	int i;
	int iState; 
	char requestHeader[1024];
	char respondHeader[1024];
	char secuirtKey[SECURITY_KEY_LEN];
	char acceptKey[ACCEPT_KEY_LEN];
	char correctAcceptKey[ACCEPT_KEY_LEN];

	create_security_key(secuirtKey);
	Create_AcceptKey(secuirtKey, correctAcceptKey);

	sprintf(requestHeader, 
		"GET / HTTP/1.1\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Host: 127.0.0.1:9002\r\n"
		"Origin: HTTP://127.0.0.1:9002\r\n"
		"Sec-WebSocket-Key: %s\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"Sec-WebSocket-Protocol: test for UCPlayer\r\n"
           "\r\n", secuirtKey);

	if(write(socketfd, requestHeader, strlen(requestHeader)) <= 0)
		return -1;
	if(read(socketfd, respondHeader, sizeof(respondHeader))<= 0)
		return -1;

	if(Fecth_AcceptKey(respondHeader, acceptKey)<0)
		return -1;

	for(i=0;i<ACCEPT_KEY_LEN;i++)
		if(correctAcceptKey[i]!=acceptKey[i])
			return -1;

	return 0;

}

