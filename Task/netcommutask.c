#include "netcommutask.h"
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "ec600m.h"
#include "cmddeal.h"
#include "DefBean.h"
#include "rtc.h"
#include "wdg.h"


CMDtaskObj gl_CMDtaskObj;

void NetComm_task(void *pvParameters)
{
    BaseType_t err=pdFALSE;
    u8 iRet = 0;
    u8* pchBuf = NULL;
    u32 NotifyValue;
    u32 secondcount = 0;
	u32 timetemp = 0;
    int itemp = 0;

    vTaskDelay(5000);
    INIT:
    printf("4G start!!!\r\n");
	EC600M_Init();
    itemp = EC600M_OpenSocket(NET_SOCKET, "UDP", gl_LampConfigObj.ServerIP, gl_LampConfigObj.ServerPort);  //"39.98.57.97"
    if(itemp < 0)
    {
        reset();
        vTaskDelay(5000);
        goto INIT;
    }      
    SIGNIN:
	iRet = Signin_Cmd(3, 5);
    if(iRet)                                                            //判断登录是否成功
    {
        gl_CMDtaskObj.firsttime = RTC_GetCounter();                     //记录登录成功时间
        gl_CMDtaskObj.recvdatatime = gl_CMDtaskObj.firsttime;           //最初同步时间
        printf("登陆成功！！\r\n");
    }
    else
    {
        gl_CMDtaskObj.firsttime = 0;                                    //记录登录成功时间
        gl_CMDtaskObj.recvdatatime = 0;  
        printf("登陆失败!!\r\n");
        DEBUG("关闭SOCKET!!\r\n");
        EC600M_CloseSocket(NET_SOCKET);
        EC600M_OpenSocket(NET_SOCKET, "UDP", gl_LampConfigObj.ServerIP, gl_LampConfigObj.ServerPort);
        goto SIGNIN;
    }
    /****************设置闹钟*********************/
    secondcount = RTC_GetCounter();
    timetemp = (secondcount/60 + 1)*60;            //当前时间+1分钟 设置成闹钟时间
    RTC_Calculate(timetemp);	
    AlarmRTCSet(timetemp);
    RTC_ALRIntEnable();
    RTC_Get();//更新时间 
    printf("当前时间:%d-%d-%d %d:%d:%d\r\n",calendar.w_year,calendar.w_month,calendar.w_date,calendar.hour,calendar.min,calendar.sec);
    /**************************************************************/
    while(1)
    {
        printf("NetComm_task running!\r\n");
        // DEBUG("NetComm_task gl_IcapDevObj.heartcountflag:%d\r\n", gl_IcapDevObj.heartcountflag);
        gl_IcapDevObj.heartcountflag = NETTASK_NODATA;
        err = xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,0);
        if(err == pdTRUE)
        {
            printf("NetComm_task recv_获取mutex!\r\n");
            pchBuf = EC600M_Receive(1);                                     //等待1s接收数据
            if(pchBuf != NULL)              //收到消息
            {  
                // DEBUG("_______________NetComm_task recv Something!!!\r\n");
                if(gl_IcapDevObj.heartcountflag == NETTASK_RECVDATA)        //网络任务收到客户平台的消息才更新时间
                {
                    gl_CMDtaskObj.recvdatatime = RTC_GetCounter();          //记录收数据时间                    
                }
                UnPackCmdParam((char*)pchBuf);
            }
            xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
            printf("NetComm_task recv_释放mutex!\r\n");
            NotifyValue = ulTaskNotifyTake(pdTRUE, 0);	                    //网络重新进行连接
            if(NotifyValue == 1)
            {
                EC600M_CloseSocket(NET_SOCKET);
                DEBUG("4G重新连接......\r\n");
                EC600M_OpenSocket(NET_SOCKET, "UDP", gl_LampConfigObj.ServerIP, gl_LampConfigObj.ServerPort);
                goto SIGNIN;
            }
        }
        if(gl_IcapDevObj.heartcountflag == NETTASK_NODATA)
        {
            // DEBUG("_____++++++++______NetComm_task NO recv !!!\r\n");
            gl_CMDtaskObj.firsttime = RTC_GetCounter(); 
            if((gl_CMDtaskObj.firsttime-gl_CMDtaskObj.recvdatatime) >= HEARTOVERTIME)                        //判断空闲时间是否超过心跳超时时间
            {
                // DEBUG("线路空闲时间：%d\r\n", gl_CMDtaskObj.firsttime - gl_CMDtaskObj.recvdatatime);
                Heart_Cmd();                                            //发送心跳
                gl_CMDtaskObj.heart_cnt++; 
                DEBUG("发送第%d次心跳！！\r\n", gl_CMDtaskObj.heart_cnt);
                if(gl_CMDtaskObj.heart_cnt >= 4)                        //连续发送3次心跳没有反应后重启连接
                {
                    EC600M_CloseSocket(NET_SOCKET);
                    DEBUG("断开连接！！！\r\n");
                    EC600M_OpenSocket(NET_SOCKET, "UDP", gl_LampConfigObj.ServerIP, gl_LampConfigObj.ServerPort);
                    goto SIGNIN;
                }														
            }
        }
        IWDG_Feed(); 
        // vTaskDelay(1000);
    }
}

