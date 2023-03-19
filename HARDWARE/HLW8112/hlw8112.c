/**********************************************************************
* Copyright (C) 2022-2023 天津白泽技术有限公司 
* 文件名: hlw8112.c
* 描述:  hlw8112电量芯片操作文件
**********************************************************************/
#ifndef HLW8112_H
#include "hlw8112.h"
#endif
#include <stdio.h> 
#include <string.h> 
#include "usart.h"
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "DefBean.h"
#include "sys.h"
#include "rtc.h"
#include "tsensor.h"
#include "wdg.h"
#include "ec600m.h"


//char RunTimeInfo[400];		//保存任务运行时间信息
//char InfoBuffer[1000];				//保存信息的数组
ADCSearchObj	g_oADCSearchObj;

u16 		U16_RMSIAC_RegData; 						// A通道电流转换系数
u16 		U16_RMSIBC_RegData; 	    				// B通道电流转换系数
u16 		U16_RMSUC_RegData; 		    				// 电压通道转换系数
u16 		U16_PowerPAC_RegData; 						// A通道功率转换系数
u16 		U16_PowerPBC_RegData; 						// B通道功率转换系数
u16 		U16_PowerSC_RegData; 						// 视在功率转换系数，如果选择A通道则是A通道视在功率转换系数，AB只能二选其一
u16 		U16_EnergyAC_RegData; 						// A通道有功电能转换系数
u16 		U16_EnergyBC_RegData; 						// B通道有功电能转换系数
u16 		U16_HFConst_RegData;	    				// 电量常数
///////////////////////////////////////////////////////
void   	HC4052_Switch(unsigned char hlw8112_num);                	//74HC4052通道切换函数
void   	HLW8112_CmdData_Read(unsigned char Addr);                	//HLW8112读命令 
void   	HLW8112_CmdData_Write(unsigned char Addr,
                unsigned int Data,unsigned char DataNum);       	//HLW8112写命令 
u8  	HLW8112_Check(unsigned char Addr,unsigned char Number);  	//HLW8112数据校验   
u16 	HLW8112_Read_Coefficient(unsigned char Addr);           	//读取单个8112系数
void 	HLW8112Coefficient_Read(void);            					//读取指定8112系数 
void   	Init_HLW8112(unsigned char num);                         	//8112初始化
u32 	HLW8112_Uart2Read(unsigned char reg,unsigned char len);  	//读取串口数据并计算芯片返回值
float  	HLW8112_Read_Voltage(void);                              	//读电压值 
float  	GetCurData(unsigned long int dat,unsigned int reg,
                 float base,unsigned char item);                	//电流值计算方法
float 	HLW8112_Read_CurrentA(unsigned char ACchannel);           	//读A通道电流值
float 	HLW8112_Read_CurrentB(unsigned char ACchannel);           	//读B通道电流值
float 	GetKwhData(unsigned long int dat,unsigned int reg,			//计算电量和功率
                 float base,unsigned char item);
float 	HLW8112_Read_PowerA(unsigned char ACchannel);             	//读A通道功率值 
float 	HLW8112_Read_PowerB(unsigned char ACchannel);             	//读B通道功率值
void 	HLW8112_Read_ENERGYA(unsigned char ACchannel);            	//读A通道电量值
void 	HLW8112_Read_ENERGYB(unsigned char ACchannel);            	//读B通道电量值
void  	HLW8112ReadA(ADCElecObj* obj,unsigned char item);         	//读取A通道电量参数 
void  	HLW8112ReadB(ADCElecObj* obj,unsigned char item);         	//读取B通道电参数 
void  	HLW8112Data_Read(unsigned char num);                      	//单个电量参数读取 
float 	HLW8112_Read_Factor(float power);							//读功率因数
//////////////////////////////////////////////////////

/*************************************************/
//4052初始化
//4052_A:PB0  4052_B:PB1                         
/*************************************************/
void HC4052_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	  		//使能PB端口时钟

  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;				        
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		    	//推挽输出
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		    	//IO口速度为50MHz
  	GPIO_Init(GPIOB, &GPIO_InitStructure);					    //根据设定参数初始化GPIOB.15
 	GPIO_ResetBits(GPIOB, GPIO_Pin_0|GPIO_Pin_1);					//PB0.1输出低
}
/*************************************************/
/* 74HC4052通道切换函数                          */
/*************************************************/
void HC4052_Switch(unsigned char hlw8112_num)
{
	switch(hlw8112_num)
	{
		case 1:	 //总输入AC220V						
			HC4052_A = 0;
			HC4052_B = 0;
			break;
		case 2:	//AC220V输入1\2
			HC4052_A = 1;
			HC4052_B = 0;
			break;
	}
}
/*************************************************/
/* 8112初始化                                    */
/*************************************************/
void Init_HLW8112(unsigned char num)
{
	if(num == 1)														//220输入电量采集
	{
		HLW8112_CmdData_Write(0XEA,0xE5,1);		 						//写使能
		HLW8112_CmdData_Write(0XEA,0x5A,1);		 						
		//ea + 5a 电流通道A设置命令，指定当前用于计算视在功率、功率因数、相角、
		//瞬时有功功率、瞬时视在功率和有功功率过载的信号指示 的通道为通道A 
		HLW8112_CmdData_Write(0X40,0x4000,2);	 						//写中断配置和允许寄存器IE，使能电压过零中断
		HLW8112_CmdData_Write(0X1D,0x3299,2);							//使INT1、INT2都输出电压通道过零信号
		HLW8112_CmdData_Write(0X13,0x0CE5,2);	 						//写EMUCON2，使Energy读后不清零，过零检测;
		HLW8112_CmdData_Write(0X01,0x1183,2);	 						//写EMUCON ZXD1=1 ZXD0=1
		HLW8112_CmdData_Write(0X00,0x0B1B,2);	 						//写SYSCON开启电流通道A PGA=8 电压通道PGA=8
		HLW8112_CmdData_Write(0XEA,0xDC,1);	
	}
	else if(num == 2)													//220输出电量采集
	{
		HLW8112_CmdData_Write(0XEA,0xE5,1);		 						//写使能
		HLW8112_CmdData_Write(0XEA,0x5A,1);		 						
		//ea + 5a 电流通道A设置命令，指定当前用于计算视在功率、功率因数、相角、
		//瞬时有功功率、瞬时视在功率和有功功率过载的信号指示 的通道为通道A 
		HLW8112_CmdData_Write(0X40,0x4000,2);	 						//写中断配置和允许寄存器IE，使能电压过零中断
		HLW8112_CmdData_Write(0X1D,0x3299,2);							//使INT1、INT2都输出电压通道过零信号
		HLW8112_CmdData_Write(0X13,0x0C81,2);	 						//写EMUCON2，使Energy读后不清零，关闭过零检测;
		HLW8112_CmdData_Write(0X01,0x1183,2);	 						//写EMUCON ZXD1=1 ZXD0=1
		HLW8112_CmdData_Write(0X00,0x0F1C,2);	 						//写SYSCON开启电流通道A、B PGA=16 电压通道PGA=8
		HLW8112_CmdData_Write(0XEA,0xDC,1);	
	}
}
/*************************************************/
/* 初始化电量采集芯片                            */
/*************************************************/
void HLW8112_Init(void)
{
	HC4052_Init();
	HC4052_Switch(AC220_INPUT);
	delay_ms(500);
	Init_HLW8112(AC220_INPUT);									//220v总输入8112初始化
	delay_ms(20);

	HC4052_Switch(AC220_OUTPUT12);								//切换成220v输出1和2
	delay_ms(100);
	Init_HLW8112(AC220_OUTPUT12);
	delay_ms(20);
	DEBUG("HLW8112 Init Done!!\r\n");
}

/*************************************************/
/* HLW8112读命令                                */
/*************************************************/
void HLW8112_CmdData_Read(unsigned char Addr)
{
	USART_SendByte(USART2, 0xA5);
	USART_SendByte(USART2, Addr);
}
/*************************************************/
/* HLW8112写命令                                */
/*************************************************/
void HLW8112_CmdData_Write(unsigned char Addr,unsigned int Data,unsigned char DataNum)
{
	unsigned char a = 0;
	unsigned char i;
	unsigned char dat[2];
	USART_SendByte(USART2, 0xA5);
	if(Addr != 0XEA)
	{
		Addr |= 0x80;
		dat[0] = Data>>8;
		dat[1] = (unsigned char)Data;
	}
	else
	{
		dat[0] = (unsigned char)Data;
	}
	USART_SendByte(USART2, Addr);
	for(i=0; i<DataNum; i++)
	{
		USART_SendByte(USART2, dat[i]);
		a += dat[i]; 
	}
	 a = a + 0xA5 + Addr;
	 a = ~a;
	 USART_SendByte(USART2, a); 
}
/*************************************************/
/* 8112校验                                      */
/*************************************************/
u8 HLW8112_Check(unsigned char Addr,unsigned char Number)
{
	unsigned char a = 0;
	unsigned char i;
	for(i = 0;i<Number; i++)
	{
		a+=	USART2_RX_BUF[i];
	}
	a = a + 0xA5 + Addr;
	a = ~a;
	return a;
}
/*************************************************/
/* 读取单个8112系数                                  */
/*************************************************/
u16 HLW8112_Read_Coefficient(unsigned char Addr)
{
	unsigned int Coefficient;
	HLW8112_CmdData_Read(Addr);
	delay_ms(10);
	if(USART2_RX_BUF[2] == HLW8112_Check(Addr,2))
	{
		Coefficient = USART2_RX_BUF[0]*256 + USART2_RX_BUF[1];
	}
	USART2_RX_NUM = 0;
	return Coefficient;
}
/*************************************************/
/* 读取多个8112系数                               */
/*************************************************/
void HLW8112Coefficient_Read(void)
{
	U16_RMSIAC_RegData	 = HLW8112_Read_Coefficient(REG_RMS_IAC_ADDR);
	delay_ms(3);
	U16_RMSIBC_RegData	 = HLW8112_Read_Coefficient(REG_RMS_IBC_ADDR);
	delay_ms(3);
	U16_RMSUC_RegData 	 = HLW8112_Read_Coefficient(REG_RMS_UC_ADDR);
	delay_ms(3);
	U16_PowerPAC_RegData = HLW8112_Read_Coefficient(REG_POWER_PAC_ADDR);
	delay_ms(3);
	U16_PowerPBC_RegData = HLW8112_Read_Coefficient(REG_POWER_PBC_ADDR);
	delay_ms(3);	
	U16_EnergyAC_RegData = HLW8112_Read_Coefficient(REG_ENERGY_AC_ADDR);
	delay_ms(3);	
	U16_EnergyBC_RegData = HLW8112_Read_Coefficient(REG_ENERGY_BC_ADDR);
}

/*************************************************/
/* 读取串口数据并计算芯片返回值                   */
/*************************************************/
u32 HLW8112_Uart2Read(unsigned char reg,unsigned char len)
{
	u32 Udata = 0;
	unsigned char t = 0;	
BEGIN:
	USART2_RX_NUM = 0;
	HLW8112_CmdData_Read(reg);
	delay_ms(10);
	if(USART2_RX_BUF[len] == HLW8112_Check(reg,len))
	{
		if(len == 3)
		{
			Udata = USART2_RX_BUF[0]*65536 + USART2_RX_BUF[1]*256 + 
		       	    USART2_RX_BUF[2];				
		}
		else
		{
			Udata = USART2_RX_BUF[0]*16777216 + USART2_RX_BUF[1]*65536 + 
		        	USART2_RX_BUF[2]*256 + USART2_RX_BUF[3];				
		}
	}
	else
	{
		t++;
//		DEBUG("@@@== %bd\r\n",t);
		if(t < 5)
		{
			goto BEGIN;
		}
	}
//	DEBUG("@@@== END\r\n");
	USART2_RX_NUM = 0;	
	return Udata;
}
/*************************************************/
/* 读电压值                                      */
/*************************************************/
float HLW8112_Read_Voltage(void)
{
	float b = 0;
	unsigned int Udata = 0;

	Udata = HLW8112_Uart2Read(REG_RMSU_ADDR,3);
//	DEBUG("Voltage data = %lu\r\n",Udata);
	if((Udata & 0x800000) == 0x800000)
	{	
		b = 0.0;
	}
	else
	{
		b = (float)Udata;					
	 	b = b*((float)(U16_RMSUC_RegData)); 	
	  	b = b/4194304.0;       				
		b = b*4/8.0;	//电压系数0.25 PGA:8				
	  	b = b/100.0;  //得出的值单位如果要换算成伏得除以100
	  	b = b*1.0;								
	}
	return b;
}
/*************************************************/
/* 电流值计算方法                                 */
/*************************************************/
float GetCurData(unsigned long int dat,unsigned int reg,
                 float base,unsigned char item)
{
	float fRet = 0;

	fRet = (float)dat;				
	fRet = fRet*((float)(reg)); 
	if(item == 0)
	{
		fRet = fRet*2.0;
	}	
	fRet = fRet/10.0;
	fRet = fRet/1000.0;	
	fRet = fRet/base;
	fRet = fRet*1.0;		
	return fRet;
}
/*************************************************/
/* 读A通道电流值                                  */
/*************************************************/
float HLW8112_Read_CurrentA(unsigned char ACchannel)
{
	float b = 0;
	unsigned int Udata = 0;

	Udata = HLW8112_Uart2Read(REG_RMSIA_ADDR,3);
	if((Udata & 0x800000) == 0x800000)
	{	
		b = 0.0;
	}
	else
	{
		b = GetCurData(Udata,U16_RMSIAC_RegData,8388608.0,(ACchannel)/2);
	}
	return b;
}
/*************************************************/
/* 读B通道电流值                                  */
/*************************************************/
float HLW8112_Read_CurrentB(unsigned char ACchannel)
{
	float b = 0;
	unsigned int Udata = 0;
	
	Udata = HLW8112_Uart2Read(REG_RMSIB_ADDR,3);
	if ((Udata & 0x800000) == 0x800000)
	{	
		b = 0.0;
	}
	else
	{
		b = GetCurData(Udata,U16_RMSIBC_RegData,8388608.0,(ACchannel)/2);
	}
	return b;
}
/*************************************************/
/* 计算电量和功率方法                             	  */
/*************************************************/
float GetKwhData(unsigned long int dat,unsigned int reg,
                 float base,unsigned char item)
{
	float fRet = 0;

	fRet = (float)dat;				
	fRet = fRet*((float)(reg)); 
	if(item == 0)
	{
		fRet = fRet*2.0;
	}	
	fRet = fRet/base;						
	fRet = fRet*4.00/8.0;	
	fRet = fRet/10.0;
	fRet = fRet*1.0;
	return fRet;
}
/*************************************************/
/* 计算电量                             		  */
/*************************************************/
// float GetEnergyData(unsigned long int dat,unsigned int reg,
//                  float base,unsigned char item)
// {
// 	float fRet = 0;
// 	unsigned int Udata = 0;

// 	fRet = (float)dat;				
// 	fRet = fRet*((float)(reg)); 
// 	fRet = fRet/base;						
// 	fRet = fRet*4.00/8.0;	
// 	if(item == 1)
// 	{
// 		fRet = fRet/10.0;
// 	}
// 	else
// 	{
// 		fRet = fRet/680.0;
// 	}
// 	fRet = fRet*1.0;
// 	if(fRet >= 0.01)  //如果读到累计电量大于0.01度电
// 	{
// 		DEBUG("电量大于0.01\r\n");
// 		DEBUG("电量值：%f\r\n", fRet);
// 		HLW8112_CmdData_Write(0XEA,0xE5,1);	//寄存器写使能
// 		HLW8112_CmdData_Write(0X13,0x08E5,2);//设置电量寄存器读后清零
// 		HLW8112_Uart2Read(REG_ENERGY_PA_ADDR,3);//读后清零
// 		Udata = HLW8112_Uart2Read(REG_ENERGY_PA_ADDR,3);
// 		DEBUG("清零后电能有效值寄存器：%08X\r\n", Udata);
// 		HLW8112_CmdData_Write(0X13,0x0CE5,2);//设置电量寄存器读后不清零
// 		HLW8112_CmdData_Write(0XEA,0xDC,1);//寄存器写失能
// 	}		
// 	else
// 	{
// 		fRet = 0;
// 	}
// 	return fRet;

// }
/*************************************************/
/* 读A通道功率值                                  */
/*************************************************/
float HLW8112_Read_PowerA(unsigned char ACchannel)
{
	unsigned int Udata = 0;

	Udata = HLW8112_Uart2Read(REG_POWER_PA_ADDR,4);
//	DEBUG("PowerA_Data:%x\r\n", Udata);
	if(Udata > 0x80000000)
	{	
		Udata = ~Udata;
	}
	return GetKwhData(Udata,U16_PowerPAC_RegData,2147483648.0,(ACchannel/2));
}
/*************************************************/
/* 读B通道功率值                                  */
/*************************************************/
float HLW8112_Read_PowerB(unsigned char ACchannel)
{
	unsigned int Udata = 0;
	
	Udata = HLW8112_Uart2Read(REG_POWER_PB_ADDR,4);
//	DEBUG("PowerB_Data:%x\r\n", Udata);
	if(Udata  > 0x80000000)
	{	
		Udata = ~Udata;
	}									
	return GetKwhData(Udata,U16_PowerPBC_RegData,2147483648.0,(ACchannel)/2);
}
/*************************************************/
/* 读A通道电量值                                 */
/*************************************************/
void HLW8112_Read_ENERGYA(unsigned char ACchannel)
{
	unsigned int Udata = 0;
	unsigned int energy = 0;
	float energy_a = 0;
	Udata = HLW8112_Uart2Read(REG_ENERGY_PA_ADDR,3);
	energy_a = GetKwhData(Udata,U16_EnergyAC_RegData,536870912.0,(ACchannel)/2);
	if(energy_a >= 0.01)  //如果读到累计电量大于0.01度电
	{
		DEBUG("A电量大于0.01\r\n");
		DEBUG("A电量值：%f\r\n", energy_a);
		HLW8112_CmdData_Write(0XEA,0xE5,1);			//寄存器写使能
		HLW8112_CmdData_Write(0X13,0x08E5,2);		//设置电量寄存器读后清零
		HLW8112_Uart2Read(REG_ENERGY_PA_ADDR,3);	//读后清零
		energy = HLW8112_Uart2Read(REG_ENERGY_PA_ADDR,3);
		DEBUG("A清零后电能有效值寄存器：%08X\r\n", energy);
		HLW8112_CmdData_Write(0X13,0x0CE5,2);		//设置电量寄存器读后不清零
		HLW8112_CmdData_Write(0XEA,0xDC,1);			//寄存器写失能
		switch (ACchannel)
		{
			case 1:
				g_oADCSearchObj.oAC220_In.fdQ += energy_a;
				break;
			case 2:
				g_oADCSearchObj.oAC220_Out[0].fdQ += energy_a;
				break;
		}
	}
}
/*************************************************/
/* 读B通道电量值                                 */
/*************************************************/
void HLW8112_Read_ENERGYB(unsigned char ACchannel)
{
	unsigned int Udata = 0;
	unsigned int energy = 0;
	float energy_b = 0;

	Udata = HLW8112_Uart2Read(REG_ENERGY_PB_ADDR,3);
	energy_b = GetKwhData(Udata,U16_EnergyBC_RegData,536870912.0,(ACchannel)/2);
	DEBUG("B电能：%f\r\n", energy_b);
	if(energy_b >= 0.01)  //如果读到累计电量大于0.01度电
	{
		DEBUG("B电量大于0.01\r\n");
		DEBUG("B电量值：%f\r\n", energy_b);
		HLW8112_CmdData_Write(0XEA,0xE5,1);			//寄存器写使能
		HLW8112_CmdData_Write(0X13,0x0481,2);		//设置电量寄存器读后清零
		HLW8112_Uart2Read(REG_ENERGY_PB_ADDR,3);	//读后清零
		energy = HLW8112_Uart2Read(REG_ENERGY_PB_ADDR,3);
		DEBUG("B清零后电能有效值寄存器：%08X\r\n", energy);
		HLW8112_CmdData_Write(0X13,0x0C81,2);		//设置电量寄存器读后不清零
		HLW8112_CmdData_Write(0XEA,0xDC,1);			//寄存器写失能	
		g_oADCSearchObj.oAC220_Out[1].fdQ += energy_b;	
	}
}
/*************************************************/
/*读取功率因数                                    */
/*************************************************/
float HLW8112_Read_Factor(float power)
{
  float a;
  unsigned long b;

  b = HLW8112_Uart2Read(REG_PF_ADDR,3);
	// U16_PFData = b;
 	
  
  if (b>0x800000)       //为负，容性负载
  {
      a = (float)(0xffffff-b + 1)/0x7fffff;
  }
  else
  {
      a = (float)b/0x7fffff;
  }
  
  // 如果功率很小，接近0，那么因PF = 有功/视在功率 = 1，那么此处应将PF 设成 0；
  
  if (power < 0.3) // 小于0.3W
	  a = 0; 
  
   return a;	
}
/*************************************************/
/* 读取A通道电量参数                             */
/*************************************************/
void HLW8112ReadA(ADCElecObj* obj,unsigned char item)
{
	obj->fdV = HLW8112_Read_Voltage();	
	obj->fdA = HLW8112_Read_CurrentA(item);					 									
	obj->fdP = HLW8112_Read_PowerA(item);					 										
	HLW8112_Read_ENERGYA(item);
	obj->ffc = HLW8112_Read_Factor(obj->fdP);
}
/*************************************************/
/* 读取B通道电参数                                */
/*************************************************/
void HLW8112ReadB(ADCElecObj* obj,unsigned char item)
{
	obj->fdV = HLW8112_Read_Voltage();					 									
	obj->fdA = HLW8112_Read_CurrentB(item);					 									
	obj->fdP = HLW8112_Read_PowerB(item);				 										
	HLW8112_Read_ENERGYB(item);
	obj->ffc = HLW8112_Read_Factor(obj->fdP);
}
/*************************************************/
/* 单个电量参数读取                               */
/*************************************************/
void HLW8112Data_Read(unsigned char chipselect)
{
	switch(chipselect)
	{
		case AC220_INPUT:
			HLW8112Coefficient_Read();
			HLW8112ReadA(&(g_oADCSearchObj.oAC220_In), chipselect);
			break;
		case AC220_OUTPUT12:
			HLW8112Coefficient_Read();
			HLW8112ReadA(&(g_oADCSearchObj.oAC220_Out[0]), chipselect);
			HLW8112ReadB(&(g_oADCSearchObj.oAC220_Out[1]), chipselect);
			break;
	}
}

/*************************************************/
/* 多个电量参数读取                              */
/*************************************************/
void HLW8112_CapData(void)
{
	HC4052_Switch(AC220_INPUT);
	delay_ms(10);
	HLW8112Data_Read(AC220_INPUT);
	delay_ms(50);
	HC4052_Switch(AC220_OUTPUT12);
	delay_ms(10);
	HLW8112Data_Read(AC220_OUTPUT12);
} 
/*************************************************/
/* HLW8112服务函数                              */
/*************************************************/
void Electricparam_task(void *pvParameters)
{
	static u16 loop_num;

	while(1)
	{
//		memset(&g_oADCSearchObj, 0, sizeof(ADCSearchObj));					//初始化电能结构体变量
		printf("Electricparam_task running\r\n");
		// taskENTER_CRITICAL();           	//进入临界区
		HLW8112_CapData();
		g_oADCSearchObj.temprature = Get_Temprate()/100;
		// taskEXIT_CRITICAL();            	//退出临界区
		loop_num++;
		if(loop_num%10 == 0)    			//每隔10s钟打印一次
		{
			printf("220_IN_Voltage:%f\r\n", g_oADCSearchObj.oAC220_In.fdV);
			printf("220_IN_Current:%f\r\n", g_oADCSearchObj.oAC220_In.fdA);
			printf("220_IN_Power:%f\r\n", g_oADCSearchObj.oAC220_In.fdP);
			printf("220_IN_Energy:%f\r\n", g_oADCSearchObj.oAC220_In.fdQ);
			printf("220_IN_factor:%f\r\n", g_oADCSearchObj.oAC220_In.ffc);
			printf("220_OUT1_Voltage:%f\r\n", g_oADCSearchObj.oAC220_Out[0].fdV);
			printf("220_OUT1_Current:%f\r\n", g_oADCSearchObj.oAC220_Out[0].fdA);
			printf("220_OUT1_Power:%f\r\n", g_oADCSearchObj.oAC220_Out[0].fdP);
			printf("220_OUT1_Energy:%f\r\n", g_oADCSearchObj.oAC220_Out[0].fdQ);
			printf("220_OUT2_Voltage:%f\r\n", g_oADCSearchObj.oAC220_Out[1].fdV);
			printf("220_OUT2_Current:%f\r\n", g_oADCSearchObj.oAC220_Out[1].fdA);
			printf("220_OUT2_Power:%f\r\n", g_oADCSearchObj.oAC220_Out[1].fdP);
			printf("220_OUT2_Energy:%f\r\n", g_oADCSearchObj.oAC220_Out[1].fdQ);
			RTC_Get();
			printf("Now Time:%d-%d-%d %d:%d:%d\r\n",calendar.w_year,calendar.w_month,calendar.w_date,calendar.hour,calendar.min,calendar.sec);
			/*******************************************************************/	
//			/*查看各任务运行时间*/
//			memset(RunTimeInfo,0,400);				//信息缓冲区清零
//			vTaskGetRunTimeStats(RunTimeInfo);		//获取任务运行时间信息
//			DEBUG("任务名\t\t运行时间\t运行所占百分比\r\n");
//			DEBUG("%s\r\n",RunTimeInfo);
//			/*******************************************************************/	
//			//函数vTaskList()的使用	
//			DEBUG("/*************函数vTaskList()的使用*************/\r\n");
//			vTaskList(InfoBuffer);					//获取所有任务的信息
//			DEBUG("%s\r\n",InfoBuffer);			//通过串口打印所有任务的信息
//			DEBUG("/**************************结束**************************/\r\n");
		}
		if(loop_num%1200 == 0)						//20分钟存一次电量数据
		{
			loop_num = 0;
			xEventGroupSetBits(gl_IcapDevObj.oEventGroupHandler, EVENT_SAVEENERGY);
		}
		IWDG_Feed(); 
		vTaskDelay(1000);
	}
}


