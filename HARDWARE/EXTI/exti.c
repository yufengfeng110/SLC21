#include "exti.h"
#include "DefBean.h"
#include "led.h"  
#include "DefBean.h" 
 

//外部中断0服务程序
//初始化PA0,PA13,PA15为中断输入.
void EXTIX_Init(void)
{
 
 	EXTI_InitTypeDef EXTI_InitStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//使能PORTA,PORTE时钟

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_12;//KEY0-KEY2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOE2,3,4

  //GPIOA.0	  中断线以及中断初始化配置 上升沿触发
  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
 	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource12);

 	EXTI_InitStructure.EXTI_Line = EXTI_Line12;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);		//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

  	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;			//使能按键所在的外部中断通道
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EXTI12_IRQPRIORITY;	//抢占优先级2， 
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//使能外部中断通道
  	NVIC_Init(&NVIC_InitStructure); 
}

//外部中断12服务程序 
void EXTI15_10_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line12)!=RESET)//判断某个线上的中断是否发生 
	{
		g_oADCSearchObj.chStatus |= (0x01 << 2);
		printf("********waibuzhongduan*********chStatus:%d\r\n", g_oADCSearchObj.chStatus);
		LED4 = 0;
		EXTI_ClearITPendingBit(EXTI_Line12); //清除 LINE 上的中断标志位 
	}
}



