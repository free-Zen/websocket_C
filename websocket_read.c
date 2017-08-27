#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>//for atoi
#include <string.h>//for memset and memcpy
#include <sys/types.h>
#include <sys/socket.h>
#include "webSocket.h"


typedef struct _WS_RECV_FRAME_{
	BOOL Fin;
	unsigned char Opcode;
	BOOL Mask;
	INT Payload_len;
	unsigned char Masking_key[4];
	INT RecvState;
	INT iNeedByte;
	unsigned char * iRecvMark;
	unsigned char * iRecvEnd;
	unsigned char Recvbuf[MAXDATASIZE];
}WS_Recv_Frame;

enum ws_recv_state{
	RECV_HEADER,
	RECV_EXC,
	RECV_MASKKEY,
	RECV_PAYLOAD
};

#define WSRECV_AVAIL_IBUF(ctx) ((int)(ctx->iRecvEnd - ctx->iRecvMark))


static void WS_shift_ibuf(WS_Recv_Frame * ctx)
{
 	int len = ctx->iRecvEnd-ctx->iRecvMark;
 	memmove(ctx->Recvbuf, ctx->iRecvMark, len);
	ctx->iRecvEnd = ctx->Recvbuf+len;
	ctx->iRecvMark = ctx->Recvbuf;
}

static int Get_Enough_Data(int socketfd,WS_Recv_Frame * ctx)
{
	int iRecvLens;
	iRecvLens = recv(socketfd, ctx->iRecvEnd,ctx->Recvbuf+sizeof(ctx->Recvbuf)-ctx->iRecvEnd,0);
	if(iRecvLens <0) return -1;
	else
	{
		ctx->iRecvEnd += iRecvLens;
		return iRecvLens;
	}	
}

static int Insure_Plenty_Data(int socketfd,WS_Recv_Frame * ctx)
{
	if(ctx->iRecvMark != ctx->Recvbuf)
		WS_shift_ibuf(ctx);
	if(WSRECV_AVAIL_IBUF(ctx) < ctx->iNeedByte)
	{
		Get_Enough_Data(socketfd,ctx);
		if(WSRECV_AVAIL_IBUF(ctx) < ctx->iNeedByte)
			return -1;
	}
	return WSRECV_AVAIL_IBUF(ctx);
}


static void init_RecvContent(WS_Recv_Frame * ctx)
{
	ctx->iRecvMark = ctx->iRecvEnd = ctx->Recvbuf;
	memset(ctx->Recvbuf,0,MAXDATASIZE);
	ctx->RecvState = RECV_HEADER;
	ctx->iNeedByte = 2;
	ctx->Payload_len = 0;
}

static INT Recv_Frame_Header(int socketfd,WS_Recv_Frame * ctx)
{
	unsigned char Payload;
	if(Insure_Plenty_Data(socketfd,ctx) <0) return -1;
	ctx->Fin = (ctx->iRecvMark[0] >> 7) & 1;
	ctx->Opcode = ctx->iRecvMark[0] & 0xf;
	ctx->Mask = (ctx->iRecvMark[1] >> 7) & 1;
	Payload = ctx->iRecvMark[1] & 0x7f;
	ctx->iRecvMark += 2;
	if(Payload == 126)
	{
		ctx->RecvState = RECV_EXC;
		ctx->iNeedByte = 2;
	}
	else if(Payload == 127)
	{
		return -1;
	}
	else
	{
		ctx->Payload_len =(INT) Payload;
		if(ctx->Mask  )
		{
			ctx->RecvState = RECV_MASKKEY;
			ctx->iNeedByte = 4;
		}
		else
		{
			ctx->RecvState = RECV_PAYLOAD;
			ctx->iNeedByte = 1;
		}
	}
	return 0;
}


static int Recv_Frame_ExternPayload(int socketfd,WS_Recv_Frame * ctx)
{
    int  iGetLens;
	if( Insure_Plenty_Data(socketfd,ctx) <0) return -1;
	if(ctx->RecvState == RECV_EXC)
	{
		iGetLens = (INT) (((ctx->Recvbuf[0] & 0xFF)<<8) |(ctx->Recvbuf[1] &0xFF));
		ctx->iRecvMark += 2;
		ctx->Payload_len = iGetLens;
	}
	if(ctx->Mask  )
	{
		ctx->RecvState = RECV_MASKKEY;
		ctx->iNeedByte = 4;
	}
	else{
		ctx->RecvState = RECV_PAYLOAD;
		ctx->iNeedByte = 1;
	} 
	return 0;
}

static int Recv_Frame_MaskKey(int socketfd,WS_Recv_Frame * ctx)
{
	if(Insure_Plenty_Data(socketfd,ctx) <0) return -1;
	if(ctx->RecvState == RECV_MASKKEY)
	{
		memcpy(ctx->Masking_key,ctx->iRecvMark, 4 );
	}
	ctx->RecvState = RECV_PAYLOAD;
	ctx->iRecvMark +=4;
	ctx->iNeedByte = 1;
	return 0 ;
}

static CHAR * Recv_Frame_Payload(int socketfd,WS_Recv_Frame * ctx)
{
	int iNeedRecvLens;
	int iRecvTotal;
	int iRecvSize;
	int iGetLen;
	int i;
	char * RecvPayload = NULL;
	if(ctx->Payload_len <= 0) return NULL;
	iRecvTotal = iNeedRecvLens = ctx->Payload_len;
	RecvPayload = (char *)malloc((iRecvTotal+1) * sizeof(char));
	if(RecvPayload == NULL) return NULL;
	memset(RecvPayload,0,iRecvTotal+1);
	
	iGetLen = Insure_Plenty_Data(socketfd,ctx) ;
	iGetLen = (iGetLen >= iNeedRecvLens ? iNeedRecvLens: iGetLen);
	while(1)
	{
		if(iGetLen <0) 
		{
			free(RecvPayload);
			return NULL;
		}
		if(ctx->Mask)
			for(i=0;i<iGetLen;i++)
				ctx->Recvbuf[i] ^= ctx->Masking_key[i%4];
		memcpy(RecvPayload+iRecvTotal-iNeedRecvLens,ctx->Recvbuf,iGetLen);
		iNeedRecvLens -=iGetLen;
		
		if(iNeedRecvLens <=0) break;
		iRecvSize = iNeedRecvLens > MAXDATASIZE ? MAXDATASIZE : iNeedRecvLens;
		iGetLen = recv(socketfd,ctx->Recvbuf, iRecvSize,0);	
	}
	return RecvPayload;	
}

static int  Recv_Data(int socketfd ,char **ppRecvData, BOOL IsServer)
{
	WS_Recv_Frame RecvContent;
	if(socketfd <= 0 || ppRecvData ==NULL) return -1;
	init_RecvContent(&RecvContent);
	if(RecvContent.RecvState == RECV_HEADER )
		if(Recv_Frame_Header(socketfd,&RecvContent) <0)
			return -1;
		
	if(RecvContent.RecvState == RECV_EXC )
		if(Recv_Frame_ExternPayload(socketfd,&RecvContent) < 0)
			return -1;

	if(RecvContent.RecvState == RECV_MASKKEY )
		if(Recv_Frame_MaskKey(socketfd,&RecvContent)<0)
			return -1;
	
	if(RecvContent.RecvState == RECV_PAYLOAD )
		*ppRecvData = Recv_Frame_Payload(socketfd,&RecvContent);
	if(*ppRecvData == NULL || RecvContent.Mask != IsServer) return -1;
	else  return (RecvContent.Payload_len);
}



int webSocket_ServerRead(int socketfd,char **ppRecvData)
{
	return Recv_Data(socketfd,ppRecvData,1);
}


int webSocket_ClientRead(int socketfd,char **ppRecvData)
{
	return Recv_Data(socketfd,ppRecvData,0);
}



