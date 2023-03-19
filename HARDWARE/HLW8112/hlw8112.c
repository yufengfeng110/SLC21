/**********************************************************************
* Copyright (C) 2022-2023 �����������޹�˾ 
* �ļ���: hlw8112.c
* ����:  hlw8112����оƬ�����ļ�
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


//char RunTimeInfo[400];		//������������ʱ����Ϣ
//char InfoBuffer[1000];				//������Ϣ������
ADCSearchObj	g_oADCSearchObj;

u16 		U16_RMSIAC_RegData; 						// Aͨ������ת��ϵ��
u16 		U16_RMSIBC_RegData; 	    				// Bͨ������ת��ϵ��
u16 		U16_RMSUC_RegData; 		    				// ��ѹͨ��ת��ϵ��
u16 		U16_PowerPAC_RegData; 						// Aͨ������ת��ϵ��
u16 		U16_PowerPBC_RegData; 						// Bͨ������ת��ϵ��
u16 		U16_PowerSC_RegData; 						// ���ڹ���ת��ϵ�������ѡ��Aͨ������Aͨ�����ڹ���ת��ϵ����ABֻ�ܶ�ѡ��һ
u16 		U16_EnergyAC_RegData; 						// Aͨ���й�����ת��ϵ��
u16 		U16_EnergyBC_RegData; 						// Bͨ���й�����ת��ϵ��
u16 		U16_HFConst_RegData;	    				// ��������
///////////////////////////////////////////////////////
void   	HC4052_Switch(unsigned char hlw8112_num);                	//74HC4052ͨ���л�����
void   	HLW8112_CmdData_Read(unsigned char Addr);                	//HLW8112������ 
void   	HLW8112_CmdData_Write(unsigned char Addr,
                unsigned int Data,unsigned char DataNum);       	//HLW8112д���� 
u8  	HLW8112_Check(unsigned char Addr,unsigned char Number);  	//HLW8112����У��   
u16 	HLW8112_Read_Coefficient(unsigned char Addr);           	//��ȡ����8112ϵ��
void 	HLW8112Coefficient_Read(void);            					//��ȡָ��8112ϵ�� 
void   	Init_HLW8112(unsigned char num);                         	//8112��ʼ��
u32 	HLW8112_Uart2Read(unsigned char reg,unsigned char len);  	//��ȡ�������ݲ�����оƬ����ֵ
float  	HLW8112_Read_Voltage(void);                              	//����ѹֵ 
float  	GetCurData(unsigned long int dat,unsigned int reg,
                 float base,unsigned char item);                	//����ֵ���㷽��
float 	HLW8112_Read_CurrentA(unsigned char ACchannel);           	//��Aͨ������ֵ
float 	HLW8112_Read_CurrentB(unsigned char ACchannel);           	//��Bͨ������ֵ
float 	GetKwhData(unsigned long int dat,unsigned int reg,			//��������͹���
                 float base,unsigned char item);
float 	HLW8112_Read_PowerA(unsigned char ACchannel);             	//��Aͨ������ֵ 
float 	HLW8112_Read_PowerB(unsigned char ACchannel);             	//��Bͨ������ֵ
void 	HLW8112_Read_ENERGYA(unsigned char ACchannel);            	//��Aͨ������ֵ
void 	HLW8112_Read_ENERGYB(unsigned char ACchannel);            	//��Bͨ������ֵ
void  	HLW8112ReadA(ADCElecObj* obj,unsigned char item);         	//��ȡAͨ���������� 
void  	HLW8112ReadB(ADCElecObj* obj,unsigned char item);         	//��ȡBͨ������� 
void  	HLW8112Data_Read(unsigned char num);                      	//��������������ȡ 
float 	HLW8112_Read_Factor(float power);							//����������
//////////////////////////////////////////////////////

/*************************************************/
//4052��ʼ��
//4052_A:PB0  4052_B:PB1                         
/*************************************************/
void HC4052_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	  		//ʹ��PB�˿�ʱ��

  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;				        
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		    	//�������
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		    	//IO���ٶ�Ϊ50MHz
  	GPIO_Init(GPIOB, &GPIO_InitStructure);					    //�����趨������ʼ��GPIOB.15
 	GPIO_ResetBits(GPIOB, GPIO_Pin_0|GPIO_Pin_1);					//PB0.1�����
}
/*************************************************/
/* 74HC4052ͨ���л�����                          */
/*************************************************/
void HC4052_Switch(unsigned char hlw8112_num)
{
	switch(hlw8112_num)
	{
		case 1:	 //������AC220V						
			HC4052_A = 0;
			HC4052_B = 0;
			break;
		case 2:	//AC220V����1\2
			HC4052_A = 1;
			HC4052_B = 0;
			break;
	}
}
/*************************************************/
/* 8112��ʼ��                                    */
/*************************************************/
void Init_HLW8112(unsigned char num)
{
	if(num == 1)														//220��������ɼ�
	{
		HLW8112_CmdData_Write(0XEA,0xE5,1);		 						//дʹ��
		HLW8112_CmdData_Write(0XEA,0x5A,1);		 						
		//ea + 5a ����ͨ��A�������ָ����ǰ���ڼ������ڹ��ʡ�������������ǡ�
		//˲ʱ�й����ʡ�˲ʱ���ڹ��ʺ��й����ʹ��ص��ź�ָʾ ��ͨ��Ϊͨ��A 
		HLW8112_CmdData_Write(0X40,0x4000,2);	 						//д�ж����ú�����Ĵ���IE��ʹ�ܵ�ѹ�����ж�
		HLW8112_CmdData_Write(0X1D,0x3299,2);							//ʹINT1��INT2�������ѹͨ�������ź�
		HLW8112_CmdData_Write(0X13,0x0CE5,2);	 						//дEMUCON2��ʹEnergy�������㣬������;
		HLW8112_CmdData_Write(0X01,0x1183,2);	 						//дEMUCON ZXD1=1 ZXD0=1
		HLW8112_CmdData_Write(0X00,0x0B1B,2);	 						//дSYSCON��������ͨ��A PGA=8 ��ѹͨ��PGA=8
		HLW8112_CmdData_Write(0XEA,0xDC,1);	
	}
	else if(num == 2)													//220��������ɼ�
	{
		HLW8112_CmdData_Write(0XEA,0xE5,1);		 						//дʹ��
		HLW8112_CmdData_Write(0XEA,0x5A,1);		 						
		//ea + 5a ����ͨ��A�������ָ����ǰ���ڼ������ڹ��ʡ�������������ǡ�
		//˲ʱ�й����ʡ�˲ʱ���ڹ��ʺ��й����ʹ��ص��ź�ָʾ ��ͨ��Ϊͨ��A 
		HLW8112_CmdData_Write(0X40,0x4000,2);	 						//д�ж����ú�����Ĵ���IE��ʹ�ܵ�ѹ�����ж�
		HLW8112_CmdData_Write(0X1D,0x3299,2);							//ʹINT1��INT2�������ѹͨ�������ź�
		HLW8112_CmdData_Write(0X13,0x0C81,2);	 						//дEMUCON2��ʹEnergy�������㣬�رչ�����;
		HLW8112_CmdData_Write(0X01,0x1183,2);	 						//дEMUCON ZXD1=1 ZXD0=1
		HLW8112_CmdData_Write(0X00,0x0F1C,2);	 						//дSYSCON��������ͨ��A��B PGA=16 ��ѹͨ��PGA=8
		HLW8112_CmdData_Write(0XEA,0xDC,1);	
	}
}
/*************************************************/
/* ��ʼ�������ɼ�оƬ                            */
/*************************************************/
void HLW8112_Init(void)
{
	HC4052_Init();
	HC4052_Switch(AC220_INPUT);
	delay_ms(500);
	Init_HLW8112(AC220_INPUT);									//220v������8112��ʼ��
	delay_ms(20);

	HC4052_Switch(AC220_OUTPUT12);								//�л���220v���1��2
	delay_ms(100);
	Init_HLW8112(AC220_OUTPUT12);
	delay_ms(20);
	DEBUG("HLW8112 Init Done!!\r\n");
}

/*************************************************/
/* HLW8112������                                */
/*************************************************/
void HLW8112_CmdData_Read(unsigned char Addr)
{
	USART_SendByte(USART2, 0xA5);
	USART_SendByte(USART2, Addr);
}
/*************************************************/
/* HLW8112д����                                */
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
/* 8112У��                                      */
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
/* ��ȡ����8112ϵ��                                  */
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
/* ��ȡ���8112ϵ��                               */
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
/* ��ȡ�������ݲ�����оƬ����ֵ                   */
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
/* ����ѹֵ                                      */
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
		b = b*4/8.0;	//��ѹϵ��0.25 PGA:8				
	  	b = b/100.0;  //�ó���ֵ��λ���Ҫ����ɷ��ó���100
	  	b = b*1.0;								
	}
	return b;
}
/*************************************************/
/* ����ֵ���㷽��                                 */
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
/* ��Aͨ������ֵ                                  */
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
/* ��Bͨ������ֵ                                  */
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
/* ��������͹��ʷ���                             	  */
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
/* �������                             		  */
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
// 	if(fRet >= 0.01)  //��������ۼƵ�������0.01�ȵ�
// 	{
// 		DEBUG("��������0.01\r\n");
// 		DEBUG("����ֵ��%f\r\n", fRet);
// 		HLW8112_CmdData_Write(0XEA,0xE5,1);	//�Ĵ���дʹ��
// 		HLW8112_CmdData_Write(0X13,0x08E5,2);//���õ����Ĵ�����������
// 		HLW8112_Uart2Read(REG_ENERGY_PA_ADDR,3);//��������
// 		Udata = HLW8112_Uart2Read(REG_ENERGY_PA_ADDR,3);
// 		DEBUG("����������Чֵ�Ĵ�����%08X\r\n", Udata);
// 		HLW8112_CmdData_Write(0X13,0x0CE5,2);//���õ����Ĵ�����������
// 		HLW8112_CmdData_Write(0XEA,0xDC,1);//�Ĵ���дʧ��
// 	}		
// 	else
// 	{
// 		fRet = 0;
// 	}
// 	return fRet;

// }
/*************************************************/
/* ��Aͨ������ֵ                                  */
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
/* ��Bͨ������ֵ                                  */
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
/* ��Aͨ������ֵ                                 */
/*************************************************/
void HLW8112_Read_ENERGYA(unsigned char ACchannel)
{
	unsigned int Udata = 0;
	unsigned int energy = 0;
	float energy_a = 0;
	Udata = HLW8112_Uart2Read(REG_ENERGY_PA_ADDR,3);
	energy_a = GetKwhData(Udata,U16_EnergyAC_RegData,536870912.0,(ACchannel)/2);
	if(energy_a >= 0.01)  //��������ۼƵ�������0.01�ȵ�
	{
		DEBUG("A��������0.01\r\n");
		DEBUG("A����ֵ��%f\r\n", energy_a);
		HLW8112_CmdData_Write(0XEA,0xE5,1);			//�Ĵ���дʹ��
		HLW8112_CmdData_Write(0X13,0x08E5,2);		//���õ����Ĵ�����������
		HLW8112_Uart2Read(REG_ENERGY_PA_ADDR,3);	//��������
		energy = HLW8112_Uart2Read(REG_ENERGY_PA_ADDR,3);
		DEBUG("A����������Чֵ�Ĵ�����%08X\r\n", energy);
		HLW8112_CmdData_Write(0X13,0x0CE5,2);		//���õ����Ĵ�����������
		HLW8112_CmdData_Write(0XEA,0xDC,1);			//�Ĵ���дʧ��
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
/* ��Bͨ������ֵ                                 */
/*************************************************/
void HLW8112_Read_ENERGYB(unsigned char ACchannel)
{
	unsigned int Udata = 0;
	unsigned int energy = 0;
	float energy_b = 0;

	Udata = HLW8112_Uart2Read(REG_ENERGY_PB_ADDR,3);
	energy_b = GetKwhData(Udata,U16_EnergyBC_RegData,536870912.0,(ACchannel)/2);
	DEBUG("B���ܣ�%f\r\n", energy_b);
	if(energy_b >= 0.01)  //��������ۼƵ�������0.01�ȵ�
	{
		DEBUG("B��������0.01\r\n");
		DEBUG("B����ֵ��%f\r\n", energy_b);
		HLW8112_CmdData_Write(0XEA,0xE5,1);			//�Ĵ���дʹ��
		HLW8112_CmdData_Write(0X13,0x0481,2);		//���õ����Ĵ�����������
		HLW8112_Uart2Read(REG_ENERGY_PB_ADDR,3);	//��������
		energy = HLW8112_Uart2Read(REG_ENERGY_PB_ADDR,3);
		DEBUG("B����������Чֵ�Ĵ�����%08X\r\n", energy);
		HLW8112_CmdData_Write(0X13,0x0C81,2);		//���õ����Ĵ�����������
		HLW8112_CmdData_Write(0XEA,0xDC,1);			//�Ĵ���дʧ��	
		g_oADCSearchObj.oAC220_Out[1].fdQ += energy_b;	
	}
}
/*************************************************/
/*��ȡ��������                                    */
/*************************************************/
float HLW8112_Read_Factor(float power)
{
  float a;
  unsigned long b;

  b = HLW8112_Uart2Read(REG_PF_ADDR,3);
	// U16_PFData = b;
 	
  
  if (b>0x800000)       //Ϊ�������Ը���
  {
      a = (float)(0xffffff-b + 1)/0x7fffff;
  }
  else
  {
      a = (float)b/0x7fffff;
  }
  
  // ������ʺ�С���ӽ�0����ô��PF = �й�/���ڹ��� = 1����ô�˴�Ӧ��PF ��� 0��
  
  if (power < 0.3) // С��0.3W
	  a = 0; 
  
   return a;	
}
/*************************************************/
/* ��ȡAͨ����������                             */
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
/* ��ȡBͨ�������                                */
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
/* ��������������ȡ                               */
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
/* �������������ȡ                              */
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
/* HLW8112������                              */
/*************************************************/
void Electricparam_task(void *pvParameters)
{
	static u16 loop_num;

	while(1)
	{
//		memset(&g_oADCSearchObj, 0, sizeof(ADCSearchObj));					//��ʼ�����ܽṹ�����
		printf("Electricparam_task running\r\n");
		// taskENTER_CRITICAL();           	//�����ٽ���
		HLW8112_CapData();
		g_oADCSearchObj.temprature = Get_Temprate()/100;
		// taskEXIT_CRITICAL();            	//�˳��ٽ���
		loop_num++;
		if(loop_num%10 == 0)    			//ÿ��10s�Ӵ�ӡһ��
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
//			/*�鿴����������ʱ��*/
//			memset(RunTimeInfo,0,400);				//��Ϣ����������
//			vTaskGetRunTimeStats(RunTimeInfo);		//��ȡ��������ʱ����Ϣ
//			DEBUG("������\t\t����ʱ��\t������ռ�ٷֱ�\r\n");
//			DEBUG("%s\r\n",RunTimeInfo);
//			/*******************************************************************/	
//			//����vTaskList()��ʹ��	
//			DEBUG("/*************����vTaskList()��ʹ��*************/\r\n");
//			vTaskList(InfoBuffer);					//��ȡ�����������Ϣ
//			DEBUG("%s\r\n",InfoBuffer);			//ͨ�����ڴ�ӡ�����������Ϣ
//			DEBUG("/**************************����**************************/\r\n");
		}
		if(loop_num%1200 == 0)						//20���Ӵ�һ�ε�������
		{
			loop_num = 0;
			xEventGroupSetBits(gl_IcapDevObj.oEventGroupHandler, EVENT_SAVEENERGY);
		}
		IWDG_Feed(); 
		vTaskDelay(1000);
	}
}


