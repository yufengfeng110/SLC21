#include "led.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "DefBean.h"

void LED_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15|GPIO_Pin_14|GPIO_Pin_13|GPIO_Pin_12;				 				//LED0->PB.15 端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 											//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;											//IO口速度为50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);					 											//根据设定参数初始化GPIOB.15
	GPIO_SetBits(GPIOB,GPIO_Pin_15|GPIO_Pin_14|GPIO_Pin_13|GPIO_Pin_12);	//PB.15 输出高
	DEBUG("LED_Init完成！\r\n");
}
 



