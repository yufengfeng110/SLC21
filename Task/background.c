#include <string.h>
#include <stdlib.h>
#include "background.h"
#include "FreeRTOS.h"
#include "task.h"
#include "DefBean.h"
#include "rtc.h"
#include "ec600m.h"
#include "cmddeal.h"
#include "24cxx.h"
#include "wdg.h"

u8   BGtask_Txbuf[100] = {0};
u32  bgtasknum = 0;

void BGtaskCmd(char* buf,u8 type,u16 cmd,u8* pctrid,u32 ledid,u32* serial);

void BackGround_task(void *pvParameters)
{
	u32 BGtask_notivalue;     //��̨����ֵ֪ͨ 
	u32 alarm_timer = 0;
	u8 time = 0;
	BaseType_t err=pdFALSE;
	
	while(1)
	{
		printf("@@@@BackGround_task running!\r\n");
		BGtask_notivalue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);	                //�������½�������
		if(BGtask_notivalue == 1)
		{
			EC600M_GetCSQ();
			alarm_timer = RTC_GetCounter();
			alarm_timer += 59;      //ÿ��60s����һ�κ�̨����
			AlarmRTCSet(alarm_timer);
			srand(alarm_timer);
			DEBUG("####@@@@@@@!!!!!!!!\r\n");
			EC600M_OpenSocket(1, "UDP", "39.98.57.97", 7900);
			bgtasknum = rand();
			BGtaskCmd((char*)BGtask_Txbuf, CMD_DEALREPLY, CMD_SigninHeart, gl_LampConfigObj.ControllerID,
					0x00, &bgtasknum);
			printf("�������������԰����ͳɹ���\r\n");
			while(time < 10)      //ÿ���յ����ݺ���ٵ�60s
			{
				time++;
				DEBUG("@@@@@@@@@@@@@@@@@@@@@@@@@BGtask�е���ʱ��%d\r\n", time);
				err=xSemaphoreTake(gl_IcapDevObj.BGtaskSemaphore,0);
				if(err==pdTRUE)    //�յ��ź���
				{
					time = 0;
				}
				vTaskDelay(1000);
			}
			time = 0;
			EC600M_CloseSocket(BG_SOCKET);
		}
		IWDG_Feed(); 
		//  vTaskDelay(1000);
	}	
}


//��̨��������
void BGtaskCmd(char* buf,u8 type,u16 cmd,u8* pctrid,u32 ledid,u32* serial)
{
    u8 ctrid[4] = {0};
    u8 BGtaskTXCnt = 0;
    StrToHexArray( (unsigned char*) pctrid, (unsigned char*)ctrid );
    ((CMDHeadObj*)buf)->CMDType = type;                         
    ((CMDHeadObj*)buf)->CMDSerialnum = __REV(*serial);
    ((CMDHeadObj*)buf)->CMDLength = 10;                             
    ((CMDHeadObj*)buf)->CMDID = __REV16(cmd);
    ((CMDHeadObj*)buf)->ControllerID = *((u32*)ctrid);
    ((CMDHeadObj*)buf)->luminiareID = __REV(ledid);
    PackAddStringParam((char*)BGtask_Txbuf,SERIALNUM,(u8*)gl_IcapDevObj.DevSerialnum);
    PackMsgCrc((char*)BGtask_Txbuf);
    BGtaskTXCnt = __REV16(((CMDHeadObj*)BGtask_Txbuf)->CMDLength) + 13; //��������
	xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY); 
    EC600M_Send(BG_SOCKET, (char*)BGtask_Txbuf, BGtaskTXCnt);
	xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
    memset(BGtask_Txbuf, 0, 100);
}



