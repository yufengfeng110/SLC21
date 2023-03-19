#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "otaupgradetask.h"
#include "ec600m.h"
#include "cmddeal.h"
#include "stmflash.h"
#include "delay.h"
#include "dma.h"
#include "wdg.h"
///////////////////////////////////////////////////////////////////////
#define HTTP_FILE_MAXLEN  100*1024
#define HTTP_GET  "GET /%s HTTP/1.1\r\nHOST: %s:%d\r\n\r\n"   
#define HTTP_FILE "GET /%s HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; rv:12.0) \
Gecko/20100101 Firefox/12.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n \
Accept-Language: zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\nAccept-Encoding: gzip,deflate\r\nConnection: keep-alive\r\n\r\n"
///////////////////////////////////////////////////////////////////////
char OTA_RX_Buf[EC600M_RX_BUF] = {0};
char temp[100] = {"http://39.98.57.97:8084/config-center/version/upgrade"};   //OTA升级地址
////////////////////////////////////////////////////////////////////////
int getUpdatePath(char* pchUrl,char* version,char** pchPath);
char* http_get(const char *url);
void http_tcpclient_close(int socket);
int http_parse_url(const char *url,char *host,char *file,u16 *port);
int http_tcpclient_create(u8 *host, u16 port);
int http_tcpclient_send(int socket,char *buff,int size);
void* http_tcpclient_recv(int times);
char *http_parse_result(const char*lpbuf);
int ParseStringValue(char* json,char* key,char* value);
int getUpdateFile(char *url);
int http_check_result(const char*lpbuf);
int http_file_size(const char*lpbuf);
int OTA_GETdata_create(u8 *host, u16 port);
int OTA_GETdata_recv(u16 len, int times, char* pchotadata);
////////////////////////////////////////////////////////////////////////
void OTAupgrade_task(void *pvParameters)
{
   	u16 time = 0;
	char *pchPath = NULL;
	int iRet = -1;
	
	strcpy((char*)gl_IcapDevObj.pchUpgradeUrl, temp);
	while(1)
	{
		printf("OOOOOOOOOOOOOOOTAupgrade_task running!!\r\n");
		time++;
		vTaskDelay(1000);
		if(time == OTATIME)
		{
			time = 0;
			pchPath = pvPortMalloc(200);
			memset(pchPath, 0, 200);
			if(strcmp((const char*)gl_IcapDevObj.pchUpgradeUrl,"") != 0 && getUpdatePath((char*)gl_IcapDevObj.pchUpgradeUrl,APPVERSION,&pchPath) == 1)
			{
				printf("--------------升级地址解析成功！！！\r\n");
				if(strcmp(pchPath,"") != 0 && strstr(pchPath,APPVERSION) == NULL)
				{
					iRet = getUpdateFile(pchPath);
					if(iRet == 1)
					{
					 	printf("update application ok,will restart\r\n");
					 	vPortFree(pchPath);
					 	SoftReset();
					}
					else
					{
						printf("update application is not ok!!!!\r\n");
					}
				}
			}
			vPortFree(pchPath);
		}
		IWDG_Feed(); 
	}
}

/********************************************************/
int getUpdatePath(char* pchUrl,char* version,char** pchPath)
{
	int iRet = 0;
	char* pchGetUrl = NULL;
	char* pchBuf = NULL;
	// pWKS_UpdateObj pObj = NULL;

	pchGetUrl = pvPortMalloc(100);
	// pObj = pvPortMalloc(sizeof(WKS_UpdateObj));
	if(pchGetUrl == NULL)
	{
		DEBUG("malloc geturl and obj error");
		goto END;
	}
	sprintf(pchGetUrl,"%s?version=%s",pchUrl,version);
	DEBUG("^^^^^^^^^^^^pchGetUrl:%s\r\n", pchGetUrl);
	pchBuf = http_get(pchGetUrl);
	if(pchBuf != NULL)
	{
		DEBUG("OOOOOOOOOOOOOOO## pchBuf %s",pchBuf);
		ParseStringValue(pchBuf, "url", *pchPath);
		if(strcmp((const char*)(*pchPath), "null") != 0)
		{
			DEBUG("URL_addr is:%s\r\n", (*pchPath));
			iRet = 1;
		}
		else
		{
			DEBUG("URL_addr is NULL!!!\r\n");
		}
	}
END:
	IWDG_Feed(); 
	vPortFree(pchGetUrl);
	// vPortFree(pObj);
	vPortFree(pchBuf);
	return iRet;
}
/********************************************************/
char* http_get(const char *url)
{
	int    	socket_fd = -1;
	char*  	pchRet = NULL;
	char*  	pchHost = NULL;
	char*  	pchUrl = NULL;
	char*  	pchBuf = NULL;
	char*	pchupgrade = NULL;
	u16    iPort;
	
	if(url == NULL || strcmp(url,"") == 0)
	{
		DEBUG("url is NULL or temp\r\n");
		goto END;
	}
	pchHost = pvPortMalloc(100);
	pchUrl = pvPortMalloc(100);
	pchBuf = pvPortMalloc(100);
	if(pchHost == NULL || pchUrl == NULL || pchBuf == NULL)
	{
		DEBUG("malloc buf error");
		goto END;
	}
   	if(http_parse_url(url,pchHost,pchUrl,&iPort))
	{
       DEBUG("http_parse_url failed!\r\n");
       goto END;
   	}
   	socket_fd =  http_tcpclient_create((u8*)pchHost,iPort);     //AT+OPEN创建socket连接
   	if(socket_fd < 0)
	{
       DEBUG("http_tcpclient_create failed\r\n");
       goto END;
   	}
	sprintf(pchBuf,HTTP_GET,pchUrl,pchHost,iPort);
	if(http_tcpclient_send(socket_fd,pchBuf,strlen(pchBuf)) < 0)
	{
       DEBUG("http_tcpclient_send failed..\r\n");
       goto END;		
	}
   	if((pchupgrade = http_tcpclient_recv(100)) == NULL)
	{
       DEBUG("http_tcpclient_recv failed\r\n");
       goto END;
   	}
	pchRet = http_parse_result((const char*)pchupgrade);
END:
	if(socket_fd != -1)
	{
		http_tcpclient_close(socket_fd);
	}
	vPortFree(pchHost);
	vPortFree(pchUrl);
	vPortFree(pchBuf);
	return pchRet;
}
/********************************************************/
void http_tcpclient_close(int socket)
{
	if(socket == OTA_SOCKET)
	{
		EC600M_CloseSocket(socket);
	}
}
/********************************************************/
//请求平台获取下载升级包地址（version为当前版本）
//http://ip:port/config-center/version/upgrade?version=3.2.0
/********************************************************/
int http_parse_url(const char *url,char *host,char *file,u16 *port)
{
   char *ptr1,*ptr2;
   int len = 0;
   if(!url || !host || !file || !port)
	{
       return -1;
   }
   ptr1 = (char *)url;
   if(!strncmp(ptr1,"http://",strlen("http://")))//检查是否收到http://
	{
       ptr1 += strlen("http://");
   }
	else
	{
       return -1;
   }
   ptr2 = strchr(ptr1,'/');
   if(ptr2)
	{
       len = strlen(ptr1) - strlen(ptr2);		//获得ip:port
       memcpy(host,ptr1,len);			//拷贝到ip:port
       host[len] = '\0';
       if(*(ptr2 + 1))
		{
           memcpy(file,ptr2 + 1,strlen(ptr2) - 1 );  //获取config-center/version/upgrade?version=3.2.0
           file[strlen(ptr2) - 1] = '\0';
       }
   }
	else
	{
       memcpy(host,ptr1,strlen(ptr1));
       host[strlen(ptr1)] = '\0';
   }
   ptr1 = strchr(host,':');
   if(ptr1)
	{
       *ptr1++ = '\0';
       *port = atoi(ptr1);					//获取端口号
   }
	else
	{
       *port = 80;						
   }
   IWDG_Feed(); 
   return 0;
}
/********************************************************/
int http_tcpclient_create(u8 *host, u16 port)
{
	int socket_fd = -1;
	int iRet = 0;
	BEGIN:	
	EC600M_CloseSocket(OTA_SOCKET);
	iRet = EC600M_OpenSocket(OTA_SOCKET, "TCP", host, port);
	if(iRet < 0)
	{
		goto BEGIN;
	}
	socket_fd = OTA_SOCKET;
	IWDG_Feed(); 
	return socket_fd;
}
/******************************************************/
int OTA_GETdata_create(u8 *host, u16 port)
{
	BaseType_t err=pdFALSE;
	char *strx=NULL;
	char *pchBuf=NULL;
	char num = 0;
	int socket_fd = -1;

	pchBuf = pvPortMalloc(15);
	memset(pchBuf, 0 , 15);
    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
	printf("OTA_getdata创建链接接收到信号量！\r\n");
	sprintf(ATOPEN,"AT+QIOPEN=1,%d,\"%s\",\"%s\",%d,0,0\r\n", OTA_SOCKET, "TCP", host, port);
	sprintf(pchBuf, "+QIOPEN: %d,0", OTA_SOCKET);
	strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,ATOPEN);                                       //UDP登录中心并设置为直吐模式 
	memset(ATOPEN, 0 , 60);
	printf("+QIOPEN命令:%s\r\n", gl_Ec600MObj.g_ec600m_TXBuf);
	gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
	DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);  
	printf("创建链接_发送完成！\r\n");                         
	err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
	if(err==pdTRUE)
	{
		DEBUG("建链接收到的数据：%s\r\n", gl_CMDtaskObj.CmdRecvBuf); 
		strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, (const char *)pchBuf);                  //检查是否登陆成功
		while(strx == NULL)
		{
			clear_RXBuffer();                                                                                          
			err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf, 100);	
			DEBUG("循环内建链接收到的数据：%s\r\n", gl_CMDtaskObj.CmdRecvBuf); 
			if(err==pdTRUE)                                                                   
			{
				strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, (const char *)pchBuf);                     
			}
			num++;
			if(num == 10)
			{
				goto END;
			} 
		}
		socket_fd = OTA_SOCKET;
		printf("AT+QIOPEN=1,%d回复命令:%s\r\n", OTA_SOCKET, gl_CMDtaskObj.CmdRecvBuf);
	}
    END:
	clear_RXBuffer();
    clear_TXBuffer();
    vPortFree(pchBuf);
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
    printf("OTA_getdata创建链接释放信号量！\r\n");
	IWDG_Feed(); 
	return socket_fd;
}
/******************************************************/
int http_tcpclient_send(int socket,char *buff,int size)
{
   	int sent=0,tmpres=0;
   	while(sent < size)
	{
		xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
		printf("OTA创建完链接发送数据_接收sem！\r\n");
		tmpres = EC600M_Send(socket,buff+sent,size-sent);
		xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
		printf("OTA创建完链接发送数据_发送sem！\r\n");
		if(tmpres == 0)
		{
			return -1;
		}
		sent += tmpres;
   }
   IWDG_Feed(); 
   return sent;
}
/********************************************************/
void* http_tcpclient_recv(int times)
{
   	char* precvbuf = NULL;
	xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
	DEBUG("OTA发送GET后等接收_接收sem！\r\n");
	precvbuf = EC600M_Receive(times);
	xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
	DEBUG("OTA发送GET后等接收_接收sem！\r\n");
	IWDG_Feed(); 
   	return precvbuf;
}
/********************************************************/
//packnum:1-第一包 2-第一包后面的数据包
//times:延迟时间
//pchotadata:升级数据的头指针
//len:要读取有效数据的长度 （不包括+QIRD头）
int OTA_GETdata_recv(u16 len, int times, char* pchotadata)
{
	BaseType_t err=pdFALSE;
//	char 	*pTemp = NULL;	
	char 	*strx = NULL;
	// char	*exstrx = NULL; 
	char 	*pchBuf = NULL;
	int 	datalen = 0;
	u8 		circle = 0;
	// char    datalength[5] = {0};
	// char 	i;

	xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
	printf("OTA发送GET后等接收_接收sem！\r\n");
	times = times*100;                                          //超时时间单位ms
	vTaskDelay(times);
	printf("进入OTA接收模式!!!\r\n");
	memset(gl_Ec600MObj.g_ec600m_TXBuf, 0, EC600M_BUF_NUM);
	sprintf(gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIRD=2,%d\r\n", len);
	gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
	printf("OTA 发送长度：%d\r\n", gl_Ec600MObj.g_ec600m_TXCnt);
	DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);
	printf("OTA +QIRD send success!\r\n"); 
	err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
	printf("OTA recvqueue recved!\r\n");
	if(err==pdTRUE) 
	{
		strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QIRD:");                          
		while(strx == NULL)
		{
			if(circle == 30)
			{
				goto END;
			}
			circle++;
			clear_RXBuffer();                                                                                           
			err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf, 100);	
			if(err==pdTRUE)                                                                   
			{
				strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QIRD:");            
			} 
		}
		// exstrx = strstr(strx, "\r\n");
		// DEBUG("strx:%08X\r\n", (unsigned int)strx);
		// DEBUG("exstrx:%08X\r\n", (unsigned int)exstrx);
		// memcpy(datalength, strx+7, exstrx-(strx+7));
		// datalength[exstrx-(strx+7)] = '\0';
		// DEBUG("datalength:%s\r\n", datalength);
		// datalen = atoi((const char*)datalength);
		sscanf(strx, "+QIRD: %d\r\n", &datalen);
		printf("@@@@@@@@@@@收到升级包长度：%d\r\n", datalen); 
		pchBuf = pvPortMalloc(15); 
		memset(pchBuf, 0 , 15);
		sprintf(pchBuf, "+QIRD: %d\r\n", datalen);
		strx += strlen(pchBuf);
		vPortFree(pchBuf);
		memcpy(pchotadata, (const char*)strx, datalen);
	}

	END:
	clear_RXBuffer();
	xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
	printf("OTA发送GET后等接收_释放sem！\r\n");
	IWDG_Feed(); 
   	return datalen;
}
/********************************************************/
char* http_parse_result(const char*lpbuf)
{
	char *ptmp = NULL; 
	char *response = NULL;
	ptmp = (char*)strstr(lpbuf,"HTTP/1.1");
	if(!ptmp)
	{
		DEBUG("http/1.1 not find");
		return NULL;
	}
	if(atoi(ptmp + 9) != 200){
		DEBUG("result:\n%s",lpbuf);
		return NULL;
	}

	ptmp = (char*)strstr(lpbuf,"\r\n\r\n");
	if(!ptmp){
		DEBUG("ptmp is NULL");
		return NULL;
	}
		//跳过长度字段
	ptmp = (char*)strstr(ptmp+4,"\r\n");
	if(!ptmp){
		DEBUG("ptmp is NULL");
		return NULL;
	}
		response = pvPortMalloc(strlen(ptmp)+1);
	if(!response){
		DEBUG("malloc failed");
		return NULL;
	}
	strcpy(response,ptmp+2);
	IWDG_Feed(); 
	return response;
}
/********************************************************/
int getUpdateFile(char *url)
{
	int    iRet = 0;
	int    socket_fd = -1;
	char*  pchHost = NULL;
	char*  pchUrl = NULL;
	char*  pchBurHead = NULL;
	char*  pchTemp = NULL;
//	char*  pchData = NULL;
	u16    iTemp;
	int    iLen = 0;
	int    iTotal = 0;
	int    iPos = 0;
	
	printf("getUpdateFile start....!");
	if(url == NULL || strcmp(url,"") == 0)
	{
		printf("url is NULL or temp");
		goto END;
	}
	pchHost = pvPortMalloc(100);
	pchUrl = pvPortMalloc(100);
	pchBurHead = pvPortMalloc(200);
	if(pchHost == NULL || pchUrl == NULL || 
	   pchBurHead == NULL)
	{
		printf("malloc buf error");
		goto END;
	}
	if(http_parse_url(url,pchHost,pchUrl,&iTemp))
		{
		printf("http_parse_url failed!");
		goto END;
	}
	socket_fd =  OTA_GETdata_create((u8*)pchHost,iTemp);
	if(socket_fd < 0)
	{
       printf("http_tcpclient_create failed");
       goto END;
   	}
	memset(OTA_RX_Buf, 0, EC600M_RX_BUF);					//先清一下缓存再往里放数据
	sprintf(OTA_RX_Buf,HTTP_FILE,pchUrl,pchHost,iTemp);
	if(http_tcpclient_send(socket_fd,OTA_RX_Buf,strlen(OTA_RX_Buf)) < 0)
	{
       printf("http_tcpclient_send failed..");
       goto END;		
	}
	iLen = OTA_GETdata_recv(600, 10, OTA_RX_Buf);
	if(iLen != 0)
	{
		pchTemp = strstr(OTA_RX_Buf,"\r\n\r\n") + 4;
		if(pchTemp == NULL)
		{
			printf("Not find 0a 0d 0a 0d");
			goto END;
		}
		memcpy(pchBurHead,OTA_RX_Buf,pchTemp-OTA_RX_Buf);
		if(http_check_result(pchBurHead) == 0)
		{
			printf("http result is not 200");
			goto END;
		}
		iTotal = http_file_size(pchBurHead);
		if(iTotal == 0 || iTotal > (HTTP_FILE_MAXLEN - 30))
		{
			printf("http file size is not right %d",iTotal);
			goto END;
		}
		printf("HTTP收到更新数据总长：%d\r\n", iTotal);
		iRet = UPDATE_ENABLE;
		iPos = 0;
		printf("flash write upgrade enable");
		STMFLASH_Write(FLASH_BIN_ADDR + iPos,(u16*)&iRet,2); //写入升级使能
		iPos += 4;
		iRet = iTotal;
		printf("flash write upgrade total size");
		STMFLASH_Write(FLASH_BIN_ADDR + iPos,(u16*)&iRet,2); //写入文件长度
		iPos += 4;
		printf("flash write upgrade data1!\r\n");
		iLen = iLen - (pchTemp-OTA_RX_Buf);  //去掉HTTP头长度 获取此次读取的升级包长度
		printf("去掉包头后数据长度：%d\r\n", iLen);
		while(iLen >= 200)
		{
			STMFLASH_Write(FLASH_BIN_ADDR + iPos,(u16*)pchTemp,100);
			pchTemp += 200;
			iPos += 200;
			iLen -= 200;
		}
		printf("flash 写data1剩下的!\r\n");
		if(iLen > 0)
		{
			memcpy(OTA_RX_Buf,pchTemp,iLen);
			memset(OTA_RX_Buf+iLen, 0, EC600M_RX_BUF-iLen);
			iRet = OTA_GETdata_recv(200-iLen, 1, OTA_RX_Buf+iLen);
			if(iRet <= 0 || (iRet + iLen) != 200)
			{
				printf("recv file error %d  %d",iRet,200 - iLen);
				goto END;					
			}	
			STMFLASH_Write(FLASH_BIN_ADDR + iPos,(u16*)OTA_RX_Buf,100);
			iPos += 200;
		}
		IWDG_Feed(); 
		printf("第一包凑完200写完\r\n");
		iLen = OTA_GETdata_recv(600, 1, OTA_RX_Buf);
		while((iPos - 8) < iTotal)
		{
			if(iLen == 600)
			{
				printf("flash write upgrade progress, %d\r\n",iPos);
				STMFLASH_Write(FLASH_BIN_ADDR + iPos,(u16*)OTA_RX_Buf,300);
				iPos += iLen;
				iLen = OTA_GETdata_recv(600, 1, OTA_RX_Buf);
				IWDG_Feed(); 
			}
			else
			{
				if((iPos + iLen - 8) == iTotal)
				{
					printf("flash write upgrade end, %d\r\n",iTotal);
					STMFLASH_Write(FLASH_BIN_ADDR + iPos,(u16*)OTA_RX_Buf,300);
					iRet = 1;
					if(iLen > 8)
					{
						printf("tail data %02x%02x%02x%02x%02x%02x%02x%02x\r\n",
						      OTA_RX_Buf[iLen-8],OTA_RX_Buf[iLen-7],OTA_RX_Buf[iLen-6],OTA_RX_Buf[iLen-5],
						      OTA_RX_Buf[iLen-4],OTA_RX_Buf[iLen-3],OTA_RX_Buf[iLen-2],OTA_RX_Buf[iLen-1]);
					}
					printf("recv file ok\r\n");
					break;
				}
				memcpy(OTA_RX_Buf,pchTemp,iLen);
				memset(OTA_RX_Buf+iLen, 0, EC600M_RX_BUF-iLen);
				iRet = OTA_GETdata_recv(600-iLen, 1, OTA_RX_Buf+iLen);  //(200-iLen, 10, OTA_RX_Buf+iLen)
				if(iRet <= 0 || (iRet + iLen) != 600)
				{
					printf("recv file error %d  %d\r\n",iRet,600 - iLen);
					goto END;					
				}
				iLen = 600;
				IWDG_Feed(); 
			}
			printf("recv file percent is %d %d\r\n",(iPos - 8),iTotal);
		}
		if((iPos - 8) >= iTotal)
		{		
			printf("recv file ok......\r\n");
			iRet = 1;
		}
	}
END:
	http_tcpclient_close(socket_fd);
	vPortFree(pchHost);
	vPortFree(pchUrl);
	vPortFree(pchBurHead);
	printf("getUpdateFile end....!\r\n");
	IWDG_Feed(); 
	return iRet;	
}
/********************************************************/
int http_check_result(const char*lpbuf)
{
	char *ptmp = NULL; 
	ptmp = (char*)strstr(lpbuf,"HTTP/1.1");
   if(!ptmp || atoi(ptmp + 9) != 200)
	{
       return 0;
   }
   IWDG_Feed(); 
	return 1;
}
/********************************************************/
int http_file_size(const char*lpbuf)
{
	int iRet = 0;
	char* pchLength = NULL;
	char* pchLine = NULL;
	char  pchTemp[10];
	
	if(lpbuf == NULL)
	{
		DEBUG("param is NULL");
		goto END;
	}
	if((strstr(lpbuf,"Content-Length")) == NULL)
	{
		DEBUG("Content-Length is not find");
		goto END;		
	}
	pchLength = strstr(lpbuf,"Content-Length:")+strlen("Content-Length: ");
	pchLine = strstr(pchLength,"\r\n");
	if((pchLine - pchLength) > 9)
	{
		DEBUG("Content-Length is too long");
		goto END;
	}
	memset(pchTemp,0,10);
	memcpy(pchTemp,pchLength,pchLine-pchLength);
	iRet = atoi(pchTemp);
	if(iRet > HTTP_FILE_MAXLEN)
	{
		DEBUG("Content-Length is too long > %d ",HTTP_FILE_MAXLEN);
		iRet = 0;
	}
END:
	IWDG_Feed(); 
	return iRet;
}
/*******************************************************/
int ParseIntValue(char* json,char* key)
{
	int iRet = 0;
	char pchTemp[20];
	char* pstr1;
	char* pstr2;
	memset(pchTemp,0,20);
	sprintf(pchTemp,"\"%s\":",key);
	pstr1 = strstr(json,pchTemp);
	if(pstr1 != NULL)
	{
		memset(pchTemp,0,20);
		pstr2 = strstr(pstr1 + (strlen(key) + 3),",");
		if(pstr2 != NULL)
		{
			memcpy(pchTemp,pstr1 + (strlen(key) + 3), pstr2 - pstr1 - (strlen(key) + 3));
			iRet = atoi(pchTemp);
		}
		else
		{
			pstr2 = strstr(pstr1 + (strlen(key) + 3),"}");
			if(pstr2 != NULL)
			{
				memcpy(pchTemp,pstr1 + (strlen(key) + 3), pstr2 - pstr1 - (strlen(key) + 3));
				iRet = atoi(pchTemp);
			}
		}
	}
	return iRet; 
}	

/*******************************************************/
int ParseStringValue(char* json,char* key,char* value)
{
	int iRet = -1;
	char pchTemp[30];
	char* pstr1;
	char* pstr2;
	memset(pchTemp,0,30);
	sprintf(pchTemp,"\"%s\":",key); 
	pstr1 = strstr(json,pchTemp);
	if(pstr1 != NULL)
	{
		if(pstr1[strlen(key) + 3] != '\"')
		{
			strcpy(value,"null");
			iRet = 0;
			goto END;
		}
		pstr2 = strstr(pstr1 + (strlen(key) + 4),"\"");
		if(pstr2 != NULL)
		{
			memcpy(value, pstr1 + (strlen(key) + 4), 
			pstr2 - pstr1 - (strlen(key) + 4));
			iRet = 1;
		}
	}
END: 
	return iRet;
}


