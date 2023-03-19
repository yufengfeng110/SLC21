#include "ec600m.h"
#include <stdlib.h>
#include <string.h>
#include "stm32f10x.h"
#include <stdio.h>
#include "dma.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "usart.h"
#include "delay.h"
#include "task.h"
#include "DefBean.h"
#include "netcommutask.h"
#include "otaupgradetask.h"



/*********************************************************************
 * GLOBAL VARIABLES
 */ 
char ATOPEN[60] = {0};         //AT+OPEN命令BUF
// char BGtask_ATOPEN[60];         //后台任务使用的AT+OPEN命令BUF
Ec600NetObj  gl_Ec600MObj;                                       
/*********************************************************************/

void clear_RXBuffer(void);
void clear_TXBuffer(void);
//void reset(void);

//
//
void GPIO_4G_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_11;				//PA.8/11 端口配置
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 						//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;						//IO口速度为50MHz
  GPIO_Init(GPIOA, &GPIO_InitStructure);					 						//根据设定参数初始化GPIOA.0/1
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;									//PA.8/11 端口配置
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 						//开漏输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;						//IO口速度为50MHz
  GPIO_Init(GPIOC, &GPIO_InitStructure);					 						//根据设定参数初始化GPIOA.0/1
	
  GPIO_ResetBits(GPIOA, GPIO_Pin_8|GPIO_Pin_11);							//PA.8/11 输出低 4G模块电源关
	// GPIO_SetBits(GPIOC, GPIO_Pin_13);														//4G模块复位控制
}

//
//
void EC600M_RUN(void)
{
  GPIO_4G_Init();
  delay_ms(500);
  GPIO_SetBits(GPIOA, GPIO_Pin_8);              //4G模块电源开
  delay_ms(100);
  EC600_OPEN;                                   //4G开机
}

/**
 @brief 初始化
 @param 无
 @return 1 - 成功；0 - 失败
*/
void EC600M_Init(void)
{	
  char *strx,*extstrx;	
  BaseType_t err=pdFALSE;

  xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
  printf("EC600 INIT get mutex!!\r\n");
  strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT\r\n");       	
  gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
  DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                //发送命令
  printf("AT SEND SUCCESS!!\r\n");
  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);                  //获取队列
  if(err==pdTRUE)                                                          
  {
    strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                      //检索关键词
    while(strx == NULL)                                                                  //没检索到 
    {
      clear_RXBuffer();                                                                 //没有收到正确数据 清空接收缓存
      // DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //再次发送命令
      err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
      if(err==pdTRUE)                                                                   //如果串口接收完成
      {
        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                
      }
    }
    printf("AT回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
    clear_RXBuffer();
    clear_TXBuffer();
    strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"ATE0\r\n");       	                      //关闭回显
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //发送命令
	  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	                          
    if(err==pdTRUE)                                                                     
    {
      DEBUG("ATE0回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
      clear_RXBuffer();
      clear_TXBuffer();
      strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+CSQ\r\n");                          //检查CSQ
      gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
      DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //发送命令
	    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
      if(err==pdTRUE)                                                                    
      {
        printf("AT+CSQ回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
        clear_RXBuffer();
        clear_TXBuffer();
        strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"ATI\r\n");                           //检查模块版本号
        gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
        DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                         //发送命令
	      err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
        if(err==pdTRUE)                                                                     
        {
          DEBUG("ATI回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
          clear_RXBuffer();
          clear_TXBuffer();
          strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+CPIN?\r\n");                          //检查SIM卡是否在
          gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
          DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //发送命令
          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
          if(err==pdTRUE)                                                                     //
          {	
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CPIN: READY");              //检索关键词
            while(strx == NULL)                                                                  //没检索到 
            {
              clear_RXBuffer();                                                                    //没有收到正确数据 清空接收缓存
              // DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //再次发送命令
              err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
              if(err==pdTRUE)                                                                   //如果串口接收完成
              {
                strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CPIN: READY");              
              }
            }
            DEBUG("AT+CPIN回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
            clear_RXBuffer();
            clear_TXBuffer();
            strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+CREG?\r\n");                          //是否注册GSM网络
            gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
            DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //发送命令
            err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
            if(err==pdTRUE)                                                                     //
            {	
              strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CREG: 0,1");            //返回正常
              extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CREG: 0,5");        //返回正常 漫游
              while(strx == NULL && extstrx == NULL)
              {
                clear_RXBuffer();                                                                 //没有收到正确数据 清空接收缓存
                DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //再次发送命令
                err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
                if(err==pdTRUE)                                                                   //如果串口接收完成
                {
                  strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CREG: 0,1");        
                  extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CREG: 0,5");              
                } 
              }
              DEBUG("AT+CREG回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
              clear_RXBuffer();
              clear_TXBuffer();
              strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+CGREG?\r\n");                         //是否注册GPRS网络
              gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
              DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //发送命令
              err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
              if(err==pdTRUE)                                                                     //
              {	
                strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CGREG: 0,1");           //只有注册成功才可以进行GPRS数据传输
                extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CGREG: 0,5");       //返回正常 漫游
                while(strx == NULL && extstrx == NULL)
                {
                  clear_RXBuffer();                                                                 //没有收到正确数据 清空接收缓存
                  DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //再次发送命令
                  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
                  if(err==pdTRUE)                                                                   //如果串口接收完成
                  {
                    strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CGREG: 0,1");        
                    extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CGREG: 0,5");              
                  } 
                }
                DEBUG("AT+CGREG回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                clear_RXBuffer();
                clear_TXBuffer();
                strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+COPS?\r\n");                         //查看注册到哪个运营商
                gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //发送命令
                err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                if(err==pdTRUE)
                {
                  DEBUG("AT+COPS回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                  clear_RXBuffer();
                  clear_TXBuffer();
                  strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QICLOSE=0\r\n");                      //关闭socket连接
                  gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                  DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //发送命令
                  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                  if(err==pdTRUE)                                                                     //
                  {	
                    DEBUG("AT+QICLOSE回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                    clear_RXBuffer();
                    clear_TXBuffer();
                    strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QICSGP=1,1,\"UNINET\",\"\",\"\",0\r\n"); //接入APN 无用户名密码
                    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                            
                    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                    if(err==pdTRUE) 
                    {
                      strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                  //开启成功
                      while(strx == NULL)
                      {
                        clear_RXBuffer();                                                                 
                        DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           
                        err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
                        if(err==pdTRUE)                                                                   
                        {
                          strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                     
                        } 
                      }
                      DEBUG("AT+QICSGP回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                      clear_RXBuffer();
                      clear_TXBuffer();
                      strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIDEACT=1\r\n");                      //去激活场景
                      gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                      DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
                      err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                      if(err==pdTRUE) 
                      {
                        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                  //开启成功
                        while(strx == NULL)
                        {
                          clear_RXBuffer();                                                                 
                          // DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           
                          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
                          if(err==pdTRUE)                                                                   
                          {
                            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                     
                          } 
                        }
                        DEBUG("AT+QIDEACT回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                        clear_RXBuffer();
                        clear_TXBuffer();
                        strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIACT=1\r\n");                      //激活场景
                        gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                        DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
                        err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                        if(err==pdTRUE) 
                        {
                          strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                  //开启成功
                          while(strx == NULL)
                          {
                            clear_RXBuffer();                                                                 
                            // DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           
                            err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
                            if(err==pdTRUE)                                                                   
                            {
                              strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                     
                            } 
                          }
                          printf("AT+QIACT回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                          clear_RXBuffer();
                          clear_TXBuffer();
                          strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIACT?\r\n");                      //获取当前卡的IP地址
                          gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                          DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
                          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                          if(err==pdTRUE)
                          {
                            printf("AT+QIACT?回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                            clear_RXBuffer();
                            clear_TXBuffer();
                            strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIDNSCFG=1,\"114.114.114.114\"\r\n");                  //设置DNS
                            gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                            DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
                            err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                            if(err==pdTRUE) 
                            {
                                strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                                    //开启成功
                                while(strx == NULL)
                                {
                                  clear_RXBuffer();                                                                 
                                  // DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           
                                  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
                                  if(err==pdTRUE)                                                                   
                                  {
                                    strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                     
                                  } 
                                }
                                DEBUG("AT+QIDNSCFG回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                                clear_RXBuffer();
                                clear_TXBuffer();
                            }
                          }
                        }
                      } 
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
}
//组AT+QIOPEN命令包
//serverip:服务器IP
//serverport:服务器端口号
void PackATOPEN(int socketid, char* sockettype, char* serverip, u16 serverport)
{
    sprintf(ATOPEN,"AT+QIOPEN=1,%d,\"%s\",\"%s\",%d,0,1\r\n", socketid, sockettype, serverip, serverport);  
}
//建立连接
int EC600M_OpenSocket(int socketid, char* sockettype, u8* serverip, u16 serverport)
{
    char *strx=NULL;
    char *pchBuf=NULL;	
    int iRet = -1;
    char num = 0;
    BaseType_t err=pdFALSE;

    pchBuf = pvPortMalloc(15);
    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    DEBUG("EC600创建链接接收到信号量！\r\n");
    sprintf((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIDNSGIP=1,%s\r\n", serverip);        //DNS解析域名  \"temp.lilunxinli.com\"
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
    if(err==pdTRUE) 
    {
        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QIURC:");                          //开启成功
        while(strx == NULL)
        {
          clear_RXBuffer();                                                                                          
          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
          if(err==pdTRUE)                                                                   
          {
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QIURC:");                     
          } 
        }
        printf("AT+QIDNSGIP回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
        clear_RXBuffer();
        clear_TXBuffer();
        PackATOPEN(socketid, sockettype, (char*)serverip, serverport);
        sprintf(pchBuf, "+QIOPEN: %d,0", socketid);
        strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,ATOPEN);                                       //UDP登录中心并设置为直吐模式 
        memset(ATOPEN, 0 , 60);
        printf("+QIOPEN命令:%s\r\n", gl_Ec600MObj.g_ec600m_TXBuf);
        gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
        DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);  
        DEBUG("创建链接_发送完成！\r\n");                         
        err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
        if(err==pdTRUE)
        {
            printf("建链接收到的数据：%s\r\n", gl_CMDtaskObj.CmdRecvBuf); 
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, (const char *)pchBuf);                  //检查是否登陆成功
            while(strx == NULL)
            {
              clear_RXBuffer();                                                                 
              // DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                          
              err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf, 100);	
              printf("循环内建链接收到的数据：%s\r\n", gl_CMDtaskObj.CmdRecvBuf); 
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
            iRet = 1;
            printf("AT+QIOPEN=1,%d回复命令:%s\r\n", socketid, gl_CMDtaskObj.CmdRecvBuf);
        }
    }
    END:
    clear_RXBuffer();
    clear_TXBuffer();
    vPortFree(pchBuf);
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
    DEBUG("EC600创建链接释放信号量！\r\n");
    return iRet;
}
//关闭连接
void EC600M_CloseSocket(int socketid)
{
    char *strx,*extstrx;	
    BaseType_t err=pdFALSE;
    u8 circle = 0;

    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    printf("EC600关闭链接接收信号量！\r\n");
    sprintf((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QICLOSE=%d,10\r\n", socketid);
    printf("关闭%d号链接！！\r\n", socketid);
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);
    DEBUG("关闭链接_发送完成！\r\n"); 
    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY); 
    printf("关闭链接_发送完成：%s\r\n", gl_CMDtaskObj.CmdRecvBuf); 
    if(err==pdTRUE)
    {
        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK"); 
        extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "ERROR");
        while(strx == NULL && extstrx == NULL)
        {
          if(circle == 30)
          {
              break;
          }
          circle++;
          clear_RXBuffer();                                                                 //没有收到正确数据 清空接收缓存
          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf, 100);	
          if(err==pdTRUE)                                                                   //如果串口接收完成
          {
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");        
            extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "ERROR");              
          } 
        }
        printf("AT+QICLOSE回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
        clear_RXBuffer();
        clear_TXBuffer(); 
    }
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
    printf("EC600关闭链接释放信号量！\r\n");
}
//查询socket状态
void Query_Socketstate(void)
{
    char *strx,*extstrx;	
    BaseType_t err=pdFALSE;

    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    DEBUG("后台查询链接状态接收信号量！\r\n");
    strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QISTATE=0,1\r\n");                  //
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
    if(err==pdTRUE) 
    {
        DEBUG("查询socket状态收到回复！！\r\n");
        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QISTATE:");
        extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                                   
        while(strx == NULL && extstrx == NULL)
        {
          DEBUG("但是数据不对！！\r\n");
          DEBUG("回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
          clear_RXBuffer();                                                                 
          DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           
          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
          if(err==pdTRUE)                                                                   
          {
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QISTATE:");
            extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                    
          } 
        }
        DEBUG("AT+QISTATE=1,1回复命令:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
        clear_RXBuffer();
        clear_TXBuffer();
    }
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
    DEBUG("后台查询链接状态释放信号量！\r\n");
}
/**
 @brief 发送数据到UDP服务器
 @param socketid:发送数据链接号
 @param pchbuf:发送数据
 @param len:数据长度
 @return 无
*/
int EC600M_Send(int socketid, char *pchBuf, u16 len)
{
    char *strx;	
    BaseType_t err=pdFALSE;
    u8 i;
    u8 circle = 0;

    sprintf(gl_Ec600MObj.g_ec600m_TXBuf, "AT+QISEND=%d,%d\r\n", socketid, len);
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt); 
    DEBUG("Send发送数据%s！！\r\n", gl_Ec600MObj.g_ec600m_TXBuf);                            
    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
    DEBUG("4G接收到数据:%s\r\n", gl_CMDtaskObj.CmdRecvBuf); 
    if(err==pdTRUE)
    {
      strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, ">"); 
      while(strx == NULL)
      {
        if(circle == 30)
        {
            clear_RXBuffer();
            clear_TXBuffer(); 
            return 0;
        }
        circle++;
        clear_RXBuffer();                                                                          
        err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
        if(err==pdTRUE)                                                                   
        {
          DEBUG("~~~~~~~~~~~~~~~~~~~~~~收到的数据:%s\r\n",gl_CMDtaskObj.CmdRecvBuf);
          strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, ">");                     
        } 
      }
      circle = 0;
      clear_RXBuffer();
      clear_TXBuffer();
      memcpy(gl_Ec600MObj.g_ec600m_TXBuf, pchBuf, len);
      memset(pchBuf, 0, sizeof(pchBuf));                    
      DEBUG("数据拷贝数：%d\r\n", len);
      if(socketid == 2)
      {
          DEBUG("EC600发送数据：%s\r\n", gl_Ec600MObj.g_ec600m_TXBuf);
      }
      else
      {
          printf("EC600发送数据：");
          for(i=0;i<len;i++)
          {
            printf("%02X",gl_Ec600MObj.g_ec600m_TXBuf[i]);
          }
          printf("\r\n");
      }
      gl_Ec600MObj.g_ec600m_TXCnt = len;
      DEBUG("发送数据数：%d\r\n", gl_Ec600MObj.g_ec600m_TXCnt);
      DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                 
      err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
      DEBUG("发送完数据等着OK：%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
      if(err==pdTRUE)
      {
        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "SEND OK");
        while(strx == NULL)
        {
          if(circle == 30)
          {
              clear_RXBuffer();
              clear_TXBuffer(); 
              return 0;
          }
          circle++;
          clear_RXBuffer();                                                                 
          // DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           
          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
          if(err==pdTRUE)                                                                   
          {
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "SEND OK");                     
          } 
        }
        clear_RXBuffer();
        clear_TXBuffer(); 
        printf("4G发送倍儿~~成功！\r\n"); 
        return gl_Ec600MObj.g_ec600m_TXCnt;
      }
    }
    clear_RXBuffer();
    clear_TXBuffer(); 
    return 0;
}

/**
 @brief 从服务器接收数据
 @param overtime：接收超时时间 S
 @return 有效消息地址
*/
void* EC600M_Receive(u32 overtime)
{
  BaseType_t err=pdFALSE;
  int RecDataLen = 0;
  char* pTemp = NULL;	
  char* pchBuf = NULL;
  u8 i;

  overtime = overtime*1000;                                          //超时时间单位ms
	// DEBUG("超时时间：%d\r\n", overtime);
  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,overtime);	             //获取信号量
  if(err==pdTRUE)                                                    
  {
    // DEBUG("+++++++++++++++++++++++收到回复！！++++++++++++++++++++++++\r\n");
    // DEBUG("收到的内容：%s\r\n",gl_CMDtaskObj.CmdRecvBuf);
    if((pTemp = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QIURC: \"recv\","))!= NULL)    // 如果检索到关键词
    {
      sscanf(pTemp, "+QIURC: \"recv\",%d,%d\r\n", &gl_IcapDevObj.socketid, &RecDataLen);
      if(gl_IcapDevObj.socketid == BG_SOCKET)
      {
        xSemaphoreGive(gl_IcapDevObj.BGtaskSemaphore);
      }
      else if (gl_IcapDevObj.socketid == OTA_SOCKET)            //如果是给OTA任务的数据拷贝到OTA_RX_Buf
      {
        memcpy(OTA_RX_Buf, (const char*)gl_CMDtaskObj.CmdRecvBuf, EC600M_RX_BUF);
        clear_RXBuffer();
        pTemp = strstr((const char *)OTA_RX_Buf, "+QIURC: \"recv\",");
      }
      else if (gl_IcapDevObj.socketid == NET_SOCKET)
      {
        gl_IcapDevObj.heartcountflag = NETTASK_RECVDATA;
      }
      printf("---------------------收到SOKET%d回复！！-------------------\r\n", gl_IcapDevObj.socketid);
      // DEBUG("收到数据长度：%d\r\n", RecDataLen);
      pchBuf = pvPortMalloc(30);
      memset(pchBuf, 0 ,30);
      sprintf(pchBuf,"+QIURC: \"recv\",%d,%d\r\n", gl_IcapDevObj.socketid, RecDataLen);
      pTemp += strlen(pchBuf);
      if(gl_IcapDevObj.socketid == 2)
      {
        DEBUG("接收到%d长的数据：%s\r\n", RecDataLen, pTemp);
      }
      else
      {
        DEBUG("接收到%d长的数据：", RecDataLen);
        for(i=0;i<RecDataLen;i++)
        {
          DEBUG("%02X",pTemp[i]);
        }
        DEBUG("\r\n");
      }

      vPortFree(pchBuf);
    }
  }
  return pTemp;
}

/*************************************************************/
void EC600M_GetCSQ(void)
{
    BaseType_t err=pdFALSE;
    char 	*strx = NULL;
    u8 		circle = 0;

    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    printf("get CSQ_获取mutex！！\r\n");
    clear_RXBuffer();
    clear_TXBuffer();
    strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+CSQ\r\n");                          //检查CSQ
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //发送命令
    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
    if(err==pdTRUE)                                                                    
    {  
        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CSQ:");                      //检索关键词
        while(strx == NULL)                                                                  //没检索到 
        {
          	if(circle == 30)
            {
              goto END;
            }
            circle++;
            clear_RXBuffer();                                                                 //没有收到正确数据 清空接收缓存
            err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
            if(err==pdTRUE)                                                                   //如果串口接收完成
            {
              strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CSQ:");                
            }
        }
        sscanf(strx, "+CSQ: %d,", &g_oADCSearchObj.iCSQ);
        printf("get CSQ：%d\r\n", g_oADCSearchObj.iCSQ); 
    }
    END:
    clear_RXBuffer();
    clear_TXBuffer();
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
    printf("get CSQ_释放mutex！！\r\n");
}
/**
 @brief 清空缓存
 @param 无
 @return 无
*/
void clear_RXBuffer(void)
{
  memset(gl_CMDtaskObj.CmdRecvBuf, 0, EC600M_RX_BUF);
  gl_Ec600MObj.g_ec600m_RXCnt = 0;
}
/*******************************************************/
void clear_TXBuffer(void)
{
  memset(gl_Ec600MObj.g_ec600m_TXBuf, 0, EC600M_BUF_NUM);
}

/**
 @brief 重启模块
 @param 无
 @return 无
*/
void reset(void)
{
  printf("4G模块复位!\r\n");
  GPIO_ResetBits(GPIOA, GPIO_Pin_8);
  vTaskDelay(2000);
  GPIO_SetBits(GPIOA, GPIO_Pin_8);
  delay_ms(100);
  EC600_OPEN;   
}

/****************************************************END OF FILE****************************************************/
