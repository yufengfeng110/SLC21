#include <string.h>
#include "sys.h"
#include "usart.h"	  
#include "dma.h"
#include "relay.h"
#include "ec600m.h"
#include "DefBean.h"

#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"				//os 使用
#include "semphr.h"	
#endif



#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	// while((USART3->SR&0X40)==0);//循环发送,直到发送完毕   
	// USART3->DR = (u8)ch;  
	while(USART_GetFlagStatus(USART3, USART_FLAG_TC)==RESET); 
	USART_SendData(USART3,ch);  
	return ch;
}
#endif 

/*使用microLib的方法*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART3, (uint8_t) ch);

	while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/


//串口发送函数
//解决丢失第一个字节问题
void USART_SendByte(USART_TypeDef* USARTx, uint16_t Data)
{
	while(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==RESET);
	USART_SendData(USARTx,Data);
}
/*************************************************UART1相关函数***************************************************************/ 
#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
  
void uart_init(u32 bound)
{
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	

	//USART1_TX   GPIOA.9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//USART1_RX	  GPIOA.10初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//PA.10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure); 

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=UART1_IRQPRIORITY;//抢占优先级5
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			
	NVIC_Init(&NVIC_InitStructure);	

	//USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

  USART_Init(USART1, &USART_InitStructure); 				//初始化串口1
  USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);		//使能串口空闲中断
	USART_DMACmd(USART1,USART_DMAReq_Rx|USART_DMAReq_Tx,ENABLE); //使能串口1的DMA发送、接收 
  USART_Cmd(USART1, ENABLE);                    		//使能串口1 

	UartRxDMA1_Config(DMA1_Channel5,(u32)&USART1->DR,(u32)gl_Ec600MObj.g_ec600m_RXBuf,EC600M_RX_BUF);  //串口1接收DMA配置
	DMA_Enable(DMA1_Channel5, EC600M_RX_BUF);
	UartTxDMA1_Config(DMA1_Channel4,(u32)&USART1->DR,(u32)gl_Ec600MObj.g_ec600m_TXBuf,EC600M_BUF_NUM);  //串口1发送DMA配置

	DEBUG("uart_init完成！\r\n");
}

void USART1_IRQHandler(void)                	//串口1中断服务程序
{
	u8 iTemp;
	BaseType_t xHigherPriorityTaskWoken;

	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)  //空闲中断+DMA进行接收
	{
		iTemp = USART1->SR;  	//先读SR，然后读DR才能清除
    	iTemp = USART1->DR;  	// 清除DR
		iTemp = iTemp;       	// 防止编译器警告

		DMA_Cmd(DMA1_Channel5, DISABLE);
		gl_Ec600MObj.g_ec600m_RXCnt = EC600M_RX_BUF - DMA_GetCurrDataCounter(DMA1_Channel5); 
		DMA_Enable(DMA1_Channel5, EC600M_RX_BUF);
		if((gl_Ec600MObj.EC600M_RxData_Queue != NULL))//接收到数据，并且二值信号量有效
		{
			xQueueSendFromISR(gl_Ec600MObj.EC600M_RxData_Queue, gl_Ec600MObj.g_ec600m_RXBuf, &xHigherPriorityTaskWoken);	//释放二值信号量
			memset(gl_Ec600MObj.g_ec600m_RXBuf,0,EC600M_RX_BUF);
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);//如果需要的话进行一次任务切换
		}
	}
} 
#endif	

/**************************************************UART2相关函数***************************************************/
#if EN_USART2_RX   //如果使能了接收

 	
u8 USART2_RX_BUF[USART2_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART2_RX_NUM=0;       //接收计数	  
  
void uart2_init(u32 bound)
{
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	

	//USART2_TX   GPIOA.2初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//USART2_RX	  GPIOA.3初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//PA.3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure); 

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=UART2_IRQPRIORITY ;//抢占优先级6
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			
	NVIC_Init(&NVIC_InitStructure);	

	//USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_9b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_Even;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;	//收发模式

  USART_Init(USART2, &USART_InitStructure); 				//初始化串口2
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);		//开启串口接受中断
  USART_Cmd(USART2, ENABLE);                    		//使能串口2

}


//串口2中断服务函数
//抢占优先级3 子优先级0
//处理接收中断
void USART2_IRQHandler(void)                	//串口1中断服务程序
{
	u16 Res;

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //接收中断
	{
		Res = USART_ReceiveData(USART2);	//读取接收到的数据
		USART2_RX_BUF[USART2_RX_NUM++] = Res;
	} 
} 
#endif	


/**************************************************UART3相关函数***************************************************/

#if EN_USART3_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
// u8 USART3_RX_BUF[USART3_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
// u16 USART3_RX_NUM=0;      //接收状态标记	 
// u16 USART3_RX_LEN=0;			//串口3接收数据长度
// u8 USART3_RX_Flag=0;			//串口3接收标志  
void uart3_init(u32 bound)
{
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//使能USART3，GPIOB时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	//USART3_TX   GPIOB.10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PB.10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOB.10

	//USART3_RX	  GPIOB.11初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;//PB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOB.11  

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=UART3_IRQPRIORITY ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

	//USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Tx;	//发送模式

  USART_Init(USART3, &USART_InitStructure); //初始化串口3
//   USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);//开启串口总线空闲中断
// 	USART_DMACmd(USART3,USART_DMAReq_Rx,ENABLE); //使能串口3的DMA接收 
  USART_Cmd(USART3, ENABLE);                    //使能串口3

	// UartRxDMA1_Config(DMA1_Channel3,(u32)&USART3->DR,(u32)USART3_RX_BUF,USART3_REC_LEN);
	// DMA_Enable(DMA1_Channel3, USART3_REC_LEN);
}

// void USART3_IRQHandler(void)                	//串口1中断服务程序
// {
// 	// u8 Res;
// 	u8  iTemp;

// 	if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)  //空闲中断+DMA进行接收
// 	{
// 		iTemp = USART3->SR;  	//先读SR，然后读DR才能清除
//     iTemp = USART3->DR;  	// 清除DR
// 		iTemp = iTemp;       	// 防止编译器警告

// 		DMA_Cmd(DMA1_Channel3, DISABLE);
// 		USART3_RX_LEN = USART3_REC_LEN - DMA_GetCurrDataCounter(DMA1_Channel3); 
// 		USART3_RX_Flag = 1;
// 		DMA_Enable(DMA1_Channel3, USART3_REC_LEN);
// 		// Res =USART_ReceiveData(USART3);	//读取接收到的数据		
// 		// USART3_RX_BUF[USART3_RX_NUM] = Res ;
// 		// USART3_RX_NUM++;
// 		// if(USART3_RX_NUM>=2)
// 		// {
// 		// 	USART3_RX_NUM = 0;
// 		// 	USART3_RX_Flag = 1;
// 		// } 	 
//   } 
// }

#endif	

