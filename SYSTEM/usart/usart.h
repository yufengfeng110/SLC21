#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 


#define EN_USART1_RX 			1				//使能（1）/禁止（0）串口1接收
#define USART2_REC_LEN  	20  	  //定义最大接收字节数 200
#define EN_USART2_RX 			1				//使能（1）/禁止（0）串口1接收
// #define USART3_REC_LEN  	200 		//定义最大接收字节数 200
#define EN_USART3_RX 			1				//使能（1）/禁止（0）串口1接收
	  	

extern u8   USART2_RX_BUF[USART2_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16  USART2_RX_NUM;         		//接收计数
// extern u8   USART3_RX_BUF[USART3_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
// extern u16  USART3_RX_LEN;         		//接收数据长度
// extern u8   USART3_RX_Flag;	            //接收完成标志位

void uart_init(u32 bound);
void uart2_init(u32 bound);
void uart3_init(u32 bound);
int GetKey(void);
void USART_SendByte(USART_TypeDef* USARTx, uint16_t Data);
#endif


