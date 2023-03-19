#include "relay.h"
#include "usart.h"
#include "stdio.h"
#include "timer.h"
#include "dma.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ec600m.h"
#include "cmddeal.h"

void RelayControl_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);	 

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_11;				//PA.0/1/11 端口配置  PA11用于4G模块开关
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 								//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;								//IO口速度为50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);					 						//根据设定参数初始化GPIOA.0/1
	GPIO_ResetBits(GPIOA, GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_11);						//PA.0/1/11 输出低

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;							//PB.8/9端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 								//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;								//IO口速度为50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);					 						//根据设定参数初始化GPIOPB.8/9
	GPIO_ResetBits(GPIOB, GPIO_Pin_8|GPIO_Pin_9);									//PB.8/9 输出低	
	DEBUG("RelayControl_Init完成！\r\n");
}

//控制灯具开关
//lampid:灯具ID
//status:0 关 1 开
 void LampControl(u8 lampid, u8 status)
{
	if (lampid == 1)
	{
		switch (status)
		{
			case 0:
				GPIO_SetBits(GPIOB, GPIO_Pin_9);
				delay_ms(50);
				GPIO_ResetBits(GPIOB, GPIO_Pin_9);
				g_oADCSearchObj.chStatus &= ~(0x01 << 0);
				break;
			case 1:
				GPIO_SetBits(GPIOB, GPIO_Pin_8);
				delay_ms(50);
				GPIO_ResetBits(GPIOB, GPIO_Pin_8);
				g_oADCSearchObj.chStatus |= (0x01 << 0);		
				break;
		}
	}
	else if (lampid == 2)
	{
		switch (status)
		{
			case 0:
				GPIO_SetBits(GPIOA, GPIO_Pin_0);
				delay_ms(50);
				GPIO_ResetBits(GPIOA, GPIO_Pin_0);
				g_oADCSearchObj.chStatus &= ~(0x01 << 1);
				break;
			case 1:
				GPIO_SetBits(GPIOA, GPIO_Pin_1);
				delay_ms(50);
				GPIO_ResetBits(GPIOA, GPIO_Pin_1);
				g_oADCSearchObj.chStatus |= (0x01 << 1);
				break;
		}
	}
}




