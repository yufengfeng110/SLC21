#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "24cxx.h" 
#include "delay.h"
#include "DefBean.h"


// u8 TEXT_Buffer[10]={0};
// u8 BUFFER[50] = {0};
// u8 IDBuf[10]="0000000f";
// u8 IPBuf[16]={0};
// u8 PORTBuf[6]={0};

//初始化IIC接口
void AT24CXX_Init(void)
{
	IIC_Init();
	DEBUG("AT24CXX_Init完成！\r\n");
}
//在AT24CXX指定地址读出一个数据
//ReadAddr:开始读数的地址  
//返回值  :读到的数据
u8 AT24CXX_ReadOneByte(u16 ReadAddr)
{				  
	u8 temp=0;		  	    																 
	IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(0XA0);	   //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(ReadAddr>>8);//发送高地址
		IIC_Wait_Ack();		 
	}
	else 
	{
		IIC_Send_Byte(0XA0+((ReadAddr/256)<<1));   //发送器件地址0XA0,写数据 	 
	}
		IIC_Wait_Ack(); 
		IIC_Send_Byte(ReadAddr%256);   //发送低地址
		IIC_Wait_Ack();	    
		IIC_Start();  	 	   
		IIC_Send_Byte(0XA1);           //进入接收模式			   
		IIC_Wait_Ack();	 
		temp=IIC_Read_Byte(0);		   
		IIC_Stop();//产生一个停止条件	    
	return temp;
}
//在AT24CXX指定地址写入一个数据
//WriteAddr  :写入数据的目的地址    
//DataToWrite:要写入的数据
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite)
{				   	  	    																 
	IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(0XA0);	    //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(WriteAddr>>8);//发送高地址
 	}
	else
	{
		IIC_Send_Byte(0XA0+((WriteAddr/256)<<1));   //发送器件地址0XA0,写数据 
	}	 
	IIC_Wait_Ack();	   
	IIC_Send_Byte(WriteAddr%256);   //发送低地址
	IIC_Wait_Ack(); 	 										  		   
	IIC_Send_Byte(DataToWrite);     //发送字节							   
	IIC_Wait_Ack();  		    	   
	IIC_Stop();//产生一个停止条件 
	delay_ms(10);	 
}
//在AT24CXX里面的指定地址开始写入长度为Len的数据
//该函数用于写入16bit或者32bit的数据.
//WriteAddr  :开始写入的地址  
//DataToWrite:数据数组首地址
//Len        :要写入数据的长度2,4
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len)
{  	
	u8 t;
	for(t=0;t<Len;t++)
	{
		AT24CXX_WriteOneByte(WriteAddr+t,(DataToWrite>>(8*t))&0xff);
	}												    
}

//在AT24CXX里面的指定地址开始读出长度为Len的数据
//该函数用于读出16bit或者32bit的数据.
//ReadAddr   :开始读出的地址 
//返回值     :数据
//Len        :要读出数据的长度2,4
u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len)
{  	
	u8 t;
	u32 temp=0;
	for(t=0;t<Len;t++)
	{
		temp<<=8;
		temp+=AT24CXX_ReadOneByte(ReadAddr+Len-t-1); 	 				   
	}
	return temp;												    
}
//AT24CXX清零
//WriteAddr:开始清零地址
//Clearnum:需要清零个数
void AT24CXX_Clear(u16 WriteAddr,u16 Clearnum)
{
	u16 i;
	for(i=0;i<Clearnum;i++)
	{
		AT24CXX_WriteOneByte(WriteAddr+i,0);
	}
}

//检查AT24CXX是否正常
//这里用了24XX的最后一个地址(255)来存储标志字.
//如果用其他24C系列,这个地址要修改
//返回1:检测失败
//返回0:检测成功
u8 EEPROM_Init(u16 at24cxx_type)
{
	u8 temp;

	AT24CXX_Clear(0, 512);
	temp=AT24CXX_ReadOneByte(at24cxx_type);										//避免每次开机都写AT24CXX			   
	if(temp==0X55)
	{
		printf("EEPROM有数据，读取到配置变量里......\r\n");
		AT24CXX_Read(42, &gl_LampConfigObj.RestoreFlag, 1);
		if(gl_LampConfigObj.RestoreFlag == 1)									//恢复出厂设置
		{
			printf("!!!!!!!!!有复位标志位！！");
			gl_LampConfigObj.RestoreFlag = 0;
			AT24CXX_Write(42, 0, 1);											//清恢复出厂设置标志
			strcpy((char*)gl_LampConfigObj.ControllerID, "00001333");	
			strcpy((char*)gl_LampConfigObj.ServerIP, "smartpole.bethlabs.com");    //""39.98.57.97
			gl_LampConfigObj.ServerPort = 7900;
			g_oADCSearchObj.oAC220_In.fdQ = 0;
			g_oADCSearchObj.oAC220_Out[0].fdQ = 0;
			g_oADCSearchObj.oAC220_Out[1].fdQ = 0;
			printf("恢复出厂设置完成！\r\n");
		}
		else																	//不需要恢复出厂设置
		{
			printf("~~~~~~~~没有复位标志位！！");

			AT24CXX_Read(0, (u8*)gl_LampConfigObj.ControllerID, 10);
			AT24CXX_Read(10, (u8*)gl_LampConfigObj.ServerIP, 30);
			AT24CXX_Read(40, (u8*)(&gl_LampConfigObj.ServerPort), 2);
			AT24CXX_Read(43, (u8*)(&(g_oADCSearchObj.oAC220_In.fdQ)), 4);
			AT24CXX_Read(47, (u8*)(&(g_oADCSearchObj.oAC220_Out[0].fdQ)), 4);
			AT24CXX_Read(51, (u8*)(&(g_oADCSearchObj.oAC220_Out[1].fdQ)), 4);
		}
		printf("ControllerID:%s\r\n", gl_LampConfigObj.ControllerID);
		printf("ServerIP:%s\r\n", gl_LampConfigObj.ServerIP);
		printf("ServerPort:%d\r\n", gl_LampConfigObj.ServerPort);
		return 0;
	}		   
	else																		//排除第一次初始化的情况
	{
		printf("EEPROM空！\r\n");
		AT24CXX_Write(0, (u8*)gl_LampConfigObj.ControllerID, 10);
		AT24CXX_Write(10, (u8*)gl_LampConfigObj.ServerIP, 20);
		AT24CXX_Write(40, (u8*)(&gl_LampConfigObj.ServerPort), 2);
		AT24CXX_WriteOneByte(at24cxx_type,0X55);
		temp=AT24CXX_ReadOneByte(at24cxx_type);	  
		if(temp==0X55)
		{
			printf("ControllerID:%s\r\n", gl_LampConfigObj.ControllerID);
			printf("ServerIP:%s\r\n", gl_LampConfigObj.ServerIP);
			printf("ServerPort:%d\r\n", gl_LampConfigObj.ServerPort);
			return 0;
		}
	}
	return 1;											  
}

//在AT24CXX里面的指定地址开始读出指定个数的数据
//ReadAddr :开始读出的地址 对24c02为0~255
//pBuffer  :数据数组首地址
//NumToRead:要读出数据的个数
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead)
{
	while(NumToRead)
	{
		*pBuffer++=AT24CXX_ReadOneByte(ReadAddr++);	
		NumToRead--;
	}
}  
//在AT24CXX里面的指定地址开始写入指定个数的数据
//WriteAddr :开始写入的地址 对24c02为0~255
//pBuffer   :数据数组首地址
//NumToWrite:要写入数据的个数
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite)
{
	while(NumToWrite--)
	{
		AT24CXX_WriteOneByte(WriteAddr,*pBuffer);
		WriteAddr++;
		pBuffer++;
	}
}

//字符串转十六进制（“123F”->0x123F）
//strSrc：要转换的字符串
//strDest：转换完存放的Buf
void StrToHexArray( unsigned char* strSrc, unsigned char* strDest )
{
	int nDone = 0;
	unsigned char value = 0;
	
//	if ( strSrc == NULL || strDest == NULL )
//	return NULL;

	while( *strSrc )
	{
		nDone++;
		value = *strSrc - '0';
		if( value <= 9 )
		{/*value = value;*/}
		else if( value >= 0x31 && value <= 0x36 )
		{value -= 0x27;}
		else if( value >= 0x11 && value <= 0x16 )
		{value -= 0x07;}

		if(( nDone % 2 ) == 1 )
		{
			value <<= 4;
			*strDest = value;
		}
		else if(( nDone % 2 ) == 0 )
		{
			*strDest += value;
			strDest++;
		}
		strSrc++;
	}
}

//十六进制串转字符串
//strDest:转成字符串后存放的buf
//strSrc:十六进制串buf
//len:要转换的长度
void HexArrayToStr(unsigned char* strDest, unsigned char* strSrc, unsigned int len)
{
	unsigned char a, b, i;

	for(i=0; i<len; i++)
	{
		a = strSrc[i]>>4;
		b = strSrc[i]&0x0F;
		if(a<0X0A)
		{
			strDest[2*i] = a+0x30;
		}
		else if((a>=0X0A)&&(a<=0X0F))
		{
			strDest[2*i] = a+0x37;
		}
		if(b<0X0A)
		{
			strDest[2*i+1] = b+0x30;
		}
		else if((b>=0X0A)&&(b<=0X0F))
		{
			strDest[2*i+1] = b+0x37;
		}
	}
	strDest[2*len] = 0x00;
}
//不支持A~F的字符串转十六进制（“0x1234”->0x1234）
// int ConvertStrToHex(char* str,char* buf)
// {
//     int iRet = -1;
//     int i = 0;
//     if(str == NULL ||  buf == NULL)
//     {
//       goto END;
//     }
//     if(strlen(str)%2 != 0)
//     {
//       goto END;
//     }
//     for(i = 0; i < strlen(str)/2; i++)
//     {
//        	buf[i] = ((str[2*i] - 0x30)<<4) | (str[2*i+1] - 0x30);
//     }
//     iRet = 0;
//     END:
//     return iRet;
// }

void AT24CXX_ServiceFunc(void)
{
	u8 tempflag;

	tempflag = EEPROM_Init(AT24C04);
	if(tempflag)
	{
		DEBUG("AT24C04 check erro!!");
	}
	else
	{
		DEBUG("AT24C04 check good!!");
	}
	// DEBUG("读取存储的ID、IP、PORT...\r\n");
	// AT24CXX_Read(0, IDBuf, 10);
	// AT24CXX_Read(10, IPBuf, 16);
	// AT24CXX_Read(26, PORTBuf, 6);
	// DEBUG("ID:%s\r\n", IDBuf);
	// StrToHexArray((unsigned char*)IDBuf, (unsigned char*)i);
	// DEBUG("i:");
	// for(j=0;j<4;j++)
	// {
	// DEBUG("%02X|", ((char*)i)[j]);
	// }
	// DEBUG("\r\n");
	// DEBUG("IP:%s\r\n", IPBuf);
	// DEBUG("PORT:%s\r\n", PORTBuf);
}










