#include "led.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "DefBean.h"

void LED_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15|GPIO_Pin_14|GPIO_Pin_13|GPIO_Pin_12;				 				//LED0->PB.15 �˿�����
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 											//�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;											//IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);					 											//�����趨������ʼ��GPIOB.15
	GPIO_SetBits(GPIOB,GPIO_Pin_15|GPIO_Pin_14|GPIO_Pin_13|GPIO_Pin_12);	//PB.15 �����
	DEBUG("LED_Init��ɣ�\r\n");
}
 



