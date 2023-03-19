#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "cmddeal.h"
#include "CRC.h"
#include "dma.h"
#include "ec600m.h"
#include "DefBean.h"
#include "netcommutask.h"
#include "timer.h"
#include "24cxx.h"
#include "hlw8112.h"
#include "rtc.h"
#include "relay.h"
#include "led.h"


CMDHeadObj          gl_CMDHeadObj;
ConfigCmdObj        gl_ConfigCmdObj;
LampCtrlCmdObj      gl_LampCtrlCmdObj;
LampConfigObj       gl_LampConfigObj = {"00001333", "smartpole.bethlabs.com", 7900};   //temp.lilunxinli.com  39.98.57.97
LampStatusCmdObj 	  gl_LampStatusCmdObj;
RestoreCmdObj 		  gl_RestoreCmdObj;
TimeSynchronCmdObj 	gl_TimeSynchronCmdObj;       

// u32 gl_firstSerialNo = 1;                   //��ʼ��Ϣ���к�
u32 gl_recvSerialNo = 1;                    //�յ���Ϣ���к�

//������Ϣͷ����
//buf:����洢��
//type:��Ϣ����
//cmd:��ϢID
//ctrid:·�ƿ�����ID
//ledid:�ƾ�ID
//serial:��Ϣ��ˮ��
void PackMsgHead(char* buf,u8 type,u16 cmd,u8* pctrid,u32 ledid,u32* serial)
{
  u8 ctrid[4] = {0};
  
  StrToHexArray( (unsigned char*) pctrid, (unsigned char*)ctrid );
  ((CMDHeadObj*)buf)->CMDType = type;                         
  ((CMDHeadObj*)buf)->CMDSerialnum = __REV(*serial);
  ((CMDHeadObj*)buf)->CMDLength = 10;                             
  ((CMDHeadObj*)buf)->CMDID = __REV16(cmd);
  ((CMDHeadObj*)buf)->ControllerID = *((u32*)ctrid);
  ((CMDHeadObj*)buf)->luminiareID = __REV(ledid);
  gl_recvSerialNo = *serial;
}

//����Ϣ������ַ���
//buf:����洢��
//type:��������
//data:����ֵ(�ַ���)
void PackAddStringParam(char* buf,u16 type,u8* data)
{
   u16 len = ((CMDHeadObj*)buf)->CMDLength;

   StringParamObj* pParam = (StringParamObj*)(buf + len + CMD_HEAD_LEN);
   pParam->ParamType = __REV16(type);
   strcpy((char*)pParam->ParamValue,(const char*)data);
   ((CMDHeadObj*)buf)->CMDLength += (strlen((const char*)data) + 3); 
}

//����Ϣͷ��CRCУ��ֵ
//buf:������buffer
void PackMsgCrc(char* buf)
{
  u16 len = ((CMDHeadObj*)buf)->CMDLength ;
  ((CMDHeadObj*)buf)->CRC16 = CRC16_MODBUS((unsigned char*)buf + 13,len);
  ((CMDHeadObj*)buf)->CMDLength = __REV16(len);
}

//������Ӧ����
void CmdResponse(int socketid, u32 serialnum)
{
  char *pchBuf;
  u8 i;
  pchBuf = pvPortMalloc(CMD_HEAD_LEN);
  memset(pchBuf, 0, CMD_HEAD_LEN);
  ((CMDHeadObj*)pchBuf)->CMDType = CMD_RESPONSE;
  ((CMDHeadObj*)pchBuf)->CMDSerialnum = __REV(serialnum);
  DEBUG("����ظ���");
  for(i=0;i<CMD_HEAD_LEN;i++)
  {
    DEBUG("%02X|", pchBuf[i]);
  }
  DEBUG("\r\n");
  EC600M_Send(socketid, pchBuf, CMD_HEAD_LEN); 
  vPortFree(pchBuf);
}

//��������ظ�
//serial:�����·��������Ϣ��ˮ��
//cmd����ϢID
//ctrid:������ID
//ledid:�ƾ�ID
//pramatype:��������
//string:����ֵ
void CmdDealResult(int socketid, u32 serialnum, u16 cmdid, u32 ctrlid, u32 lampid, u8* data)
{
    char *pchBuf;
    u8 i;

    pchBuf = pvPortMalloc(CMD_RESULT_LEN);
    memset(pchBuf, 0, CMD_RESULT_LEN);
    ((CMDHeadObj*)pchBuf)->CMDType = CMD_DEALREPLY;
    ((CMDHeadObj*)pchBuf)->CMDSerialnum = __REV(serialnum);
    ((CMDHeadObj*)pchBuf)->CMDLength =__REV16(0x000F);
    ((CMDHeadObj*)pchBuf)->CMDID = __REV16(cmdid);
    ((CMDHeadObj*)pchBuf)->ControllerID = __REV(ctrlid);
    ((CMDHeadObj*)pchBuf)->luminiareID = __REV(lampid);
    strcpy(pchBuf+23, (const char*)data);
    ((CMDHeadObj*)pchBuf)->CRC16 = CRC16_MODBUS((unsigned char*)pchBuf + 13,10+strlen((const char*)data));
    DEBUG("���������");
    for(i=0;i<CMD_RESULT_LEN;i++)
    {
      DEBUG("%02X|", pchBuf[i]);
    }
    DEBUG("\r\n");
    EC600M_Send(socketid, pchBuf, CMD_RESULT_LEN);
    vPortFree(pchBuf); 
}

//��֯�������ݲ�����
void LampStatusQueryResult(int socketid, u32 serialnum, u16 cmdid, u32 ctrlid, u32 lampid, u8* voltage, u8* current, u8* power, u8* energy, 
                           u8* factor, u8* temprature, u8* csq, u8* status, u8* version, u8* serverip, u8* serverport, u8* lamp1_lux, u8* lamp2_lux)
{
    u8* pchBuf=NULL;
    u16 paramlength = 0;
    u8 i;

    paramlength = 38+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)+strlen((const char*)factor)
                  +strlen((const char*)temprature)+strlen((const char*)csq)+strlen((const char*)status)+strlen((const char*)version)+strlen((const char*)serverip)
                  +strlen((const char*)serverport)+strlen((const char*)lamp1_lux)+strlen((const char*)lamp2_lux);

    pchBuf = pvPortMalloc(paramlength);
    memset(pchBuf, 0, paramlength);
    ((CMDHeadObj*)pchBuf)->CMDType = CMD_DEALREPLY;
    ((CMDHeadObj*)pchBuf)->CMDSerialnum = __REV(serialnum);
    ((CMDHeadObj*)pchBuf)->CMDLength =__REV16(paramlength - 13);
    ((CMDHeadObj*)pchBuf)->CMDID = __REV16(cmdid);
    ((CMDHeadObj*)pchBuf)->ControllerID = __REV(ctrlid);
    ((CMDHeadObj*)pchBuf)->luminiareID = __REV(lampid);   
    ((StringParamObj*)(pchBuf+23))->ParamType = __REV16(RESOURCEVALUE);
    strcpy((char*)pchBuf+25, (const char*)voltage);
    strcpy((char*)pchBuf+26+strlen((const char*)voltage), (const char*)current);
    strcpy((char*)pchBuf+27+strlen((const char*)voltage)+strlen((const char*)current), (const char*)power);
    strcpy((char*)pchBuf+28+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power), (const char*)energy);
    strcpy((char*)pchBuf+29+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy),
          (const char*)factor);
    strcpy((char*)pchBuf+30+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)
          +strlen((const char*)factor), (const char*)temprature);
    strcpy((char*)pchBuf+31+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)
          +strlen((const char*)factor)+strlen((const char*)temprature), (const char*)csq);
    strcpy((char*)pchBuf+32+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)
          +strlen((const char*)factor)+strlen((const char*)temprature)+strlen((const char*)csq), (const char*)status);
    strcpy((char*)pchBuf+33+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)
          +strlen((const char*)factor)+strlen((const char*)temprature)+strlen((const char*)csq)+strlen((const char*)status), (const char*)version);
    strcpy((char*)pchBuf+34+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)
          +strlen((const char*)factor)+strlen((const char*)temprature)+strlen((const char*)csq)+strlen((const char*)status)+strlen((const char*)version), (const char*)serverip);   
    strcpy((char*)pchBuf+35+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)
          +strlen((const char*)factor)+strlen((const char*)temprature)+strlen((const char*)csq)+strlen((const char*)status)+strlen((const char*)version)+strlen((const char*)serverip), (const char*)serverport);
    strcpy((char*)pchBuf+36+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)
          +strlen((const char*)factor)+strlen((const char*)temprature)+strlen((const char*)csq)+strlen((const char*)status)+strlen((const char*)version)+strlen((const char*)serverip)+strlen((const char*)serverport), (const char*)lamp1_lux);
    strcpy((char*)pchBuf+37+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)
          +strlen((const char*)factor)+strlen((const char*)temprature)+strlen((const char*)csq)+strlen((const char*)status)+strlen((const char*)version)+strlen((const char*)serverip)+strlen((const char*)serverport)+strlen((const char*)lamp1_lux), (const char*)lamp2_lux);

    ((CMDHeadObj*)pchBuf)->CRC16 = CRC16_MODBUS((unsigned char*)pchBuf + 13, paramlength-13);
    printf("��״̬��ѯ�ظ���");
    printf("%s|", pchBuf+25);
    printf("%s|", pchBuf+26+strlen((const char*)voltage));
    printf("%s|", pchBuf+27+strlen((const char*)voltage)+strlen((const char*)current));
    printf("%s|", pchBuf+28+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power));
    printf("%s|", pchBuf+29+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy));
    printf("%s|", pchBuf+30+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)+strlen((const char*)factor));
    printf("%s|", pchBuf+31+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)+strlen((const char*)factor)+strlen((const char*)temprature));
    printf("%s|", pchBuf+32+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)+strlen((const char*)factor)+strlen((const char*)temprature)
          +strlen((const char*)csq));
    printf("%s|", pchBuf+33+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)+strlen((const char*)factor)+strlen((const char*)temprature)
          +strlen((const char*)csq)+strlen((const char*)status));
    printf("%s|", pchBuf+34+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)+strlen((const char*)factor)+strlen((const char*)temprature)
          +strlen((const char*)csq)+strlen((const char*)status)+strlen((const char*)version));
    printf("%s|", pchBuf+35+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)+strlen((const char*)factor)+strlen((const char*)temprature)
          +strlen((const char*)csq)+strlen((const char*)status)+strlen((const char*)version)+strlen((const char*)serverip));
    printf("%s|", pchBuf+36+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)+strlen((const char*)factor)+strlen((const char*)temprature)
          +strlen((const char*)csq)+strlen((const char*)status)+strlen((const char*)version)+strlen((const char*)serverip)+strlen((const char*)serverport));
    printf("%s|\r\n", pchBuf+37+strlen((const char*)voltage)+strlen((const char*)current)+strlen((const char*)power)+strlen((const char*)energy)+strlen((const char*)factor)+strlen((const char*)temprature)
          +strlen((const char*)csq)+strlen((const char*)status)+strlen((const char*)version)+strlen((const char*)serverip)+strlen((const char*)serverport)+strlen((const char*)lamp1_lux));

    EC600M_Send(socketid, (char*)pchBuf, paramlength);
    printf("��ѯ�Ĳ���ֵ��");
    for(i=0; i<paramlength; i++)
    {
      printf("%02X|", pchBuf[i]);
    }
    printf("\r\n");
    g_oADCSearchObj.chStatus &= ~(0x01 << 2);
    LED4 = 1;
    vPortFree(pchBuf);  
}

//���������������в���ֵ����
//buf:������������
//len:��������������Ϣ�峤��
//pos:������ɵ���Ϣ�峤��
//param:�������ŵĽṹ�����
int UnPackStringParam(char* buf,int len,int pos,StringParamObj* param)
{
  int iRet = 0;
  if((pos + CMD_HEAD_LEN) >= len)
  {
    goto END;
  }
  param->ParamType = ((StringParamObj*)(buf + CMD_HEAD_LEN + pos))->ParamType;
  strcpy((char*)param->ParamValue,(const char*)((StringParamObj*)(buf + CMD_HEAD_LEN + pos))->ParamValue);
  iRet = pos + strlen((const char*)param->ParamValue) + 3;             //��������Ϣ��ĳ���
  DEBUG("���������Ϣ�峤�ȣ�%d\r\n", strlen((const char*)param->ParamValue));

  END:
  return iRet;
}

//���ݽ�������
//pdatabuf��Ҫ����������
void UnPackCmdParam(char* pdatabuf)
{
  u8 i;
  u8 j=0;
  u16 temp=10;
  // u8 lux = 80;      //�ն�
  u8 voltage[10] = {0};
  u8 current[10] = {0};
  u8 power[10] = {0};
  u8 energy[10] = {0};
  u8 factor[10] = {0};
  u8 temprature[10] = {0};
  u8 csq[5] = {0};
  u8 status[5] = {0};
  u8 version[10] = {0};
  u8 serverip[30] = {0};
  u8 serverport[5] = {0};
  u8 lamp1_dimval[5] = {0};
  u8 lamp2_dimval[5] = {0};
  u8 contrlid[9] = {0};
  u32 id_controller = 0;
  EventBits_t elecenergy_EventValue;    //�����¼���־ֵ


  if(((CMDHeadObj*)pdatabuf)->CMDType == 0x01)              //�����·��������������������ѯ�ƾ�״̬
  {                                                         //ʱ��ͬ������ָ�������������
    DEBUG("�յ������������ݸ�����%d\r\n",__REV16(((CMDHeadObj*)pdatabuf)->CMDLength));
	  DEBUG("�յ�����ID:%02X\r\n", __REV16(((CMDHeadObj*)pdatabuf)->CMDID));
    DEBUG("�յ���CRC:%04X\r\n", ((CMDHeadObj*)pdatabuf)->CRC16);
    DEBUG("�����CRC:%04X\r\n", CRC16_MODBUS((unsigned char*)(pdatabuf+13), __REV16(((CMDHeadObj*)pdatabuf)->CMDLength)));
    if(((CMDHeadObj*)pdatabuf)->CRC16 == CRC16_MODBUS((unsigned char*)(pdatabuf+13), __REV16(((CMDHeadObj*)pdatabuf)->CMDLength)))  //У��CRC
    {
      DEBUG("CRCУ��ͨ����\r\n");
      HexArrayToStr((unsigned char*)contrlid, (unsigned char*)(&(((CMDHeadObj*)pdatabuf)->ControllerID)), 4);  
      if(strcmp((const char*)contrlid, (const char*)gl_LampConfigObj.ControllerID) == 0)                        //�ж������Ƿ��Ƿ����Լ�
      {
        switch (__REV16(((CMDHeadObj*)pdatabuf)->CMDID))
        {
            case 0x1001:            //��������
              DEBUG("�յ�������������ݣ�");
              for(i=0; i<(__REV16(((CMDHeadObj*)pdatabuf)->CMDLength)+CMD_HEAD_LEN); i++)
              {
                DEBUG("%02X|", pdatabuf[i]);
              }
              DEBUG("\r\n");
              gl_ConfigCmdObj.ConfigHead.CMDSerialnum = __REV(((CMDHeadObj*)pdatabuf)->CMDSerialnum);           //�������л�ȡ��Ϣ��ˮ��
              DEBUG("�յ�����ˮ�ţ�%04X\r\n", __REV(((CMDHeadObj*)pdatabuf)->CMDSerialnum));
              gl_ConfigCmdObj.ConfigHead.CMDLength = __REV16(((CMDHeadObj*)pdatabuf)->CMDLength);               //�������л�ȡ��Ϣ�峤��
              gl_ConfigCmdObj.ConfigHead.ControllerID = __REV(((CMDHeadObj*)pdatabuf)->ControllerID);           //�������л�ȡ������ID
              while((temp = UnPackStringParam(pdatabuf,(gl_ConfigCmdObj.ConfigHead.CMDLength + CMD_HEAD_LEN),temp,&(gl_ConfigCmdObj.ConfigParam[j]))) != 0)
              {
                  DEBUG("��Ϣ�峤��:%d\r\n", gl_ConfigCmdObj.ConfigHead.CMDLength);
                  DEBUG("tempֵ:%d\r\n", temp);
                  j++;
              }
              for(i=0; i<j; i++)    //����
              {
                DEBUG("����������%d����������:%04X\r\n", i+1,__REV16((&(gl_ConfigCmdObj.ConfigParam[i]))->ParamType));
                DEBUG("����������%d������ֵ:%s\r\n", i+1, (&(gl_ConfigCmdObj.ConfigParam[i]))->ParamValue);
              }
              memset(gl_CMDtaskObj.CmdRecvBuf, 0, EC600M_RX_BUF);          //�������������������ջ���
              if(gl_IcapDevObj.socketid == NET_SOCKET)
              {
                  CmdResponse(NET_SOCKET, gl_ConfigCmdObj.ConfigHead.CMDSerialnum);         //����������Ӧ
              }
              else
              {
                  CmdResponse(BG_SOCKET, gl_ConfigCmdObj.ConfigHead.CMDSerialnum);
              }
              for(i=0; i<j; i++)                                            //�洢����ֵ
              {
                switch (__REV16((&(gl_ConfigCmdObj.ConfigParam[i]))->ParamType))
                {
                  case 1:
                    strcpy((char*)(gl_LampConfigObj.ControllerID), (const char *)((&(gl_ConfigCmdObj.ConfigParam[i]))->ParamValue));
                    DEBUG("�¿�����ID:%s׼���洢......\r\n", gl_LampConfigObj.ControllerID);
                    xEventGroupSetBits(gl_IcapDevObj.oEventGroupHandler, EVENT_CTRLID);         //�����¼���־λ
                    break;
                  case 2:
                    strcpy((char*)(gl_LampConfigObj.ServerIP), (const char *)((&(gl_ConfigCmdObj.ConfigParam[i]))->ParamValue));
                    DEBUG("�·�����IP:%s׼���洢......\r\n", gl_LampConfigObj.ServerIP);
                    xEventGroupSetBits(gl_IcapDevObj.oEventGroupHandler, EVENT_SERVERIP);
                    break;
                  case 3:
                    strcpy((char*)(gl_LampConfigObj.ServerPort), (const char *)((&(gl_ConfigCmdObj.ConfigParam[i]))->ParamValue));
                    DEBUG("�¶˿ں�:%d׼���洢......\r\n", gl_LampConfigObj.ServerPort);
                    xEventGroupSetBits(gl_IcapDevObj.oEventGroupHandler, EVENT_SERVERPORT);
                    break;                
                  default:
                    DEBUG("���ò�������\r\n");
                    break;
                }
              }
              StrToHexArray(gl_LampConfigObj.ControllerID, (unsigned char*)&id_controller);
              if(gl_IcapDevObj.socketid == NET_SOCKET)
              {
                CmdDealResult(NET_SOCKET, gl_ConfigCmdObj.ConfigHead.CMDSerialnum, CMD_Config, __REV(id_controller), 0, (u8*)"0000");
              }
              else
              {
                CmdDealResult(BG_SOCKET, gl_ConfigCmdObj.ConfigHead.CMDSerialnum, CMD_Config, __REV(id_controller), 0, (u8*)"0000");
              }
              break;
            case 0x1208:        //�ƿ�����
                DEBUG("�յ��ƿ���������ݣ�");
                for(i=0; i<(__REV16(((CMDHeadObj*)pdatabuf)->CMDLength)+CMD_HEAD_LEN); i++)
                {
                  DEBUG("%02X|", pdatabuf[i]);
                }
                DEBUG("\r\n");
                gl_LampCtrlCmdObj.LampCtrlHead.CMDSerialnum = __REV(((CMDHeadObj*)pdatabuf)->CMDSerialnum);
                gl_LampCtrlCmdObj.LampCtrlHead.CMDLength = __REV16(((CMDHeadObj*)pdatabuf)->CMDLength);
                gl_LampCtrlCmdObj.LampCtrlHead.ControllerID = __REV(((CMDHeadObj*)pdatabuf)->ControllerID);
                gl_LampCtrlCmdObj.LampCtrlHead.luminiareID = __REV(((CMDHeadObj*)pdatabuf)->luminiareID);
                while((temp = UnPackStringParam(pdatabuf,(gl_LampCtrlCmdObj.LampCtrlHead.CMDLength + CMD_HEAD_LEN),temp,&(gl_LampCtrlCmdObj.LampCtrlParam))) != 0)
                {
                    DEBUG("tempֵ:%d\r\n", temp);
                    j++;
                }
                DEBUG("j��ֵ��%d\r\n", j);
                DEBUG("����������%d����������:%04X\r\n", j,__REV16((&(gl_LampCtrlCmdObj.LampCtrlParam))->ParamType));
                DEBUG("����������%d������ֵ:%s\r\n", j, (&(gl_LampCtrlCmdObj.LampCtrlParam))->ParamValue);
                DEBUG("��������ĵƾ�ID:%d\r\n", gl_LampCtrlCmdObj.LampCtrlHead.luminiareID);
                switch (__REV(((CMDHeadObj*)pdatabuf)->luminiareID))
                {
                  case 1:
                    if(__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x2C)          //��1 ��
                    {
                        LampControl(LAMP_1, LAMP_OPEN);
                        LED_LAMP1 = LED_ON;
                        xEventGroupSetBits(gl_IcapDevObj.ohlw8112_EventHandler, EVENT_FIRSTLAMP);
                        DEBUG("��1��!\r\n");
                    }
                    else if (__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x2D)    //��1 ��
                    {
                        LampControl(LAMP_1, LAMP_CLOSE);
                        LED_LAMP1 = LED_OFF;
                        xEventGroupClearBits(gl_IcapDevObj.ohlw8112_EventHandler, EVENT_FIRSTLAMP);
                        DEBUG("��1��!\r\n");            
                    }
                    else if (__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x21)    //��1����
                    {
                        DEBUG("��1����!\r\n");
                        gl_LampConfigObj.Lamp1_dimval = atoi((const char*)gl_LampCtrlCmdObj.LampCtrlParam.ParamValue);
                        PWM_Servicefunc(LAMP_1, gl_LampConfigObj.Lamp1_dimval);
                        DEBUG("��1�նȵ���:%d\r\n", gl_LampConfigObj.Lamp1_dimval);
                    }
                    break;
                  case 2:
                    if(__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x2C)          //��2 ��
                    {
                        LampControl(LAMP_2, LAMP_OPEN);
                        LED_LAMP2 = LED_ON;
                        xEventGroupSetBits(gl_IcapDevObj.ohlw8112_EventHandler, EVENT_SECONDLAMP);
                        DEBUG("��2��!\r\n");
                    }
                    else if (__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x2D)    //��2 ��
                    {
                        LampControl(LAMP_2, LAMP_CLOSE);
                        LED_LAMP2 = LED_OFF;
                        xEventGroupClearBits(gl_IcapDevObj.ohlw8112_EventHandler, EVENT_SECONDLAMP);
                        DEBUG("��2��!\r\n");               
                    }
                    else if (__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x21)    //��2����
                    {
                        gl_LampConfigObj.Lamp2_dimval = atoi((const char*)gl_LampCtrlCmdObj.LampCtrlParam.ParamValue);
                        PWM_Servicefunc(LAMP_2, gl_LampConfigObj.Lamp2_dimval);
                        DEBUG("��2�նȵ���:%d\r\n", gl_LampConfigObj.Lamp2_dimval);
                    }
                    break;
                  default:
                    DEBUG("·��ID����!\r\n");
                    DEBUG("����ĵƾ�ID:%d\r\n", __REV(((CMDHeadObj*)pdatabuf)->luminiareID));
                    break;
                }
                if(gl_IcapDevObj.socketid == NET_SOCKET)
                {
                  CmdResponse(NET_SOCKET, gl_LampCtrlCmdObj.LampCtrlHead.CMDSerialnum);
                  CmdDealResult(NET_SOCKET, gl_LampCtrlCmdObj.LampCtrlHead.CMDSerialnum, CMD_LampControl, gl_LampCtrlCmdObj.LampCtrlHead.ControllerID,\
                                gl_LampCtrlCmdObj.LampCtrlHead.luminiareID, (u8*)"0000");
                }
                else
                {
                  CmdResponse(BG_SOCKET, gl_LampCtrlCmdObj.LampCtrlHead.CMDSerialnum);
                  CmdDealResult(BG_SOCKET, gl_LampCtrlCmdObj.LampCtrlHead.CMDSerialnum, CMD_LampControl, gl_LampCtrlCmdObj.LampCtrlHead.ControllerID,\
                                gl_LampCtrlCmdObj.LampCtrlHead.luminiareID, (u8*)"0000");
                }
                break;
            case 0x1209:        //״̬��ѯ����
                DEBUG("�յ�·��״̬��ѯ��������ݣ�");
                for(i=0; i<__REV16(((CMDHeadObj*)pdatabuf)->CMDLength)+CMD_HEAD_LEN; i++)
                {
                  DEBUG("%02X|", pdatabuf[i]);
                }
                DEBUG("\r\n");
                gl_LampStatusCmdObj.LampStatusHead.CMDSerialnum = __REV(((CMDHeadObj*)pdatabuf)->CMDSerialnum);
                gl_LampStatusCmdObj.LampStatusHead.CMDLength = __REV16(((CMDHeadObj*)pdatabuf)->CMDLength);
                gl_LampStatusCmdObj.LampStatusHead.ControllerID = __REV(((CMDHeadObj*)pdatabuf)->ControllerID);
                gl_LampStatusCmdObj.LampStatusHead.luminiareID = __REV(((CMDHeadObj*)pdatabuf)->luminiareID);
                if(gl_IcapDevObj.socketid == NET_SOCKET)
                {
                  CmdResponse(NET_SOCKET, gl_LampStatusCmdObj.LampStatusHead.CMDSerialnum);
                }
                else
                {
                  CmdResponse(BG_SOCKET, gl_LampStatusCmdObj.LampStatusHead.CMDSerialnum);
                }
                switch (__REV(((CMDHeadObj*)pdatabuf)->luminiareID))
                {
                  case 0:       //����������
                    elecenergy_EventValue = xEventGroupGetBits(gl_IcapDevObj.ohlw8112_EventHandler);
                    elecenergy_EventValue &= 0x03;
                    if (elecenergy_EventValue == 0x03)
                    {
                      g_oADCSearchObj.oAC220_In.fdA = g_oADCSearchObj.oAC220_In.fdA - g_oADCSearchObj.oAC220_Out[0].fdA
                                                      - g_oADCSearchObj.oAC220_Out[1].fdA;
                      g_oADCSearchObj.oAC220_In.fdP = g_oADCSearchObj.oAC220_In.fdP - g_oADCSearchObj.oAC220_Out[0].fdP 
                                                      - g_oADCSearchObj.oAC220_Out[1].fdP; 
                      g_oADCSearchObj.oAC220_In.fdQ = g_oADCSearchObj.oAC220_In.fdQ - g_oADCSearchObj.oAC220_Out[0].fdQ
                                                      - g_oADCSearchObj.oAC220_Out[1].fdQ;                                                       
                    }
                    else if (elecenergy_EventValue == 0x01)
                    {
                      g_oADCSearchObj.oAC220_In.fdA = g_oADCSearchObj.oAC220_In.fdA - g_oADCSearchObj.oAC220_Out[0].fdA;
                      g_oADCSearchObj.oAC220_In.fdP = g_oADCSearchObj.oAC220_In.fdP - g_oADCSearchObj.oAC220_Out[0].fdP;
                      g_oADCSearchObj.oAC220_In.fdQ = g_oADCSearchObj.oAC220_In.fdQ - g_oADCSearchObj.oAC220_Out[0].fdQ;                 
                    }
                    else if (elecenergy_EventValue == 0x02)
                    {
                      g_oADCSearchObj.oAC220_In.fdA = g_oADCSearchObj.oAC220_In.fdA - g_oADCSearchObj.oAC220_Out[1].fdA;
                      g_oADCSearchObj.oAC220_In.fdP = g_oADCSearchObj.oAC220_In.fdP - g_oADCSearchObj.oAC220_Out[1].fdP;
                      g_oADCSearchObj.oAC220_In.fdQ = g_oADCSearchObj.oAC220_In.fdQ - g_oADCSearchObj.oAC220_Out[1].fdQ;                       
                    }         //u8* version, u8* serverip, u8* serverport, u8* lamp1_lux, u8* lamp2_lux       gl_LampConfigObj.ServerIP,                                                          
                    sprintf((char*)voltage, "%.3f", g_oADCSearchObj.oAC220_In.fdV);
                    sprintf((char*)current, "%.3f", g_oADCSearchObj.oAC220_In.fdA);
                    sprintf((char*)power, "%.3f", g_oADCSearchObj.oAC220_In.fdP);
                    sprintf((char*)energy, "%.3f", g_oADCSearchObj.oAC220_In.fdQ);
                    sprintf((char*)factor, "%.3f", g_oADCSearchObj.oAC220_In.ffc);
                    sprintf((char*)temprature, "%.3f", g_oADCSearchObj.temprature); 
                    sprintf((char*)csq, "%d", g_oADCSearchObj.iCSQ);
                    sprintf((char*)status, "%d", g_oADCSearchObj.chStatus);
                    strcpy((char*)version, APPVERSION);
                    strcpy((char*)serverip, (const char*)gl_LampConfigObj.ServerIP);
                    sprintf((char*)serverport, "%d", gl_LampConfigObj.ServerPort);
                    sprintf((char*)lamp1_dimval, "%d", gl_LampConfigObj.Lamp1_dimval);
                    sprintf((char*)lamp2_dimval, "%d", gl_LampConfigObj.Lamp2_dimval);
                   
                    if(gl_IcapDevObj.socketid == NET_SOCKET)
                    {
                      LampStatusQueryResult(NET_SOCKET, gl_LampStatusCmdObj.LampStatusHead.CMDSerialnum, CMD_LampStatus, gl_LampStatusCmdObj.LampStatusHead.ControllerID, gl_LampStatusCmdObj.LampStatusHead.luminiareID,
                                             voltage, current, power, energy, factor, temprature, csq, status, version, serverip, serverport, lamp1_dimval, lamp2_dimval);
                    }
                    else
                    {
                      LampStatusQueryResult(BG_SOCKET, gl_LampStatusCmdObj.LampStatusHead.CMDSerialnum, CMD_LampStatus, gl_LampStatusCmdObj.LampStatusHead.ControllerID, gl_LampStatusCmdObj.LampStatusHead.luminiareID, 
                                            voltage, current, power, energy, factor, temprature, csq, status, version, serverip, serverport, lamp1_dimval, lamp2_dimval);
                    }
                    break;
                  case 1:
                    /*��֯�������ݲ�����*/
                    sprintf((char*)voltage, "%.3f", g_oADCSearchObj.oAC220_Out[0].fdV);
                    sprintf((char*)current, "%.3f", g_oADCSearchObj.oAC220_Out[0].fdA);
                    sprintf((char*)power, "%.3f", g_oADCSearchObj.oAC220_Out[0].fdP);
                    sprintf((char*)energy, "%.3f", g_oADCSearchObj.oAC220_Out[0].fdQ);
                    sprintf((char*)factor, "%.3f", g_oADCSearchObj.oAC220_Out[0].ffc);
                    sprintf((char*)temprature, "%.3f", g_oADCSearchObj.temprature);
					          sprintf((char*)csq, "%d", g_oADCSearchObj.iCSQ);
                    sprintf((char*)status, "%d", g_oADCSearchObj.chStatus);	
                    strcpy((char*)version, APPVERSION);
                    strcpy((char*)serverip, (const char*)gl_LampConfigObj.ServerIP);
                    sprintf((char*)serverport, "%d", gl_LampConfigObj.ServerPort);
                    sprintf((char*)lamp1_dimval, "%d", gl_LampConfigObj.Lamp1_dimval);
                    sprintf((char*)lamp2_dimval, "%d", gl_LampConfigObj.Lamp2_dimval);		  
                    if(gl_IcapDevObj.socketid == NET_SOCKET)
                    {
                      LampStatusQueryResult(NET_SOCKET, gl_LampStatusCmdObj.LampStatusHead.CMDSerialnum, CMD_LampStatus, gl_LampStatusCmdObj.LampStatusHead.ControllerID, gl_LampStatusCmdObj.LampStatusHead.luminiareID, 
                                            voltage, current, power, energy, factor, temprature, csq, status, version, serverip, serverport, lamp1_dimval, lamp2_dimval);
                    }
                    else
                    {
                      LampStatusQueryResult(BG_SOCKET, gl_LampStatusCmdObj.LampStatusHead.CMDSerialnum, CMD_LampStatus, gl_LampStatusCmdObj.LampStatusHead.ControllerID, gl_LampStatusCmdObj.LampStatusHead.luminiareID, 
                                            voltage, current, power, energy, factor, temprature, csq, status, version, serverip, serverport, lamp1_dimval, lamp2_dimval);
                    }
                    break;
                  case 2:
                    /*��֯�������ݲ�����*/
                    sprintf((char*)voltage, "%.3f", g_oADCSearchObj.oAC220_Out[1].fdV);
                    sprintf((char*)current, "%.3f", g_oADCSearchObj.oAC220_Out[1].fdA);
                    sprintf((char*)power, "%.3f", g_oADCSearchObj.oAC220_Out[1].fdP);
                    sprintf((char*)energy, "%.3f", g_oADCSearchObj.oAC220_Out[1].fdQ);
                    sprintf((char*)factor, "%.3f", g_oADCSearchObj.oAC220_Out[1].ffc);
                    sprintf((char*)temprature, "%.3f", g_oADCSearchObj.temprature);
					          sprintf((char*)csq, "%d", g_oADCSearchObj.iCSQ);
                    sprintf((char*)status, "%d", g_oADCSearchObj.chStatus);
                    strcpy((char*)version, APPVERSION);
                    strcpy((char*)serverip, (const char*)gl_LampConfigObj.ServerIP);
                    sprintf((char*)serverport, "%d", gl_LampConfigObj.ServerPort);
                    sprintf((char*)lamp1_dimval, "%d", gl_LampConfigObj.Lamp1_dimval);
                    sprintf((char*)lamp2_dimval, "%d", gl_LampConfigObj.Lamp2_dimval);
                    if(gl_IcapDevObj.socketid == NET_SOCKET)
                    {
                      LampStatusQueryResult(NET_SOCKET, gl_LampStatusCmdObj.LampStatusHead.CMDSerialnum, CMD_LampStatus, gl_LampStatusCmdObj.LampStatusHead.ControllerID, gl_LampStatusCmdObj.LampStatusHead.luminiareID, 
                                            voltage, current, power, energy, factor, temprature, csq, status, version, serverip, serverport, lamp1_dimval, lamp2_dimval);
                    }
                    else
                    {
                      LampStatusQueryResult(BG_SOCKET, gl_LampStatusCmdObj.LampStatusHead.CMDSerialnum, CMD_LampStatus, gl_LampStatusCmdObj.LampStatusHead.ControllerID, gl_LampStatusCmdObj.LampStatusHead.luminiareID, 
                                            voltage, current, power, energy, factor, temprature, csq, status, version, serverip, serverport, lamp1_dimval, lamp2_dimval);
                    }
                    break;
                }
                break;
            case 0x1211:        //�ָ���������
                DEBUG("�յ��������ָ�������������ݣ�");
                for(i=0; i<__REV16(((CMDHeadObj*)pdatabuf)->CMDLength)+CMD_HEAD_LEN; i++)
                {
                  DEBUG("%02X|", pdatabuf[i]);
                }
                DEBUG("\r\n");
                gl_RestoreCmdObj.RestoreHead.CMDSerialnum = __REV(((CMDHeadObj*)pdatabuf)->CMDSerialnum);
                gl_RestoreCmdObj.RestoreHead.CMDLength = __REV16(((CMDHeadObj*)pdatabuf)->CMDLength);
                gl_RestoreCmdObj.RestoreHead.ControllerID = __REV(((CMDHeadObj*)pdatabuf)->ControllerID);
                while((temp = UnPackStringParam(pdatabuf,(gl_RestoreCmdObj.RestoreHead.CMDLength + CMD_HEAD_LEN),temp,&(gl_RestoreCmdObj.RestoreParam))) != 0)
                if(gl_IcapDevObj.socketid == NET_SOCKET)
                {
                  CmdResponse(NET_SOCKET, gl_RestoreCmdObj.RestoreHead.CMDSerialnum);
                }
                else
                {
                  CmdResponse(BG_SOCKET, gl_RestoreCmdObj.RestoreHead.CMDSerialnum);
                }
                xEventGroupSetBits(gl_IcapDevObj.oEventGroupHandler, EVENT_RESTORE);
                if(gl_IcapDevObj.socketid == NET_SOCKET)
                {
                  CmdDealResult(NET_SOCKET, gl_RestoreCmdObj.RestoreHead.CMDSerialnum, CMD_Restore, gl_RestoreCmdObj.RestoreHead.ControllerID, 0, (u8*)"0000");//�ָ��������óɹ�
                }
                else
                {
                  CmdDealResult(BG_SOCKET, gl_RestoreCmdObj.RestoreHead.CMDSerialnum, CMD_Restore, gl_RestoreCmdObj.RestoreHead.ControllerID, 0, (u8*)"0000");//�ָ��������óɹ�
                }
                break;
            case 0x1214:      //ʱ��ͬ��
                DEBUG("&&&&&&&&&&&&@@@@@@@@@@@@@@�յ�ʱ��ͬ����������ݣ�");
                for(i=0; i<__REV16(((CMDHeadObj*)pdatabuf)->CMDLength); i++)
                {
                  DEBUG("%02X|", pdatabuf[i]);
                }
                DEBUG("\r\n");
                gl_TimeSynchronCmdObj.TimeSynchronHead.CMDSerialnum = __REV(((CMDHeadObj*)pdatabuf)->CMDSerialnum);
                gl_TimeSynchronCmdObj.TimeSynchronHead.CMDLength = __REV16(((CMDHeadObj*)pdatabuf)->CMDLength);
                while((temp = UnPackStringParam(pdatabuf,(gl_TimeSynchronCmdObj.TimeSynchronHead.CMDLength + CMD_HEAD_LEN),temp,&(gl_TimeSynchronCmdObj.TimeSynchronParam))) != 0)
                DEBUG("�������Ĳ���:%s\r\n", gl_TimeSynchronCmdObj.TimeSynchronParam.ParamValue);
                if(gl_IcapDevObj.socketid == NET_SOCKET)
                {
                  CmdResponse(NET_SOCKET, gl_TimeSynchronCmdObj.TimeSynchronHead.CMDSerialnum);
                }
                else
                {
                  CmdResponse(BG_SOCKET, gl_TimeSynchronCmdObj.TimeSynchronHead.CMDSerialnum);
                }
                RTCSetTime(gl_TimeSynchronCmdObj.TimeSynchronParam.ParamValue);
                if(gl_IcapDevObj.socketid == NET_SOCKET)
                {
                  CmdDealResult(NET_SOCKET, gl_TimeSynchronCmdObj.TimeSynchronHead.CMDSerialnum, CMD_TimeSynchron, gl_RestoreCmdObj.RestoreHead.ControllerID, 0, (u8*)"0000");
                }
                else
                {
                  CmdDealResult(BG_SOCKET, gl_TimeSynchronCmdObj.TimeSynchronHead.CMDSerialnum, CMD_TimeSynchron, gl_RestoreCmdObj.RestoreHead.ControllerID, 0, (u8*)"0000");
                }
                break;
            default:
                DEBUG("��������ԣ������ݣ�");
                for(i=0; i<__REV16(((CMDHeadObj*)pdatabuf)->CMDLength); i++)
                {
                  DEBUG("%02X|", pdatabuf[i]);
                }
                DEBUG("\r\n");
                break;
        }
      }
      else
      {
        DEBUG("�����п�����ID����\r\n");
      }
    }
    else
    {
      DEBUG("����CRCУ�����\r\n");
    }
  }
  else if(((CMDHeadObj*)pdatabuf)->CMDType == 0x02)
  {
    gl_CMDtaskObj.heart_cnt = 0;          //�յ����� �������������
    DEBUG("�յ������ظ���\r\n");
  }
  else
  {
    DEBUG("���ķ���������������\r\n");
  }
}
//��¼��������
//retrycnt:���͵�½����ȴ���ʱ���Դ���
//overtime:���͵�½����ȴ���ʱʱ��
//iRet:1 ��½�ɹ� 0 ʧ��
u8 Signin_Cmd(u8 retrycnt, u32 overtime)
{
  char *pchBuf = NULL;
  char *pchCmdbuf = NULL;
  u8 iRet = 0;
	u8 i;
  u32 randserialnum = 0; 

  randserialnum = Randnumget();
  pchCmdbuf = pvPortMalloc(100);
  memset(pchCmdbuf, 0, 100);
  PackMsgHead(pchCmdbuf, CMD_DEALREPLY,CMD_SigninHeart, gl_LampConfigObj.ControllerID, 0x00, &randserialnum);
  PackAddStringParam(pchCmdbuf, SERIALNUM, (u8*)gl_IcapDevObj.DevSerialnum);
  PackMsgCrc(pchCmdbuf);
  gl_CMDtaskObj.CmdTranCnt = __REV16(((CMDHeadObj*)pchCmdbuf)->CMDLength) + 13;            //��������¼��
  for(i=0; i<retrycnt; i++)
  {
    DEBUG("��¼���ʹ�����%d\r\n", i);
    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    DEBUG("��¼���������ź�����\r\n");
    EC600M_Send(NET_SOCKET, pchCmdbuf, gl_CMDtaskObj.CmdTranCnt);                                      //��������
    pchBuf = EC600M_Receive(overtime);
    if(pchBuf != NULL)
    {
      DEBUG("gl_recvSerialNo:%d\r\n", gl_recvSerialNo);
      DEBUG("�յ������к�:%d\r\n", __REV(((CMDHeadObj*)pchBuf)->CMDSerialnum));
      if((((CMDHeadObj*)pchBuf)->CMDType == 0x02) &&  (__REV(((CMDHeadObj*)pchBuf)->CMDSerialnum) == gl_recvSerialNo))
      {
        iRet = 1;
        DEBUG("��¼�����ͷ��ź�����\r\n");
        xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
        break;
      }
    }
    DEBUG("û�յ�����¼�����ͷ��ź�����\r\n");
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
  }
  // memset(pchCmdbuf, 0, EC600M_BUF_NUM);
  clear_RXBuffer();
  clear_TXBuffer();
  vPortFree(pchCmdbuf);
  return iRet;
}

//��������

void Heart_Cmd(void)
{
    u32 randserialnum = 0; 
    char *pchCmdbuf = NULL;

    randserialnum = Randnumget();
    pchCmdbuf = pvPortMalloc(100);
    memset(pchCmdbuf, 0, 100);
    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    DEBUG("�������������ź�����\r\n");
    PackMsgHead(pchCmdbuf, CMD_DEALREPLY, CMD_SigninHeart,
                gl_LampConfigObj.ControllerID,0x00,&randserialnum);
    PackAddStringParam(pchCmdbuf, SERIALNUM, (u8*)gl_IcapDevObj.DevSerialnum);
    PackMsgCrc(pchCmdbuf);
    gl_CMDtaskObj.CmdTranCnt = __REV16(((CMDHeadObj*)pchCmdbuf)->CMDLength) + 13; //�������� 
    EC600M_Send(NET_SOCKET, pchCmdbuf, gl_CMDtaskObj.CmdTranCnt);
    printf("�������������������ͳɹ���\r\n");
    gl_CMDtaskObj.recvdatatime = RTC_GetCounter();  //ÿ�η��������� ʱ�����¼�
    DEBUG("���������ͷ��ź�����\r\n");
    vPortFree(pchCmdbuf);
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
}

//�������ȡ
//����ֵ��32λ�����
u32 Randnumget(void)
{
    u32 timecount = 0;
    u32 randnum = 0;
    timecount = RTC_GetCounter();
    srand(timecount);
    randnum = rand();
    return randnum;
}

