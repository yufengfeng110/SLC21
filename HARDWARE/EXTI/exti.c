#include "exti.h"
#include "DefBean.h"
#include "led.h"  
#include "DefBean.h" 
 

//�ⲿ�ж�0�������
//��ʼ��PA0,PA13,PA15Ϊ�ж�����.
void EXTIX_Init(void)
{
 
 	EXTI_InitTypeDef EXTI_InitStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//ʹ��PORTA,PORTEʱ��

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_12;//KEY0-KEY2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //���ó���������
 	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOE2,3,4

  //GPIOA.0	  �ж����Լ��жϳ�ʼ������ �����ش���
  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
 	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource12);

 	EXTI_InitStructure.EXTI_Line = EXTI_Line12;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);		//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���

  	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;			//ʹ�ܰ������ڵ��ⲿ�ж�ͨ��
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EXTI12_IRQPRIORITY;	//��ռ���ȼ�2�� 
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure); 
}

//�ⲿ�ж�12������� 
void EXTI15_10_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line12)!=RESET)//�ж�ĳ�����ϵ��ж��Ƿ��� 
	{
		g_oADCSearchObj.chStatus |= (0x01 << 2);
		printf("********waibuzhongduan*********chStatus:%d\r\n", g_oADCSearchObj.chStatus);
		LED4 = 0;
		EXTI_ClearITPendingBit(EXTI_Line12); //��� LINE �ϵ��жϱ�־λ 
	}
}



