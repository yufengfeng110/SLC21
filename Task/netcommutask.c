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
    if(iRet)                                                            //�жϵ�¼�Ƿ�ɹ�
    {
        gl_CMDtaskObj.firsttime = RTC_GetCounter();                     //��¼��¼�ɹ�ʱ��
        gl_CMDtaskObj.recvdatatime = gl_CMDtaskObj.firsttime;           //���ͬ��ʱ��
        printf("��½�ɹ�����\r\n");
    }
    else
    {
        gl_CMDtaskObj.firsttime = 0;                                    //��¼��¼�ɹ�ʱ��
        gl_CMDtaskObj.recvdatatime = 0;  
        printf("��½ʧ��!!\r\n");
        DEBUG("�ر�SOCKET!!\r\n");
        EC600M_CloseSocket(NET_SOCKET);
        EC600M_OpenSocket(NET_SOCKET, "UDP", gl_LampConfigObj.ServerIP, gl_LampConfigObj.ServerPort);
        goto SIGNIN;
    }
    /****************��������*********************/
    secondcount = RTC_GetCounter();
    timetemp = (secondcount/60 + 1)*60;            //��ǰʱ��+1���� ���ó�����ʱ��
    RTC_Calculate(timetemp);	
    AlarmRTCSet(timetemp);
    RTC_ALRIntEnable();
    RTC_Get();//����ʱ�� 
    printf("��ǰʱ��:%d-%d-%d %d:%d:%d\r\n",calendar.w_year,calendar.w_month,calendar.w_date,calendar.hour,calendar.min,calendar.sec);
    /**************************************************************/
    while(1)
    {
        printf("NetComm_task running!\r\n");
        // DEBUG("NetComm_task gl_IcapDevObj.heartcountflag:%d\r\n", gl_IcapDevObj.heartcountflag);
        gl_IcapDevObj.heartcountflag = NETTASK_NODATA;
        err = xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,0);
        if(err == pdTRUE)
        {
            printf("NetComm_task recv_��ȡmutex!\r\n");
            pchBuf = EC600M_Receive(1);                                     //�ȴ�1s��������
            if(pchBuf != NULL)              //�յ���Ϣ
            {  
                // DEBUG("_______________NetComm_task recv Something!!!\r\n");
                if(gl_IcapDevObj.heartcountflag == NETTASK_RECVDATA)        //���������յ��ͻ�ƽ̨����Ϣ�Ÿ���ʱ��
                {
                    gl_CMDtaskObj.recvdatatime = RTC_GetCounter();          //��¼������ʱ��                    
                }
                UnPackCmdParam((char*)pchBuf);
            }
            xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
            printf("NetComm_task recv_�ͷ�mutex!\r\n");
            NotifyValue = ulTaskNotifyTake(pdTRUE, 0);	                    //�������½�������
            if(NotifyValue == 1)
            {
                EC600M_CloseSocket(NET_SOCKET);
                DEBUG("4G��������......\r\n");
                EC600M_OpenSocket(NET_SOCKET, "UDP", gl_LampConfigObj.ServerIP, gl_LampConfigObj.ServerPort);
                goto SIGNIN;
            }
        }
        if(gl_IcapDevObj.heartcountflag == NETTASK_NODATA)
        {
            // DEBUG("_____++++++++______NetComm_task NO recv !!!\r\n");
            gl_CMDtaskObj.firsttime = RTC_GetCounter(); 
            if((gl_CMDtaskObj.firsttime-gl_CMDtaskObj.recvdatatime) >= HEARTOVERTIME)                        //�жϿ���ʱ���Ƿ񳬹�������ʱʱ��
            {
                // DEBUG("��·����ʱ�䣺%d\r\n", gl_CMDtaskObj.firsttime - gl_CMDtaskObj.recvdatatime);
                Heart_Cmd();                                            //��������
                gl_CMDtaskObj.heart_cnt++; 
                DEBUG("���͵�%d����������\r\n", gl_CMDtaskObj.heart_cnt);
                if(gl_CMDtaskObj.heart_cnt >= 4)                        //��������3������û�з�Ӧ����������
                {
                    EC600M_CloseSocket(NET_SOCKET);
                    DEBUG("�Ͽ����ӣ�����\r\n");
                    EC600M_OpenSocket(NET_SOCKET, "UDP", gl_LampConfigObj.ServerIP, gl_LampConfigObj.ServerPort);
                    goto SIGNIN;
                }														
            }
        }
        IWDG_Feed(); 
        // vTaskDelay(1000);
    }
}

