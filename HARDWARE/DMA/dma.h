#ifndef __DMA_H
#define	__DMA_H	   
#include "sys.h"							    					    

void UartRxDMA1_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr);//≈‰÷√DMA1_CHx
void UartTxDMA1_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr);
void DMA_Enable(DMA_Channel_TypeDef*DMA_CHx, u16 ndtr);// πƒ‹DMA1_CHx
		   
#endif





