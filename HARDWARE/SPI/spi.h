#ifndef __SPI_H
#define __SPI_H
#include "sys.h"

 
void SPI1_Init(void); 				  	    													  
void SPI2_Init(void);			 //��ʼ��SPI��
void SPI1_SetSpeed(u8 SpeedSet); //����SPI�ٶ�
void SPI2_SetSpeed(u8 SpeedSet); //����SPI�ٶ� 
u8 SPI1_ReadWriteByte(u8 TxData);//SPI���߶�дһ���ֽ�  
u8 SPI2_ReadWriteByte(u8 TxData);//SPI���߶�дһ���ֽ�
		 
#endif

