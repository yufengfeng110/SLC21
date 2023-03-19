#include <stdlib.h>
#include "storagetask.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "DefBean.h"
#include "24cxx.h"
#include "cmddeal.h"
#include "wdg.h"

extern	TaskHandle_t NetCommTask_Handler;

//stm32����
void SoftReset(void)
{
	printf("��ʼ��λ������\r\n");
	__set_FAULTMASK(1);       // �ر������ж�
	NVIC_SystemReset();       // ��λ
}

void Storage_task(void *pvParameters)
{
    EventBits_t EventValue;
    u8 ec600reconflag = 0;           //4G�������ӱ�־λ
    u8 restoreflag = 0;              //�ָ��������ñ�־λ
    u16 restoretime = 0;             //�ָ�����������ʱʱ��

    while(1)
    {
        printf("Storage_task running!\r\n");
        EventValue = xEventGroupGetBits(gl_IcapDevObj.oEventGroupHandler);
        if(EventValue != 0)
        {
            if((EventValue&0x01) != 0)
            {
                AT24CXX_Write(0, (u8*)gl_LampConfigObj.ControllerID, 10);
                DEBUG("ControllerID�����洢��ϣ�\r\n"); 
            }
            if((EventValue&0x02) != 0)
            {
                AT24CXX_Write(10, (u8*)gl_LampConfigObj.ServerIP, 30);
                DEBUG("ServerIP�����洢��ϣ�\r\n"); 
                ec600reconflag = 1;
            }
            if((EventValue&0x04) != 0)
            {
                AT24CXX_Write(40, (u8*)gl_LampConfigObj.ServerPort, 2);
                DEBUG("ServerPort�����洢��ϣ�\r\n"); 
                ec600reconflag = 1;
            }
            if((EventValue&0x08) != 0)
            {
                AT24CXX_Write(42, &(gl_LampConfigObj.RestoreFlag), 1);
                DEBUG("RestoreFlag�����洢��ϣ�\r\n"); 
                restoreflag = 1;
            }
            if((EventValue&0x10) != 0)
            {
                AT24CXX_Write(43, (u8*)(&(g_oADCSearchObj.oAC220_In.fdQ)), 4);
                AT24CXX_Write(47, (u8*)(&(g_oADCSearchObj.oAC220_Out[0].fdQ)), 4);
                AT24CXX_Write(51, (u8*)(&(g_oADCSearchObj.oAC220_Out[1].fdQ)), 4);
                DEBUG("���������洢��ϣ�\r\n"); 
            }
            if(ec600reconflag == 1)                             //IP or PORT�κ�һ���������ö�Ҫ������������
            {
                xTaskNotifyGive(NetCommTask_Handler);           //���簴�������õ�IP��������
                ec600reconflag = 0;
            }
            xEventGroupClearBits(gl_IcapDevObj.oEventGroupHandler, EVENT_ALL);
            if(restoreflag == 1)                                //�����־λ������stm32
            {
                restoreflag = 0;
                restoretime = atoi((const char*)gl_RestoreCmdObj.RestoreParam.ParamValue);
               
                while(restoretime--)
                {
                    printf("$$$$$$@@@@@@@@@��λ��ʱʱ��&&&&&&&&&######��%d...\r\n", restoretime);
                    vTaskDelay(1000);
                }
                DEBUG("��λ��ʱ����!!!\r\n");
                // CmdDealResult(gl_RestoreCmdObj.RestoreHead.CMDSerialnum, CMD_Restore, gl_RestoreCmdObj.RestoreHead.ControllerID, 0, (u8*)"0000");//�ָ��������óɹ�
                DEBUG("*********************\r\n");
                SoftReset();
            }
        }
        IWDG_Feed(); 
        vTaskDelay(500);
    }  
}


