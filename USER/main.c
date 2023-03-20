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
/**************开始任务****************/
//任务优先级
#define START_TASK_PRIO 1
//任务堆栈大小
#define START_TASK_SIZE 100
//任务句柄
TaskHandle_t StartTask_Handler;
//任务函数
void start_task(void *pvParameters);
/**************指示灯任务****************/
//任务优先级
#define LED_TASK_PRIO 2
//任务堆栈大小
#define LED_TASK_SIZE 100
//任务句柄
TaskHandle_t LEDTask_Handler;
//任务函数
void led_task(void *pvParameters);
/**************存储任务****************/
//任务优先级
#define STORAGE_TASK_PRIO 2
//任务堆栈大小
#define STORAGE_TASK_SIZE 100
//任务句柄
TaskHandle_t StorageTask_Handler;
//任务函数
void Storage_task(void *pvParameters);
/**************电参数任务****************/
//任务优先级
#define Electricparam_TASK_PRIO 3
//任务堆栈大小
#define Electricparam_TASK_SIZE 200
//任务句柄
TaskHandle_t ElectricparamTask_Handler;
//任务函数
void Electricparam_task(void *pvParameters);
/**************网络通信任务****************/
//任务优先级
#define NetComm_TASK_PRIO 4
//任务堆栈大小
#define NetComm_TASK_SIZE 300
//任务句柄
TaskHandle_t NetCommTask_Handler;
//任务函数
void NetComm_task(void *pvParameters);
/**************后台任务****************/
//任务优先级
#define BackGround_TASK_PRIO 5
//任务堆栈大小
#define BackGround_TASK_SIZE 200
//任务句柄
TaskHandle_t BackGround_Handler;
//任务函数
void BackGround_task(void *pvParameters);
/**************远程升级任务****************/
//任务优先级
#define OTAupgrade_TASK_PRIO 6
//任务堆栈大小
#define OTAupgrade_TASK_SIZE 200
//任务句柄
TaskHandle_t OTAupgrade_Handler;
//任务函数
void OTAupgrade_task(void *pvParameters);
/**************存储策略任务****************/
//任务优先级
#define Savestrategy_TASK_PRIO 7
//任务堆栈大小
#define Savestrategy_TASK_SIZE 100
//任务句柄
TaskHandle_t Savestrategy_Handler;
//任务函数
void Savestrategy_task(void *pvParameters);
                                     

IcapDevObj			gl_IcapDevObj;
void GetLockCode(void);         //获取stm32序列号


int main(void)
{	
  SCB->VTOR = FLASH_BASE | 0x2800; 
  delay_init();
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);     //设置系统中断优先级分组4
  uart2_init(9600);
  uart3_init(115200);
  AT24CXX_Init();
  EXTIX_Init();
  LED_Init();
  RelayControl_Init();
  RTC_init();
  uart_init(115200);
  IWDG_Init(4,625);    //与分频数为64,重载值为625,溢出时间为1s
  T_Adc_Init();
//  W25QXX_Init();
  AT24CXX_ServiceFunc();
//  W25Q16_Servicefunc();
  TIM3_PWM_Init(59999,0);	                            //不分频。PWM频率=72000000/36000=2Khz
  HLW8112_Init();
  EC600M_RUN();
	GetLockCode();
	printf("stm32Id:%s\r\n", gl_IcapDevObj.DevSerialnum);
  LampControl(LAMP_1, LAMP_OPEN);													//默认开灯
  PWM_Servicefunc(LAMP_1, gl_LampConfigObj.Lamp1_dimval);
  LED_LAMP1 = LED_ON;
	LampControl(LAMP_2, LAMP_OPEN);
  PWM_Servicefunc(LAMP_2, gl_LampConfigObj.Lamp2_dimval);
  LED_LAMP2 = LED_ON;

  //创建开始任务
  xTaskCreate((TaskFunction_t )start_task,            //任务函数
              (const char*    )"start_task",          //任务名称
              (uint16_t       )START_TASK_SIZE,        //任务堆栈大小
              (void*          )NULL,                  //传递给任务函数的参数
              (UBaseType_t    )START_TASK_PRIO,       //任务优先级
              (TaskHandle_t*  )&StartTask_Handler);   //任务句柄  
  IWDG_Enable();  //使能IWDG            
  vTaskStartScheduler();          //开启任务调度
}

//开始任务任务函数
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //进入临界区

    gl_IcapDevObj.BGtaskSemaphore = xSemaphoreCreateBinary();
    gl_IcapDevObj.MutexSemaphore = xSemaphoreCreateMutex();
    gl_IcapDevObj.oEventGroupHandler = xEventGroupCreate();              //创建写EEPROM事件标志组
    gl_IcapDevObj.ohlw8112_EventHandler = xEventGroupCreate();              //创建电能计算事件标志组
    gl_Ec600MObj.EC600M_RxData_Queue = xQueueCreate(EC600M_Q_NUM,EC600M_RX_BUF); //创建队列,队列项长度是串口接收缓冲区长度

    //创建LED任务
    xTaskCreate((TaskFunction_t )led_task,     	
                (const char*    )"led_task",   	
                (uint16_t       )LED_TASK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )LED_TASK_PRIO,	
                (TaskHandle_t*  )&LEDTask_Handler); 
    //创建存储任务
    xTaskCreate((TaskFunction_t )Storage_task,     	
                (const char*    )"Storage_task",   	
                (uint16_t       )STORAGE_TASK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )STORAGE_TASK_PRIO,	
                (TaskHandle_t*  )&StorageTask_Handler);   
    //创建电能采集任务
    xTaskCreate((TaskFunction_t )Electricparam_task,     
                (const char*    )"Electricparam_task",   
                (uint16_t       )Electricparam_TASK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )Electricparam_TASK_PRIO,
                (TaskHandle_t*  )&ElectricparamTask_Handler);  
    //创建网络通信任务
    xTaskCreate((TaskFunction_t )NetComm_task,     
                (const char*    )"NetComm_task",   
                (uint16_t       )NetComm_TASK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )NetComm_TASK_PRIO,
                (TaskHandle_t*  )&NetCommTask_Handler);   
    //创建后台任务
    xTaskCreate((TaskFunction_t )BackGround_task,     
                (const char*    )"BackGround_task",   
                (uint16_t       )BackGround_TASK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )BackGround_TASK_PRIO,
                (TaskHandle_t*  )&BackGround_Handler);  
    //创建远程升级任务
    xTaskCreate((TaskFunction_t )OTAupgrade_task,     
                (const char*    )"OTAupgrade_task",   
                (uint16_t       )OTAupgrade_TASK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )OTAupgrade_TASK_PRIO,
                (TaskHandle_t*  )&OTAupgrade_Handler);   
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}

void led_task(void *pvParameters)
{
  while(1)
  {
		DEBUG("led_task running\r\n");
    // LED4 = !LED4;         //加密指示灯
		LED_MCU = !LED_MCU;         //MCU运行
    IWDG_Feed();          
    vTaskDelay(200);
  }
}


//读stm32序列号
void GetLockCode(void) 
{
    u32 stm32Id[3]={0};

    stm32Id[0]=*(vu32*)(0x1ffff7e8);
    stm32Id[1]=*(vu32*)(0x1ffff7ec);
    stm32Id[2]=*(vu32*)(0x1ffff7f0);
    // DEBUG("stm32长度：%d\r\n", sizeof(stm32Id));
    // DEBUG("以word读出的STM32ID:");
    // for(i=0; i<3; i++)
    // {
    //   DEBUG("%X|", ((u32*)stm32Id)[i]);
    // }
    // DEBUG("\r\n");
    // DEBUG("以Byte读出的STM32ID:");
    // for(i=0; i<12; i++)
    // {
    //   DEBUG("%X|", ((u8*)stm32Id)[i]);
    // }
    // DEBUG("\r\n");
    HexArrayToStr(gl_IcapDevObj.DevSerialnum, (unsigned char *)stm32Id, sizeof(stm32Id));
    // DEBUG("DevSerialnum长度：%d\r\n", sizeof(gl_IcapDevObj.DevSerialnum));
    // DEBUG("转换完的STM32ID:");
    // for(i=0; i<sizeof(gl_IcapDevObj.DevSerialnum); i++)
    // {
    //   DEBUG("%02X|", ((char*)gl_IcapDevObj.DevSerialnum)[i]);
    // }
    DEBUG("\r\n");
}

////////////////////////////////////////////////////////////////////////////
//FreeRTOS回调函数
///////////////////////////////////////////////////////////////////////////
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	DEBUG("任务 %s 堆栈溢出！！！\r\n", pcTaskName);
}

void vApplicationMallocFailedHook( void )
{
	DEBUG("内存分配Error:%s,%d\r\n",__FILE__,__LINE__);
}	

