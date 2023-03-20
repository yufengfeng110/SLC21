#include <string.h>
#include "stm32f10x.h"
#include "led.h"
#include "timer.h"
#include "rtc.h"
#include "usart.h"
#include "delay.h"
#include "tsensor.h"
#include "24cxx.h"
//#include "w25qxx.h"
#include "hlw8112.h"
#include "relay.h"
#include "ec600m.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"	
#include "DefBean.h"
#include "wdg.h"
#include "exti.h"
/**************��ʼ����****************/
//�������ȼ�
#define START_TASK_PRIO 1
//�����ջ��С
#define START_TASK_SIZE 100
//������
TaskHandle_t StartTask_Handler;
//������
void start_task(void *pvParameters);
/**************ָʾ������****************/
//�������ȼ�
#define LED_TASK_PRIO 2
//�����ջ��С
#define LED_TASK_SIZE 100
//������
TaskHandle_t LEDTask_Handler;
//������
void led_task(void *pvParameters);
/**************�洢����****************/
//�������ȼ�
#define STORAGE_TASK_PRIO 2
//�����ջ��С
#define STORAGE_TASK_SIZE 100
//������
TaskHandle_t StorageTask_Handler;
//������
void Storage_task(void *pvParameters);
/**************���������****************/
//�������ȼ�
#define Electricparam_TASK_PRIO 3
//�����ջ��С
#define Electricparam_TASK_SIZE 200
//������
TaskHandle_t ElectricparamTask_Handler;
//������
void Electricparam_task(void *pvParameters);
/**************����ͨ������****************/
//�������ȼ�
#define NetComm_TASK_PRIO 4
//�����ջ��С
#define NetComm_TASK_SIZE 300
//������
TaskHandle_t NetCommTask_Handler;
//������
void NetComm_task(void *pvParameters);
/**************��̨����****************/
//�������ȼ�
#define BackGround_TASK_PRIO 5
//�����ջ��С
#define BackGround_TASK_SIZE 200
//������
TaskHandle_t BackGround_Handler;
//������
void BackGround_task(void *pvParameters);
/**************Զ����������****************/
//�������ȼ�
#define OTAupgrade_TASK_PRIO 6
//�����ջ��С
#define OTAupgrade_TASK_SIZE 200
//������
TaskHandle_t OTAupgrade_Handler;
//������
void OTAupgrade_task(void *pvParameters);
/**************�洢��������****************/
//�������ȼ�
#define Savestrategy_TASK_PRIO 7
//�����ջ��С
#define Savestrategy_TASK_SIZE 100
//������
TaskHandle_t Savestrategy_Handler;
//������
void Savestrategy_task(void *pvParameters);
                                     

IcapDevObj			gl_IcapDevObj;
void GetLockCode(void);         //��ȡstm32���к�


int main(void)
{	
  SCB->VTOR = FLASH_BASE | 0x2800; 
  delay_init();
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);     //����ϵͳ�ж����ȼ�����4
  uart2_init(9600);
  uart3_init(115200);
  AT24CXX_Init();
  EXTIX_Init();
  LED_Init();
  RelayControl_Init();
  RTC_init();
  uart_init(115200);
  IWDG_Init(4,625);    //���Ƶ��Ϊ64,����ֵΪ625,���ʱ��Ϊ1s
  T_Adc_Init();
//  W25QXX_Init();
  AT24CXX_ServiceFunc();
//  W25Q16_Servicefunc();
  TIM3_PWM_Init(59999,0);	                            //����Ƶ��PWMƵ��=72000000/36000=2Khz
  HLW8112_Init();
  EC600M_RUN();
	GetLockCode();
	printf("stm32Id:%s\r\n", gl_IcapDevObj.DevSerialnum);
  LampControl(LAMP_1, LAMP_OPEN);													//Ĭ�Ͽ���
  PWM_Servicefunc(LAMP_1, gl_LampConfigObj.Lamp1_dimval);
  LED_LAMP1 = LED_ON;
	LampControl(LAMP_2, LAMP_OPEN);
  PWM_Servicefunc(LAMP_2, gl_LampConfigObj.Lamp2_dimval);
  LED_LAMP2 = LED_ON;

  //������ʼ����
  xTaskCreate((TaskFunction_t )start_task,            //������
              (const char*    )"start_task",          //��������
              (uint16_t       )START_TASK_SIZE,        //�����ջ��С
              (void*          )NULL,                  //���ݸ��������Ĳ���
              (UBaseType_t    )START_TASK_PRIO,       //�������ȼ�
              (TaskHandle_t*  )&StartTask_Handler);   //������  
  IWDG_Enable();  //ʹ��IWDG            
  vTaskStartScheduler();          //�����������
}

//��ʼ����������
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //�����ٽ���

    gl_IcapDevObj.BGtaskSemaphore = xSemaphoreCreateBinary();
    gl_IcapDevObj.MutexSemaphore = xSemaphoreCreateMutex();
    gl_IcapDevObj.oEventGroupHandler = xEventGroupCreate();              //����дEEPROM�¼���־��
    gl_IcapDevObj.ohlw8112_EventHandler = xEventGroupCreate();              //�������ܼ����¼���־��
    gl_Ec600MObj.EC600M_RxData_Queue = xQueueCreate(EC600M_Q_NUM,EC600M_RX_BUF); //��������,��������Ǵ��ڽ��ջ���������

    //����LED����
    xTaskCreate((TaskFunction_t )led_task,     	
                (const char*    )"led_task",   	
                (uint16_t       )LED_TASK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )LED_TASK_PRIO,	
                (TaskHandle_t*  )&LEDTask_Handler); 
    //�����洢����
    xTaskCreate((TaskFunction_t )Storage_task,     	
                (const char*    )"Storage_task",   	
                (uint16_t       )STORAGE_TASK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )STORAGE_TASK_PRIO,	
                (TaskHandle_t*  )&StorageTask_Handler);   
    //�������ܲɼ�����
    xTaskCreate((TaskFunction_t )Electricparam_task,     
                (const char*    )"Electricparam_task",   
                (uint16_t       )Electricparam_TASK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )Electricparam_TASK_PRIO,
                (TaskHandle_t*  )&ElectricparamTask_Handler);  
    //��������ͨ������
    xTaskCreate((TaskFunction_t )NetComm_task,     
                (const char*    )"NetComm_task",   
                (uint16_t       )NetComm_TASK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )NetComm_TASK_PRIO,
                (TaskHandle_t*  )&NetCommTask_Handler);   
    //������̨����
    xTaskCreate((TaskFunction_t )BackGround_task,     
                (const char*    )"BackGround_task",   
                (uint16_t       )BackGround_TASK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )BackGround_TASK_PRIO,
                (TaskHandle_t*  )&BackGround_Handler);  
    //����Զ����������
    xTaskCreate((TaskFunction_t )OTAupgrade_task,     
                (const char*    )"OTAupgrade_task",   
                (uint16_t       )OTAupgrade_TASK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )OTAupgrade_TASK_PRIO,
                (TaskHandle_t*  )&OTAupgrade_Handler);   
    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���
}

void led_task(void *pvParameters)
{
  while(1)
  {
		DEBUG("led_task running\r\n");
    // LED4 = !LED4;         //����ָʾ��
		LED_MCU = !LED_MCU;         //MCU����
    IWDG_Feed();          
    vTaskDelay(200);
  }
}


//��stm32���к�
void GetLockCode(void) 
{
    u32 stm32Id[3]={0};

    stm32Id[0]=*(vu32*)(0x1ffff7e8);
    stm32Id[1]=*(vu32*)(0x1ffff7ec);
    stm32Id[2]=*(vu32*)(0x1ffff7f0);
    // DEBUG("stm32���ȣ�%d\r\n", sizeof(stm32Id));
    // DEBUG("��word������STM32ID:");
    // for(i=0; i<3; i++)
    // {
    //   DEBUG("%X|", ((u32*)stm32Id)[i]);
    // }
    // DEBUG("\r\n");
    // DEBUG("��Byte������STM32ID:");
    // for(i=0; i<12; i++)
    // {
    //   DEBUG("%X|", ((u8*)stm32Id)[i]);
    // }
    // DEBUG("\r\n");
    HexArrayToStr(gl_IcapDevObj.DevSerialnum, (unsigned char *)stm32Id, sizeof(stm32Id));
    // DEBUG("DevSerialnum���ȣ�%d\r\n", sizeof(gl_IcapDevObj.DevSerialnum));
    // DEBUG("ת�����STM32ID:");
    // for(i=0; i<sizeof(gl_IcapDevObj.DevSerialnum); i++)
    // {
    //   DEBUG("%02X|", ((char*)gl_IcapDevObj.DevSerialnum)[i]);
    // }
    DEBUG("\r\n");
}

////////////////////////////////////////////////////////////////////////////
//FreeRTOS�ص�����
///////////////////////////////////////////////////////////////////////////
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	DEBUG("���� %s ��ջ���������\r\n", pcTaskName);
}

void vApplicationMallocFailedHook( void )
{
	DEBUG("�ڴ����Error:%s,%d\r\n",__FILE__,__LINE__);
}	

