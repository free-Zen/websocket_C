#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>//for atoi
#include <string.h>//for memset and memcpy
#include <sys/types.h>
#include <sys/socket.h>
#include "webSocket.h"

#define MASK_KEYLENGTH 4

static int SendData(int  socketfd,char *pGetdata,int  iDataLens,BOOL Mask)
{
	unsigned char pPackData[MAXDATASIZE];
	int  iSendSize;
	int  iSendTotal;
	int  iPointIndex = 0;
	char Mask_Key[MASK_KEYLENGTH]; 
	int  i;
	
	if(socketfd <= 0 || pGetdata == NULL || iDataLens <= 0)
	{
		return -1;
	}

	iSendTotal = iDataLens;
	memset(pPackData,0,MAXDATASIZE);
	pPackData[iPointIndex++] = 0x81;
	if(iDataLens < 0x7E) 
	{
		pPackData[iPointIndex++] = (((unsigned char) iDataLens) & 0x7F)
			|(unsigned char)(Mask <<7)  ;
	}
	else if (iDataLens< 65536)
	{
		pPackData[iPointIndex++] = (unsigned char) 0x7E |(unsigned char)(Mask <<7)  ;
		pPackData[iPointIndex++] = (unsigned char) (iDataLens >> 8) ;
		pPackData[iPointIndex++] = (unsigned char) (iDataLens & 0x000000FF) ;
	}
	else  return -1;	
	if(Mask)
	{
		for(i=0;i<MASK_KEYLENGTH;i++)
			Mask_Key[i] = pPackData[iPointIndex++] = (char)(rand()%127 +1);
	}

	iSendSize = MAXDATASIZE-iPointIndex;
	iSendSize = (iSendTotal > iSendSize ?  iSendSize : iSendTotal);
	if(Mask)
	{
		for(i=0;i<iSendSize;i++)
			pPackData[i+iPointIndex] = (char)(pGetdata[i] ^ Mask_Key[i%4]);
	}
	else 	memcpy(pPackData+iPointIndex,pGetdata,iSendSize);
	
	iSendTotal -= iSendSize;
	iSendSize += iPointIndex;
		
	while(1)
	{
		if(send(socketfd,pPackData,iSendSize,0)  != iSendSize) return -1;
		if(iSendTotal <=0)  break;
		iSendSize = (iSendTotal > MAXDATASIZE ?  MAXDATASIZE : iSendTotal);
		
		if(Mask)
		{
			for(i=0;i<iSendSize;i++)
				pPackData[i] = (char)(pGetdata[i] ^ Mask_Key[i%4]);
		}
		else 	memcpy(pPackData,pGetdata,iSendSize);
		iSendTotal -= iSendSize;
	}
	return iDataLens ;
	
}

int  webSocket_ServerWrite(int  socketfd,char* pGetdata,int  iDataLens)
{
	return SendData(socketfd,pGetdata,iDataLens,0);
}


int  webSocket_ClientWrite(int  socketfd,char* pGetdata,int  iDataLens)
{
	return SendData(socketfd,pGetdata,iDataLens,1);
}



