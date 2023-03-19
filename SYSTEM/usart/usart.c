#include <string.h>
#include "sys.h"
#include "usart.h"	  
#include "dma.h"
#include "relay.h"
#include "ec600m.h"
#include "DefBean.h"

#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"				//os ʹ��
#include "semphr.h"	
#endif



#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	// while((USART3->SR&0X40)==0);//ѭ������,ֱ���������   
	// USART3->DR = (u8)ch;  
	while(USART_GetFlagStatus(USART3, USART_FLAG_TC)==RESET); 
	USART_SendData(USART3,ch);  
	return ch;
}
#endif 

/*ʹ��microLib�ķ���*/
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


//���ڷ��ͺ���
//�����ʧ��һ���ֽ�����
void USART_SendByte(USART_TypeDef* USARTx, uint16_t Data)
{
	while(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==RESET);
	USART_SendData(USARTx,Data);
}
/*************************************************UART1��غ���***************************************************************/ 
#if EN_USART1_RX   //���ʹ���˽���
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
  
void uart_init(u32 bound)
{
	//GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	

	//USART1_TX   GPIOA.9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//USART1_RX	  GPIOA.10��ʼ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//PA.10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure); 

	//Usart1 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=UART1_IRQPRIORITY;//��ռ���ȼ�5
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			
	NVIC_Init(&NVIC_InitStructure);	

	//USART ��ʼ������

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

  USART_Init(USART1, &USART_InitStructure); 				//��ʼ������1
  USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);		//ʹ�ܴ��ڿ����ж�
	USART_DMACmd(USART1,USART_DMAReq_Rx|USART_DMAReq_Tx,ENABLE); //ʹ�ܴ���1��DMA���͡����� 
  USART_Cmd(USART1, ENABLE);                    		//ʹ�ܴ���1 

	UartRxDMA1_Config(DMA1_Channel5,(u32)&USART1->DR,(u32)gl_Ec600MObj.g_ec600m_RXBuf,EC600M_RX_BUF);  //����1����DMA����
	DMA_Enable(DMA1_Channel5, EC600M_RX_BUF);
	UartTxDMA1_Config(DMA1_Channel4,(u32)&USART1->DR,(u32)gl_Ec600MObj.g_ec600m_TXBuf,EC600M_BUF_NUM);  //����1����DMA����

	DEBUG("uart_init��ɣ�\r\n");
}

void USART1_IRQHandler(void)                	//����1�жϷ������
{
	u8 iTemp;
	BaseType_t xHigherPriorityTaskWoken;

	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)  //�����ж�+DMA���н���
	{
		iTemp = USART1->SR;  	//�ȶ�SR��Ȼ���DR�������
    	iTemp = USART1->DR;  	// ���DR
		iTemp = iTemp;       	// ��ֹ����������

		DMA_Cmd(DMA1_Channel5, DISABLE);
		gl_Ec600MObj.g_ec600m_RXCnt = EC600M_RX_BUF - DMA_GetCurrDataCounter(DMA1_Channel5); 
		DMA_Enable(DMA1_Channel5, EC600M_RX_BUF);
		if((gl_Ec600MObj.EC600M_RxData_Queue != NULL))//���յ����ݣ����Ҷ�ֵ�ź�����Ч
		{
			xQueueSendFromISR(gl_Ec600MObj.EC600M_RxData_Queue, gl_Ec600MObj.g_ec600m_RXBuf, &xHigherPriorityTaskWoken);	//�ͷŶ�ֵ�ź���
			memset(gl_Ec600MObj.g_ec600m_RXBuf,0,EC600M_RX_BUF);
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);//�����Ҫ�Ļ�����һ�������л�
		}
	}
} 
#endif	

/**************************************************UART2��غ���***************************************************/
#if EN_USART2_RX   //���ʹ���˽���

 	
u8 USART2_RX_BUF[USART2_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART2_RX_NUM=0;       //���ռ���	  
  
void uart2_init(u32 bound)
{
	//GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	

	//USART2_TX   GPIOA.2��ʼ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//USART2_RX	  GPIOA.3��ʼ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//PA.3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure); 

	//Usart1 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=UART2_IRQPRIORITY ;//��ռ���ȼ�6
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			
	NVIC_Init(&NVIC_InitStructure);	

	//USART ��ʼ������

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_9b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_Even;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;	//�շ�ģʽ

  USART_Init(USART2, &USART_InitStructure); 				//��ʼ������2
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);		//�������ڽ����ж�
  USART_Cmd(USART2, ENABLE);                    		//ʹ�ܴ���2

}


//����2�жϷ�����
//��ռ���ȼ�3 �����ȼ�0
//��������ж�
void USART2_IRQHandler(void)                	//����1�жϷ������
{
	u16 Res;

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //�����ж�
	{
		Res = USART_ReceiveData(USART2);	//��ȡ���յ�������
		USART2_RX_BUF[USART2_RX_NUM++] = Res;
	} 
} 
#endif	


/**************************************************UART3��غ���***************************************************/

#if EN_USART3_RX   //���ʹ���˽���
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
// u8 USART3_RX_BUF[USART3_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
// u16 USART3_RX_NUM=0;      //����״̬���	 
// u16 USART3_RX_LEN=0;			//����3�������ݳ���
// u8 USART3_RX_Flag=0;			//����3���ձ�־  
void uart3_init(u32 bound)
{
	//GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//ʹ��USART3��GPIOBʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	//USART3_TX   GPIOB.10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PB.10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIOB.10

	//USART3_RX	  GPIOB.11��ʼ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;//PB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIOB.11  

	//Usart1 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=UART3_IRQPRIORITY ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���

	//USART ��ʼ������

	USART_InitStructure.USART_BaudRate = bound;//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Tx;	//����ģʽ

  USART_Init(USART3, &USART_InitStructure); //��ʼ������3
//   USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);//�����������߿����ж�
// 	USART_DMACmd(USART3,USART_DMAReq_Rx,ENABLE); //ʹ�ܴ���3��DMA���� 
  USART_Cmd(USART3, ENABLE);                    //ʹ�ܴ���3

	// UartRxDMA1_Config(DMA1_Channel3,(u32)&USART3->DR,(u32)USART3_RX_BUF,USART3_REC_LEN);
	// DMA_Enable(DMA1_Channel3, USART3_REC_LEN);
}

// void USART3_IRQHandler(void)                	//����1�жϷ������
// {
// 	// u8 Res;
// 	u8  iTemp;

// 	if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)  //�����ж�+DMA���н���
// 	{
// 		iTemp = USART3->SR;  	//�ȶ�SR��Ȼ���DR�������
//     iTemp = USART3->DR;  	// ���DR
// 		iTemp = iTemp;       	// ��ֹ����������

// 		DMA_Cmd(DMA1_Channel3, DISABLE);
// 		USART3_RX_LEN = USART3_REC_LEN - DMA_GetCurrDataCounter(DMA1_Channel3); 
// 		USART3_RX_Flag = 1;
// 		DMA_Enable(DMA1_Channel3, USART3_REC_LEN);
// 		// Res =USART_ReceiveData(USART3);	//��ȡ���յ�������		
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

