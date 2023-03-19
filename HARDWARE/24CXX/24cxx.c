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

//��ʼ��IIC�ӿ�
void AT24CXX_Init(void)
{
	IIC_Init();
	DEBUG("AT24CXX_Init��ɣ�\r\n");
}
//��AT24CXXָ����ַ����һ������
//ReadAddr:��ʼ�����ĵ�ַ  
//����ֵ  :����������
u8 AT24CXX_ReadOneByte(u16 ReadAddr)
{				  
	u8 temp=0;		  	    																 
	IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(0XA0);	   //����д����
		IIC_Wait_Ack();
		IIC_Send_Byte(ReadAddr>>8);//���͸ߵ�ַ
		IIC_Wait_Ack();		 
	}
	else 
	{
		IIC_Send_Byte(0XA0+((ReadAddr/256)<<1));   //����������ַ0XA0,д���� 	 
	}
		IIC_Wait_Ack(); 
		IIC_Send_Byte(ReadAddr%256);   //���͵͵�ַ
		IIC_Wait_Ack();	    
		IIC_Start();  	 	   
		IIC_Send_Byte(0XA1);           //�������ģʽ			   
		IIC_Wait_Ack();	 
		temp=IIC_Read_Byte(0);		   
		IIC_Stop();//����һ��ֹͣ����	    
	return temp;
}
//��AT24CXXָ����ַд��һ������
//WriteAddr  :д�����ݵ�Ŀ�ĵ�ַ    
//DataToWrite:Ҫд�������
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite)
{				   	  	    																 
	IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(0XA0);	    //����д����
		IIC_Wait_Ack();
		IIC_Send_Byte(WriteAddr>>8);//���͸ߵ�ַ
 	}
	else
	{
		IIC_Send_Byte(0XA0+((WriteAddr/256)<<1));   //����������ַ0XA0,д���� 
	}	 
	IIC_Wait_Ack();	   
	IIC_Send_Byte(WriteAddr%256);   //���͵͵�ַ
	IIC_Wait_Ack(); 	 										  		   
	IIC_Send_Byte(DataToWrite);     //�����ֽ�							   
	IIC_Wait_Ack();  		    	   
	IIC_Stop();//����һ��ֹͣ���� 
	delay_ms(10);	 
}
//��AT24CXX�����ָ����ַ��ʼд�볤��ΪLen������
//�ú�������д��16bit����32bit������.
//WriteAddr  :��ʼд��ĵ�ַ  
//DataToWrite:���������׵�ַ
//Len        :Ҫд�����ݵĳ���2,4
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len)
{  	
	u8 t;
	for(t=0;t<Len;t++)
	{
		AT24CXX_WriteOneByte(WriteAddr+t,(DataToWrite>>(8*t))&0xff);
	}												    
}

//��AT24CXX�����ָ����ַ��ʼ��������ΪLen������
//�ú������ڶ���16bit����32bit������.
//ReadAddr   :��ʼ�����ĵ�ַ 
//����ֵ     :����
//Len        :Ҫ�������ݵĳ���2,4
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
//AT24CXX����
//WriteAddr:��ʼ�����ַ
//Clearnum:��Ҫ�������
void AT24CXX_Clear(u16 WriteAddr,u16 Clearnum)
{
	u16 i;
	for(i=0;i<Clearnum;i++)
	{
		AT24CXX_WriteOneByte(WriteAddr+i,0);
	}
}

//���AT24CXX�Ƿ�����
//��������24XX�����һ����ַ(255)���洢��־��.
//���������24Cϵ��,�����ַҪ�޸�
//����1:���ʧ��
//����0:���ɹ�
u8 EEPROM_Init(u16 at24cxx_type)
{
	u8 temp;

	AT24CXX_Clear(0, 512);
	temp=AT24CXX_ReadOneByte(at24cxx_type);										//����ÿ�ο�����дAT24CXX			   
	if(temp==0X55)
	{
		printf("EEPROM�����ݣ���ȡ�����ñ�����......\r\n");
		AT24CXX_Read(42, &gl_LampConfigObj.RestoreFlag, 1);
		if(gl_LampConfigObj.RestoreFlag == 1)									//�ָ���������
		{
			printf("!!!!!!!!!�и�λ��־λ����");
			gl_LampConfigObj.RestoreFlag = 0;
			AT24CXX_Write(42, 0, 1);											//��ָ��������ñ�־
			strcpy((char*)gl_LampConfigObj.ControllerID, "00001333");	
			strcpy((char*)gl_LampConfigObj.ServerIP, "smartpole.bethlabs.com");    //""39.98.57.97
			gl_LampConfigObj.ServerPort = 7900;
			g_oADCSearchObj.oAC220_In.fdQ = 0;
			g_oADCSearchObj.oAC220_Out[0].fdQ = 0;
			g_oADCSearchObj.oAC220_Out[1].fdQ = 0;
			printf("�ָ�����������ɣ�\r\n");
		}
		else																	//����Ҫ�ָ���������
		{
			printf("~~~~~~~~û�и�λ��־λ����");

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
	else																		//�ų���һ�γ�ʼ�������
	{
		printf("EEPROM�գ�\r\n");
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

//��AT24CXX�����ָ����ַ��ʼ����ָ������������
//ReadAddr :��ʼ�����ĵ�ַ ��24c02Ϊ0~255
//pBuffer  :���������׵�ַ
//NumToRead:Ҫ�������ݵĸ���
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead)
{
	while(NumToRead)
	{
		*pBuffer++=AT24CXX_ReadOneByte(ReadAddr++);	
		NumToRead--;
	}
}  
//��AT24CXX�����ָ����ַ��ʼд��ָ������������
//WriteAddr :��ʼд��ĵ�ַ ��24c02Ϊ0~255
//pBuffer   :���������׵�ַ
//NumToWrite:Ҫд�����ݵĸ���
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite)
{
	while(NumToWrite--)
	{
		AT24CXX_WriteOneByte(WriteAddr,*pBuffer);
		WriteAddr++;
		pBuffer++;
	}
}

//�ַ���תʮ�����ƣ���123F��->0x123F��
//strSrc��Ҫת�����ַ���
//strDest��ת�����ŵ�Buf
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

//ʮ�����ƴ�ת�ַ���
//strDest:ת���ַ������ŵ�buf
//strSrc:ʮ�����ƴ�buf
//len:Ҫת���ĳ���
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
//��֧��A~F���ַ���תʮ�����ƣ���0x1234��->0x1234��
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
	// DEBUG("��ȡ�洢��ID��IP��PORT...\r\n");
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










