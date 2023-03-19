#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 


#define EN_USART1_RX 			1				//ʹ�ܣ�1��/��ֹ��0������1����
#define USART2_REC_LEN  	20  	  //�����������ֽ��� 200
#define EN_USART2_RX 			1				//ʹ�ܣ�1��/��ֹ��0������1����
// #define USART3_REC_LEN  	200 		//�����������ֽ��� 200
#define EN_USART3_RX 			1				//ʹ�ܣ�1��/��ֹ��0������1����
	  	

extern u8   USART2_RX_BUF[USART2_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u16  USART2_RX_NUM;         		//���ռ���
// extern u8   USART3_RX_BUF[USART3_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
// extern u16  USART3_RX_LEN;         		//�������ݳ���
// extern u8   USART3_RX_Flag;	            //������ɱ�־λ

void uart_init(u32 bound);
void uart2_init(u32 bound);
void uart3_init(u32 bound);
int GetKey(void);
void USART_SendByte(USART_TypeDef* USARTx, uint16_t Data);
#endif


