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
char ATOPEN[60] = {0};         //AT+OPEN����BUF
// char BGtask_ATOPEN[60];         //��̨����ʹ�õ�AT+OPEN����BUF
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

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_11;				//PA.8/11 �˿�����
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 						//�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;						//IO���ٶ�Ϊ50MHz
  GPIO_Init(GPIOA, &GPIO_InitStructure);					 						//�����趨������ʼ��GPIOA.0/1
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;									//PA.8/11 �˿�����
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 						//��©���
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;						//IO���ٶ�Ϊ50MHz
  GPIO_Init(GPIOC, &GPIO_InitStructure);					 						//�����趨������ʼ��GPIOA.0/1
	
  GPIO_ResetBits(GPIOA, GPIO_Pin_8|GPIO_Pin_11);							//PA.8/11 ����� 4Gģ���Դ��
	// GPIO_SetBits(GPIOC, GPIO_Pin_13);														//4Gģ�鸴λ����
}

//
//
void EC600M_RUN(void)
{
  GPIO_4G_Init();
  delay_ms(500);
  GPIO_SetBits(GPIOA, GPIO_Pin_8);              //4Gģ���Դ��
  delay_ms(100);
  EC600_OPEN;                                   //4G����
}

/**
 @brief ��ʼ��
 @param ��
 @return 1 - �ɹ���0 - ʧ��
*/
void EC600M_Init(void)
{	
  char *strx,*extstrx;	
  BaseType_t err=pdFALSE;

  xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
  printf("EC600 INIT get mutex!!\r\n");
  strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT\r\n");       	
  gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
  DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                //��������
  printf("AT SEND SUCCESS!!\r\n");
  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);                  //��ȡ����
  if(err==pdTRUE)                                                          
  {
    strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                      //�����ؼ���
    while(strx == NULL)                                                                  //û������ 
    {
      clear_RXBuffer();                                                                 //û���յ���ȷ���� ��ս��ջ���
      // DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //�ٴη�������
      err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
      if(err==pdTRUE)                                                                   //������ڽ������
      {
        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                
      }
    }
    printf("AT�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
    clear_RXBuffer();
    clear_TXBuffer();
    strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"ATE0\r\n");       	                      //�رջ���
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //��������
	  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	                          
    if(err==pdTRUE)                                                                     
    {
      DEBUG("ATE0�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
      clear_RXBuffer();
      clear_TXBuffer();
      strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+CSQ\r\n");                          //���CSQ
      gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
      DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //��������
	    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
      if(err==pdTRUE)                                                                    
      {
        printf("AT+CSQ�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
        clear_RXBuffer();
        clear_TXBuffer();
        strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"ATI\r\n");                           //���ģ��汾��
        gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
        DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                         //��������
	      err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
        if(err==pdTRUE)                                                                     
        {
          DEBUG("ATI�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
          clear_RXBuffer();
          clear_TXBuffer();
          strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+CPIN?\r\n");                          //���SIM���Ƿ���
          gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
          DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //��������
          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
          if(err==pdTRUE)                                                                     //
          {	
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CPIN: READY");              //�����ؼ���
            while(strx == NULL)                                                                  //û������ 
            {
              clear_RXBuffer();                                                                    //û���յ���ȷ���� ��ս��ջ���
              // DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //�ٴη�������
              err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
              if(err==pdTRUE)                                                                   //������ڽ������
              {
                strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CPIN: READY");              
              }
            }
            DEBUG("AT+CPIN�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
            clear_RXBuffer();
            clear_TXBuffer();
            strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+CREG?\r\n");                          //�Ƿ�ע��GSM����
            gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
            DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //��������
            err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
            if(err==pdTRUE)                                                                     //
            {	
              strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CREG: 0,1");            //��������
              extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CREG: 0,5");        //�������� ����
              while(strx == NULL && extstrx == NULL)
              {
                clear_RXBuffer();                                                                 //û���յ���ȷ���� ��ս��ջ���
                DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //�ٴη�������
                err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
                if(err==pdTRUE)                                                                   //������ڽ������
                {
                  strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CREG: 0,1");        
                  extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CREG: 0,5");              
                } 
              }
              DEBUG("AT+CREG�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
              clear_RXBuffer();
              clear_TXBuffer();
              strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+CGREG?\r\n");                         //�Ƿ�ע��GPRS����
              gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
              DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //��������
              err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
              if(err==pdTRUE)                                                                     //
              {	
                strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CGREG: 0,1");           //ֻ��ע��ɹ��ſ��Խ���GPRS���ݴ���
                extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CGREG: 0,5");       //�������� ����
                while(strx == NULL && extstrx == NULL)
                {
                  clear_RXBuffer();                                                                 //û���յ���ȷ���� ��ս��ջ���
                  DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //�ٴη�������
                  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
                  if(err==pdTRUE)                                                                   //������ڽ������
                  {
                    strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CGREG: 0,1");        
                    extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CGREG: 0,5");              
                  } 
                }
                DEBUG("AT+CGREG�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                clear_RXBuffer();
                clear_TXBuffer();
                strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+COPS?\r\n");                         //�鿴ע�ᵽ�ĸ���Ӫ��
                gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //��������
                err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                if(err==pdTRUE)
                {
                  DEBUG("AT+COPS�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                  clear_RXBuffer();
                  clear_TXBuffer();
                  strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QICLOSE=0\r\n");                      //�ر�socket����
                  gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                  DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             //��������
                  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                  if(err==pdTRUE)                                                                     //
                  {	
                    DEBUG("AT+QICLOSE�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                    clear_RXBuffer();
                    clear_TXBuffer();
                    strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QICSGP=1,1,\"UNINET\",\"\",\"\",0\r\n"); //����APN ���û�������
                    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                            
                    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                    if(err==pdTRUE) 
                    {
                      strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                  //�����ɹ�
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
                      DEBUG("AT+QICSGP�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                      clear_RXBuffer();
                      clear_TXBuffer();
                      strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIDEACT=1\r\n");                      //ȥ�����
                      gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                      DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
                      err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                      if(err==pdTRUE) 
                      {
                        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                  //�����ɹ�
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
                        DEBUG("AT+QIDEACT�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                        clear_RXBuffer();
                        clear_TXBuffer();
                        strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIACT=1\r\n");                      //�����
                        gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                        DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
                        err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                        if(err==pdTRUE) 
                        {
                          strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                  //�����ɹ�
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
                          printf("AT+QIACT�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                          clear_RXBuffer();
                          clear_TXBuffer();
                          strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIACT?\r\n");                      //��ȡ��ǰ����IP��ַ
                          gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                          DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
                          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                          if(err==pdTRUE)
                          {
                            printf("AT+QIACT?�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
                            clear_RXBuffer();
                            clear_TXBuffer();
                            strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIDNSCFG=1,\"114.114.114.114\"\r\n");                  //����DNS
                            gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
                            DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
                            err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
                            if(err==pdTRUE) 
                            {
                                strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                                    //�����ɹ�
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
                                DEBUG("AT+QIDNSCFG�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
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
//��AT+QIOPEN�����
//serverip:������IP
//serverport:�������˿ں�
void PackATOPEN(int socketid, char* sockettype, char* serverip, u16 serverport)
{
    sprintf(ATOPEN,"AT+QIOPEN=1,%d,\"%s\",\"%s\",%d,0,1\r\n", socketid, sockettype, serverip, serverport);  
}
//��������
int EC600M_OpenSocket(int socketid, char* sockettype, u8* serverip, u16 serverport)
{
    char *strx=NULL;
    char *pchBuf=NULL;	
    int iRet = -1;
    char num = 0;
    BaseType_t err=pdFALSE;

    pchBuf = pvPortMalloc(15);
    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    DEBUG("EC600�������ӽ��յ��ź�����\r\n");
    sprintf((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QIDNSGIP=1,%s\r\n", serverip);        //DNS��������  \"temp.lilunxinli.com\"
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
    if(err==pdTRUE) 
    {
        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QIURC:");                          //�����ɹ�
        while(strx == NULL)
        {
          clear_RXBuffer();                                                                                          
          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
          if(err==pdTRUE)                                                                   
          {
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QIURC:");                     
          } 
        }
        printf("AT+QIDNSGIP�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
        clear_RXBuffer();
        clear_TXBuffer();
        PackATOPEN(socketid, sockettype, (char*)serverip, serverport);
        sprintf(pchBuf, "+QIOPEN: %d,0", socketid);
        strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,ATOPEN);                                       //UDP��¼���Ĳ�����Ϊֱ��ģʽ 
        memset(ATOPEN, 0 , 60);
        printf("+QIOPEN����:%s\r\n", gl_Ec600MObj.g_ec600m_TXBuf);
        gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
        DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);  
        DEBUG("��������_������ɣ�\r\n");                         
        err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
        if(err==pdTRUE)
        {
            printf("�������յ������ݣ�%s\r\n", gl_CMDtaskObj.CmdRecvBuf); 
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, (const char *)pchBuf);                  //����Ƿ��½�ɹ�
            while(strx == NULL)
            {
              clear_RXBuffer();                                                                 
              // DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                          
              err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf, 100);	
              printf("ѭ���ڽ������յ������ݣ�%s\r\n", gl_CMDtaskObj.CmdRecvBuf); 
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
            printf("AT+QIOPEN=1,%d�ظ�����:%s\r\n", socketid, gl_CMDtaskObj.CmdRecvBuf);
        }
    }
    END:
    clear_RXBuffer();
    clear_TXBuffer();
    vPortFree(pchBuf);
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
    DEBUG("EC600���������ͷ��ź�����\r\n");
    return iRet;
}
//�ر�����
void EC600M_CloseSocket(int socketid)
{
    char *strx,*extstrx;	
    BaseType_t err=pdFALSE;
    u8 circle = 0;

    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    printf("EC600�ر����ӽ����ź�����\r\n");
    sprintf((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QICLOSE=%d,10\r\n", socketid);
    printf("�ر�%d�����ӣ���\r\n", socketid);
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);
    DEBUG("�ر�����_������ɣ�\r\n"); 
    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY); 
    printf("�ر�����_������ɣ�%s\r\n", gl_CMDtaskObj.CmdRecvBuf); 
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
          clear_RXBuffer();                                                                 //û���յ���ȷ���� ��ս��ջ���
          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf, 100);	
          if(err==pdTRUE)                                                                   //������ڽ������
          {
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");        
            extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "ERROR");              
          } 
        }
        printf("AT+QICLOSE�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
        clear_RXBuffer();
        clear_TXBuffer(); 
    }
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
    printf("EC600�ر������ͷ��ź�����\r\n");
}
//��ѯsocket״̬
void Query_Socketstate(void)
{
    char *strx,*extstrx;	
    BaseType_t err=pdFALSE;

    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    DEBUG("��̨��ѯ����״̬�����ź�����\r\n");
    strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+QISTATE=0,1\r\n");                  //
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                             
    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
    if(err==pdTRUE) 
    {
        DEBUG("��ѯsocket״̬�յ��ظ�����\r\n");
        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QISTATE:");
        extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                                   
        while(strx == NULL && extstrx == NULL)
        {
          DEBUG("�������ݲ��ԣ���\r\n");
          DEBUG("�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
          clear_RXBuffer();                                                                 
          DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           
          err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
          if(err==pdTRUE)                                                                   
          {
            strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QISTATE:");
            extstrx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "OK");                    
          } 
        }
        DEBUG("AT+QISTATE=1,1�ظ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
        clear_RXBuffer();
        clear_TXBuffer();
    }
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
    DEBUG("��̨��ѯ����״̬�ͷ��ź�����\r\n");
}
/**
 @brief �������ݵ�UDP������
 @param socketid:�����������Ӻ�
 @param pchbuf:��������
 @param len:���ݳ���
 @return ��
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
    DEBUG("Send��������%s����\r\n", gl_Ec600MObj.g_ec600m_TXBuf);                            
    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
    DEBUG("4G���յ�����:%s\r\n", gl_CMDtaskObj.CmdRecvBuf); 
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
          DEBUG("~~~~~~~~~~~~~~~~~~~~~~�յ�������:%s\r\n",gl_CMDtaskObj.CmdRecvBuf);
          strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, ">");                     
        } 
      }
      circle = 0;
      clear_RXBuffer();
      clear_TXBuffer();
      memcpy(gl_Ec600MObj.g_ec600m_TXBuf, pchBuf, len);
      memset(pchBuf, 0, sizeof(pchBuf));                    
      DEBUG("���ݿ�������%d\r\n", len);
      if(socketid == 2)
      {
          DEBUG("EC600�������ݣ�%s\r\n", gl_Ec600MObj.g_ec600m_TXBuf);
      }
      else
      {
          printf("EC600�������ݣ�");
          for(i=0;i<len;i++)
          {
            printf("%02X",gl_Ec600MObj.g_ec600m_TXBuf[i]);
          }
          printf("\r\n");
      }
      gl_Ec600MObj.g_ec600m_TXCnt = len;
      DEBUG("������������%d\r\n", gl_Ec600MObj.g_ec600m_TXCnt);
      DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                 
      err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);
      DEBUG("���������ݵ���OK��%s\r\n", gl_CMDtaskObj.CmdRecvBuf);
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
        printf("4G���ͱ���~~�ɹ���\r\n"); 
        return gl_Ec600MObj.g_ec600m_TXCnt;
      }
    }
    clear_RXBuffer();
    clear_TXBuffer(); 
    return 0;
}

/**
 @brief �ӷ�������������
 @param overtime�����ճ�ʱʱ�� S
 @return ��Ч��Ϣ��ַ
*/
void* EC600M_Receive(u32 overtime)
{
  BaseType_t err=pdFALSE;
  int RecDataLen = 0;
  char* pTemp = NULL;	
  char* pchBuf = NULL;
  u8 i;

  overtime = overtime*1000;                                          //��ʱʱ�䵥λms
	// DEBUG("��ʱʱ�䣺%d\r\n", overtime);
  err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,overtime);	             //��ȡ�ź���
  if(err==pdTRUE)                                                    
  {
    // DEBUG("+++++++++++++++++++++++�յ��ظ�����++++++++++++++++++++++++\r\n");
    // DEBUG("�յ������ݣ�%s\r\n",gl_CMDtaskObj.CmdRecvBuf);
    if((pTemp = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+QIURC: \"recv\","))!= NULL)    // ����������ؼ���
    {
      sscanf(pTemp, "+QIURC: \"recv\",%d,%d\r\n", &gl_IcapDevObj.socketid, &RecDataLen);
      if(gl_IcapDevObj.socketid == BG_SOCKET)
      {
        xSemaphoreGive(gl_IcapDevObj.BGtaskSemaphore);
      }
      else if (gl_IcapDevObj.socketid == OTA_SOCKET)            //����Ǹ�OTA��������ݿ�����OTA_RX_Buf
      {
        memcpy(OTA_RX_Buf, (const char*)gl_CMDtaskObj.CmdRecvBuf, EC600M_RX_BUF);
        clear_RXBuffer();
        pTemp = strstr((const char *)OTA_RX_Buf, "+QIURC: \"recv\",");
      }
      else if (gl_IcapDevObj.socketid == NET_SOCKET)
      {
        gl_IcapDevObj.heartcountflag = NETTASK_RECVDATA;
      }
      printf("---------------------�յ�SOKET%d�ظ�����-------------------\r\n", gl_IcapDevObj.socketid);
      // DEBUG("�յ����ݳ��ȣ�%d\r\n", RecDataLen);
      pchBuf = pvPortMalloc(30);
      memset(pchBuf, 0 ,30);
      sprintf(pchBuf,"+QIURC: \"recv\",%d,%d\r\n", gl_IcapDevObj.socketid, RecDataLen);
      pTemp += strlen(pchBuf);
      if(gl_IcapDevObj.socketid == 2)
      {
        DEBUG("���յ�%d�������ݣ�%s\r\n", RecDataLen, pTemp);
      }
      else
      {
        DEBUG("���յ�%d�������ݣ�", RecDataLen);
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
    printf("get CSQ_��ȡmutex����\r\n");
    clear_RXBuffer();
    clear_TXBuffer();
    strcpy((char*)gl_Ec600MObj.g_ec600m_TXBuf,"AT+CSQ\r\n");                          //���CSQ
    gl_Ec600MObj.g_ec600m_TXCnt = strlen((char*)gl_Ec600MObj.g_ec600m_TXBuf);
    DMA_Enable(DMA1_Channel4, gl_Ec600MObj.g_ec600m_TXCnt);                           //��������
    err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
    if(err==pdTRUE)                                                                    
    {  
        strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CSQ:");                      //�����ؼ���
        while(strx == NULL)                                                                  //û������ 
        {
          	if(circle == 30)
            {
              goto END;
            }
            circle++;
            clear_RXBuffer();                                                                 //û���յ���ȷ���� ��ս��ջ���
            err=xQueueReceive(gl_Ec600MObj.EC600M_RxData_Queue, gl_CMDtaskObj.CmdRecvBuf,portMAX_DELAY);	
            if(err==pdTRUE)                                                                   //������ڽ������
            {
              strx = strstr((const char *)gl_CMDtaskObj.CmdRecvBuf, "+CSQ:");                
            }
        }
        sscanf(strx, "+CSQ: %d,", &g_oADCSearchObj.iCSQ);
        printf("get CSQ��%d\r\n", g_oADCSearchObj.iCSQ); 
    }
    END:
    clear_RXBuffer();
    clear_TXBuffer();
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
    printf("get CSQ_�ͷ�mutex����\r\n");
}
/**
 @brief ��ջ���
 @param ��
 @return ��
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
 @brief ����ģ��
 @param ��
 @return ��
*/
void reset(void)
{
  printf("4Gģ�鸴λ!\r\n");
  GPIO_ResetBits(GPIOA, GPIO_Pin_8);
  vTaskDelay(2000);
  GPIO_SetBits(GPIOA, GPIO_Pin_8);
  delay_ms(100);
  EC600_OPEN;   
}

/****************************************************END OF FILE****************************************************/
