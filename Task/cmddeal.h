#ifndef __CMDDEAL_H
#define __CMDDEAL_H
#include "stm32f10x.h"
#include "DefBean.h"

u8 Signin_Cmd(u8 retrycnt, u32 overtime);
void Heart_Cmd(void);
void UnPackCmdParam(char* pdatabuf);
int UnPackStringParam(char* buf,int len,int pos,StringParamObj* param);
void CmdDealResult(int socketid, u32 serialnum, u16 cmdid, u32 ctrlid, u32 lampid, u8* data);
void PackAddStringParam(char* buf,u16 type,u8* data);
void PackMsgCrc(char* buf);
u32 Randnumget(void);
void SoftReset(void);
#endif

