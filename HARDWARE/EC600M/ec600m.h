#ifndef __EC600M_H
#define __EC600M_H
#include "stm32f10x.h"


#define EC600_OPEN {GPIO_ResetBits(GPIOA, GPIO_Pin_11);\
										delay_ms(5);\
										GPIO_SetBits(GPIOA, GPIO_Pin_11);\
										delay_ms(700);\
										GPIO_ResetBits(GPIOA, GPIO_Pin_11);}

extern char ATOPEN[60];
void EC600M_RUN(void);
void EC600M_Init(void);
int EC600M_OpenSocket(int socketid, char* sockettype, u8* serverip, u16 serverport);
void EC600M_CloseSocket(int socketid);
int EC600M_Send(int socketid, char *pchBuf, u16 len);
void* EC600M_Receive(u32 overtime);
void clear_RXBuffer(void);
void clear_TXBuffer(void);
void reset(void);
void BGtask_Netcomm(void);
void BGtask_Send(char *pchBuf, u16 len);
void Query_Socketstate(void);
void reset(void);
void PackATOPEN(int socketid, char* sockettype, char* serverip, u16 serverport);
void EC600M_GetCSQ(void);
#endif



