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

//stm32重启
void SoftReset(void)
{
	printf("开始复位！！！\r\n");
	__set_FAULTMASK(1);       // 关闭所有中端
	NVIC_SystemReset();       // 复位
}

void Storage_task(void *pvParameters)
{
    EventBits_t EventValue;
    u8 ec600reconflag = 0;           //4G重新连接标志位
    u8 restoreflag = 0;              //恢复出厂设置标志位
    u16 restoretime = 0;             //恢复出厂设置延时时间

    while(1)
    {
        printf("Storage_task running!\r\n");
        EventValue = xEventGroupGetBits(gl_IcapDevObj.oEventGroupHandler);
        if(EventValue != 0)
        {
            if((EventValue&0x01) != 0)
            {
                AT24CXX_Write(0, (u8*)gl_LampConfigObj.ControllerID, 10);
                DEBUG("ControllerID参数存储完毕！\r\n"); 
            }
            if((EventValue&0x02) != 0)
            {
                AT24CXX_Write(10, (u8*)gl_LampConfigObj.ServerIP, 30);
                DEBUG("ServerIP参数存储完毕！\r\n"); 
                ec600reconflag = 1;
            }
            if((EventValue&0x04) != 0)
            {
                AT24CXX_Write(40, (u8*)gl_LampConfigObj.ServerPort, 2);
                DEBUG("ServerPort参数存储完毕！\r\n"); 
                ec600reconflag = 1;
            }
            if((EventValue&0x08) != 0)
            {
                AT24CXX_Write(42, &(gl_LampConfigObj.RestoreFlag), 1);
                DEBUG("RestoreFlag参数存储完毕！\r\n"); 
                restoreflag = 1;
            }
            if((EventValue&0x10) != 0)
            {
                AT24CXX_Write(43, (u8*)(&(g_oADCSearchObj.oAC220_In.fdQ)), 4);
                AT24CXX_Write(47, (u8*)(&(g_oADCSearchObj.oAC220_Out[0].fdQ)), 4);
                AT24CXX_Write(51, (u8*)(&(g_oADCSearchObj.oAC220_Out[1].fdQ)), 4);
                DEBUG("电量参数存储完毕！\r\n"); 
            }
            if(ec600reconflag == 1)                             //IP or PORT任何一个被新设置都要重新连接网络
            {
                xTaskNotifyGive(NetCommTask_Handler);           //网络按照新设置的IP重新连接
                ec600reconflag = 0;
            }
            xEventGroupClearBits(gl_IcapDevObj.oEventGroupHandler, EVENT_ALL);
            if(restoreflag == 1)                                //存完标志位后重启stm32
            {
                restoreflag = 0;
                restoretime = atoi((const char*)gl_RestoreCmdObj.RestoreParam.ParamValue);
               
                while(restoretime--)
                {
                    printf("$$$$$$@@@@@@@@@复位延时时间&&&&&&&&&######：%d...\r\n", restoretime);
                    vTaskDelay(1000);
                }
                DEBUG("复位延时结束!!!\r\n");
                // CmdDealResult(gl_RestoreCmdObj.RestoreHead.CMDSerialnum, CMD_Restore, gl_RestoreCmdObj.RestoreHead.ControllerID, 0, (u8*)"0000");//恢复出厂设置成功
                DEBUG("*********************\r\n");
                SoftReset();
            }
        }
        IWDG_Feed(); 
        vTaskDelay(500);
    }  
}


