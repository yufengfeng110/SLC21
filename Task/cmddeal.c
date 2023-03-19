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

// u32 gl_firstSerialNo = 1;                   //初始消息序列号
u32 gl_recvSerialNo = 1;                    //收到消息序列号

//构建消息头函数
//buf:命令存储区
//type:消息类型
//cmd:消息ID
//ctrid:路灯控制器ID
//ledid:灯具ID
//serial:消息流水号
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

//向消息中添加字符串
//buf:命令存储区
//type:参数类型
//data:参数值(字符串)
void PackAddStringParam(char* buf,u16 type,u8* data)
{
   u16 len = ((CMDHeadObj*)buf)->CMDLength;

   StringParamObj* pParam = (StringParamObj*)(buf + len + CMD_HEAD_LEN);
   pParam->ParamType = __REV16(type);
   strcpy((char*)pParam->ParamValue,(const char*)data);
   ((CMDHeadObj*)buf)->CMDLength += (strlen((const char*)data) + 3); 
}

//组消息头中CRC校验值
//buf:命令存放buffer
void PackMsgCrc(char* buf)
{
  u16 len = ((CMDHeadObj*)buf)->CMDLength ;
  ((CMDHeadObj*)buf)->CRC16 = CRC16_MODBUS((unsigned char*)buf + 13,len);
  ((CMDHeadObj*)buf)->CMDLength = __REV16(len);
}

//命令响应函数
void CmdResponse(int socketid, u32 serialnum)
{
  char *pchBuf;
  u8 i;
  pchBuf = pvPortMalloc(CMD_HEAD_LEN);
  memset(pchBuf, 0, CMD_HEAD_LEN);
  ((CMDHeadObj*)pchBuf)->CMDType = CMD_RESPONSE;
  ((CMDHeadObj*)pchBuf)->CMDSerialnum = __REV(serialnum);
  DEBUG("命令回复：");
  for(i=0;i<CMD_HEAD_LEN;i++)
  {
    DEBUG("%02X|", pchBuf[i]);
  }
  DEBUG("\r\n");
  EC600M_Send(socketid, pchBuf, CMD_HEAD_LEN); 
  vPortFree(pchBuf);
}

//命令处理结果回复
//serial:中心下发命令的消息流水号
//cmd：消息ID
//ctrid:控制器ID
//ledid:灯具ID
//pramatype:参数类型
//string:参数值
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
    DEBUG("命令处理结果：");
    for(i=0;i<CMD_RESULT_LEN;i++)
    {
      DEBUG("%02X|", pchBuf[i]);
    }
    DEBUG("\r\n");
    EC600M_Send(socketid, pchBuf, CMD_RESULT_LEN);
    vPortFree(pchBuf); 
}

//组织电量数据并发送
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
    printf("灯状态查询回复：");
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
    printf("查询的参数值：");
    for(i=0; i<paramlength; i++)
    {
      printf("%02X|", pchBuf[i]);
    }
    printf("\r\n");
    g_oADCSearchObj.chStatus &= ~(0x01 << 2);
    LED4 = 1;
    vPortFree(pchBuf);  
}

//解析不定长命令中参数值函数
//buf:待解析的命令
//len:待解析命令中消息体长度
//pos:解析完成的消息体长度
//param:解析完存放的结构体变量
int UnPackStringParam(char* buf,int len,int pos,StringParamObj* param)
{
  int iRet = 0;
  if((pos + CMD_HEAD_LEN) >= len)
  {
    goto END;
  }
  param->ParamType = ((StringParamObj*)(buf + CMD_HEAD_LEN + pos))->ParamType;
  strcpy((char*)param->ParamValue,(const char*)((StringParamObj*)(buf + CMD_HEAD_LEN + pos))->ParamValue);
  iRet = pos + strlen((const char*)param->ParamValue) + 3;             //解析完消息体的长度
  DEBUG("解析完的消息体长度：%d\r\n", strlen((const char*)param->ParamValue));

  END:
  return iRet;
}

//数据解析函数
//pdatabuf：要解析的数据
void UnPackCmdParam(char* pdatabuf)
{
  u8 i;
  u8 j=0;
  u16 temp=10;
  // u8 lux = 80;      //照度
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
  EventBits_t elecenergy_EventValue;    //电量事件标志值


  if(((CMDHeadObj*)pdatabuf)->CMDType == 0x01)              //中心下发命令：配置命令、控制命令、查询灯具状态
  {                                                         //时间同步命令、恢复出厂设置命令
    DEBUG("收到中心命令数据个数：%d\r\n",__REV16(((CMDHeadObj*)pdatabuf)->CMDLength));
	  DEBUG("收到命令ID:%02X\r\n", __REV16(((CMDHeadObj*)pdatabuf)->CMDID));
    DEBUG("收到的CRC:%04X\r\n", ((CMDHeadObj*)pdatabuf)->CRC16);
    DEBUG("算出的CRC:%04X\r\n", CRC16_MODBUS((unsigned char*)(pdatabuf+13), __REV16(((CMDHeadObj*)pdatabuf)->CMDLength)));
    if(((CMDHeadObj*)pdatabuf)->CRC16 == CRC16_MODBUS((unsigned char*)(pdatabuf+13), __REV16(((CMDHeadObj*)pdatabuf)->CMDLength)))  //校验CRC
    {
      DEBUG("CRC校验通过！\r\n");
      HexArrayToStr((unsigned char*)contrlid, (unsigned char*)(&(((CMDHeadObj*)pdatabuf)->ControllerID)), 4);  
      if(strcmp((const char*)contrlid, (const char*)gl_LampConfigObj.ControllerID) == 0)                        //判断命令是否是发给自己
      {
        switch (__REV16(((CMDHeadObj*)pdatabuf)->CMDID))
        {
            case 0x1001:            //配置命令
              DEBUG("收到配置命令！！内容：");
              for(i=0; i<(__REV16(((CMDHeadObj*)pdatabuf)->CMDLength)+CMD_HEAD_LEN); i++)
              {
                DEBUG("%02X|", pdatabuf[i]);
              }
              DEBUG("\r\n");
              gl_ConfigCmdObj.ConfigHead.CMDSerialnum = __REV(((CMDHeadObj*)pdatabuf)->CMDSerialnum);           //从命令中获取消息流水号
              DEBUG("收到的流水号：%04X\r\n", __REV(((CMDHeadObj*)pdatabuf)->CMDSerialnum));
              gl_ConfigCmdObj.ConfigHead.CMDLength = __REV16(((CMDHeadObj*)pdatabuf)->CMDLength);               //从命令中获取消息体长度
              gl_ConfigCmdObj.ConfigHead.ControllerID = __REV(((CMDHeadObj*)pdatabuf)->ControllerID);           //从命令中获取控制器ID
              while((temp = UnPackStringParam(pdatabuf,(gl_ConfigCmdObj.ConfigHead.CMDLength + CMD_HEAD_LEN),temp,&(gl_ConfigCmdObj.ConfigParam[j]))) != 0)
              {
                  DEBUG("消息体长度:%d\r\n", gl_ConfigCmdObj.ConfigHead.CMDLength);
                  DEBUG("temp值:%d\r\n", temp);
                  j++;
              }
              for(i=0; i<j; i++)    //调试
              {
                DEBUG("解析出来第%d个参数类型:%04X\r\n", i+1,__REV16((&(gl_ConfigCmdObj.ConfigParam[i]))->ParamType));
                DEBUG("解析出来第%d个参数值:%s\r\n", i+1, (&(gl_ConfigCmdObj.ConfigParam[i]))->ParamValue);
              }
              memset(gl_CMDtaskObj.CmdRecvBuf, 0, EC600M_RX_BUF);          //解析完命令清空命令接收缓存
              if(gl_IcapDevObj.socketid == NET_SOCKET)
              {
                  CmdResponse(NET_SOCKET, gl_ConfigCmdObj.ConfigHead.CMDSerialnum);         //发送命令响应
              }
              else
              {
                  CmdResponse(BG_SOCKET, gl_ConfigCmdObj.ConfigHead.CMDSerialnum);
              }
              for(i=0; i<j; i++)                                            //存储参数值
              {
                switch (__REV16((&(gl_ConfigCmdObj.ConfigParam[i]))->ParamType))
                {
                  case 1:
                    strcpy((char*)(gl_LampConfigObj.ControllerID), (const char *)((&(gl_ConfigCmdObj.ConfigParam[i]))->ParamValue));
                    DEBUG("新控制器ID:%s准备存储......\r\n", gl_LampConfigObj.ControllerID);
                    xEventGroupSetBits(gl_IcapDevObj.oEventGroupHandler, EVENT_CTRLID);         //发送事件标志位
                    break;
                  case 2:
                    strcpy((char*)(gl_LampConfigObj.ServerIP), (const char *)((&(gl_ConfigCmdObj.ConfigParam[i]))->ParamValue));
                    DEBUG("新服务器IP:%s准备存储......\r\n", gl_LampConfigObj.ServerIP);
                    xEventGroupSetBits(gl_IcapDevObj.oEventGroupHandler, EVENT_SERVERIP);
                    break;
                  case 3:
                    strcpy((char*)(gl_LampConfigObj.ServerPort), (const char *)((&(gl_ConfigCmdObj.ConfigParam[i]))->ParamValue));
                    DEBUG("新端口号:%d准备存储......\r\n", gl_LampConfigObj.ServerPort);
                    xEventGroupSetBits(gl_IcapDevObj.oEventGroupHandler, EVENT_SERVERPORT);
                    break;                
                  default:
                    DEBUG("配置参数出错！\r\n");
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
            case 0x1208:        //灯控命令
                DEBUG("收到灯控命令！！内容：");
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
                    DEBUG("temp值:%d\r\n", temp);
                    j++;
                }
                DEBUG("j的值：%d\r\n", j);
                DEBUG("解析出来第%d个参数类型:%04X\r\n", j,__REV16((&(gl_LampCtrlCmdObj.LampCtrlParam))->ParamType));
                DEBUG("解析出来第%d个参数值:%s\r\n", j, (&(gl_LampCtrlCmdObj.LampCtrlParam))->ParamValue);
                DEBUG("控制命令的灯具ID:%d\r\n", gl_LampCtrlCmdObj.LampCtrlHead.luminiareID);
                switch (__REV(((CMDHeadObj*)pdatabuf)->luminiareID))
                {
                  case 1:
                    if(__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x2C)          //灯1 开
                    {
                        LampControl(LAMP_1, LAMP_OPEN);
                        LED_LAMP1 = LED_ON;
                        xEventGroupSetBits(gl_IcapDevObj.ohlw8112_EventHandler, EVENT_FIRSTLAMP);
                        DEBUG("灯1开!\r\n");
                    }
                    else if (__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x2D)    //灯1 关
                    {
                        LampControl(LAMP_1, LAMP_CLOSE);
                        LED_LAMP1 = LED_OFF;
                        xEventGroupClearBits(gl_IcapDevObj.ohlw8112_EventHandler, EVENT_FIRSTLAMP);
                        DEBUG("灯1关!\r\n");            
                    }
                    else if (__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x21)    //灯1调光
                    {
                        DEBUG("灯1调光!\r\n");
                        gl_LampConfigObj.Lamp1_dimval = atoi((const char*)gl_LampCtrlCmdObj.LampCtrlParam.ParamValue);
                        PWM_Servicefunc(LAMP_1, gl_LampConfigObj.Lamp1_dimval);
                        DEBUG("灯1照度调节:%d\r\n", gl_LampConfigObj.Lamp1_dimval);
                    }
                    break;
                  case 2:
                    if(__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x2C)          //灯2 开
                    {
                        LampControl(LAMP_2, LAMP_OPEN);
                        LED_LAMP2 = LED_ON;
                        xEventGroupSetBits(gl_IcapDevObj.ohlw8112_EventHandler, EVENT_SECONDLAMP);
                        DEBUG("灯2开!\r\n");
                    }
                    else if (__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x2D)    //灯2 关
                    {
                        LampControl(LAMP_2, LAMP_CLOSE);
                        LED_LAMP2 = LED_OFF;
                        xEventGroupClearBits(gl_IcapDevObj.ohlw8112_EventHandler, EVENT_SECONDLAMP);
                        DEBUG("灯2关!\r\n");               
                    }
                    else if (__REV16(gl_LampCtrlCmdObj.LampCtrlParam.ParamType) == 0x21)    //灯2调光
                    {
                        gl_LampConfigObj.Lamp2_dimval = atoi((const char*)gl_LampCtrlCmdObj.LampCtrlParam.ParamValue);
                        PWM_Servicefunc(LAMP_2, gl_LampConfigObj.Lamp2_dimval);
                        DEBUG("灯2照度调节:%d\r\n", gl_LampConfigObj.Lamp2_dimval);
                    }
                    break;
                  default:
                    DEBUG("路灯ID错误!\r\n");
                    DEBUG("错误的灯具ID:%d\r\n", __REV(((CMDHeadObj*)pdatabuf)->luminiareID));
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
            case 0x1209:        //状态查询命令
                DEBUG("收到路灯状态查询命令！！内容：");
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
                  case 0:       //控制器电量
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
                    /*组织电量数据并发送*/
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
                    /*组织电量数据并发送*/
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
            case 0x1211:        //恢复出厂设置
                DEBUG("收到控制器恢复出厂命令！！内容：");
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
                  CmdDealResult(NET_SOCKET, gl_RestoreCmdObj.RestoreHead.CMDSerialnum, CMD_Restore, gl_RestoreCmdObj.RestoreHead.ControllerID, 0, (u8*)"0000");//恢复出厂设置成功
                }
                else
                {
                  CmdDealResult(BG_SOCKET, gl_RestoreCmdObj.RestoreHead.CMDSerialnum, CMD_Restore, gl_RestoreCmdObj.RestoreHead.ControllerID, 0, (u8*)"0000");//恢复出厂设置成功
                }
                break;
            case 0x1214:      //时间同步
                DEBUG("&&&&&&&&&&&&@@@@@@@@@@@@@@收到时间同步命令！！内容：");
                for(i=0; i<__REV16(((CMDHeadObj*)pdatabuf)->CMDLength); i++)
                {
                  DEBUG("%02X|", pdatabuf[i]);
                }
                DEBUG("\r\n");
                gl_TimeSynchronCmdObj.TimeSynchronHead.CMDSerialnum = __REV(((CMDHeadObj*)pdatabuf)->CMDSerialnum);
                gl_TimeSynchronCmdObj.TimeSynchronHead.CMDLength = __REV16(((CMDHeadObj*)pdatabuf)->CMDLength);
                while((temp = UnPackStringParam(pdatabuf,(gl_TimeSynchronCmdObj.TimeSynchronHead.CMDLength + CMD_HEAD_LEN),temp,&(gl_TimeSynchronCmdObj.TimeSynchronParam))) != 0)
                DEBUG("解析出的参数:%s\r\n", gl_TimeSynchronCmdObj.TimeSynchronParam.ParamValue);
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
                DEBUG("中心命令不对！！内容：");
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
        DEBUG("命令中控制器ID错误！\r\n");
      }
    }
    else
    {
      DEBUG("命令CRC校验错误！\r\n");
    }
  }
  else if(((CMDHeadObj*)pdatabuf)->CMDType == 0x02)
  {
    gl_CMDtaskObj.heart_cnt = 0;          //收到心跳 即清楚心跳计数
    DEBUG("收到心跳回复！\r\n");
  }
  else
  {
    DEBUG("中心发送命令类型有误！\r\n");
  }
}
//登录发送命令
//retrycnt:发送登陆命令等待超时重试次数
//overtime:发送登陆命令等待超时时间
//iRet:1 登陆成功 0 失败
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
  gl_CMDtaskObj.CmdTranCnt = __REV16(((CMDHeadObj*)pchCmdbuf)->CMDLength) + 13;            //组心跳登录包
  for(i=0; i<retrycnt; i++)
  {
    DEBUG("登录发送次数：%d\r\n", i);
    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    DEBUG("登录函数接收信号量！\r\n");
    EC600M_Send(NET_SOCKET, pchCmdbuf, gl_CMDtaskObj.CmdTranCnt);                                      //发送命令
    pchBuf = EC600M_Receive(overtime);
    if(pchBuf != NULL)
    {
      DEBUG("gl_recvSerialNo:%d\r\n", gl_recvSerialNo);
      DEBUG("收到的序列号:%d\r\n", __REV(((CMDHeadObj*)pchBuf)->CMDSerialnum));
      if((((CMDHeadObj*)pchBuf)->CMDType == 0x02) &&  (__REV(((CMDHeadObj*)pchBuf)->CMDSerialnum) == gl_recvSerialNo))
      {
        iRet = 1;
        DEBUG("登录函数释放信号量！\r\n");
        xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
        break;
      }
    }
    DEBUG("没收到，登录函数释放信号量！\r\n");
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
  }
  // memset(pchCmdbuf, 0, EC600M_BUF_NUM);
  clear_RXBuffer();
  clear_TXBuffer();
  vPortFree(pchCmdbuf);
  return iRet;
}

//心跳命令

void Heart_Cmd(void)
{
    u32 randserialnum = 0; 
    char *pchCmdbuf = NULL;

    randserialnum = Randnumget();
    pchCmdbuf = pvPortMalloc(100);
    memset(pchCmdbuf, 0, 100);
    xSemaphoreTake(gl_IcapDevObj.MutexSemaphore,portMAX_DELAY);
    DEBUG("心跳函数接收信号量！\r\n");
    PackMsgHead(pchCmdbuf, CMD_DEALREPLY, CMD_SigninHeart,
                gl_LampConfigObj.ControllerID,0x00,&randserialnum);
    PackAddStringParam(pchCmdbuf, SERIALNUM, (u8*)gl_IcapDevObj.DevSerialnum);
    PackMsgCrc(pchCmdbuf);
    gl_CMDtaskObj.CmdTranCnt = __REV16(((CMDHeadObj*)pchCmdbuf)->CMDLength) + 13; //组心跳包 
    EC600M_Send(NET_SOCKET, pchCmdbuf, gl_CMDtaskObj.CmdTranCnt);
    printf("￥￥￥￥￥心跳包发送成功！\r\n");
    gl_CMDtaskObj.recvdatatime = RTC_GetCounter();  //每次发送完心跳 时间重新记
    DEBUG("心跳函数释放信号量！\r\n");
    vPortFree(pchCmdbuf);
    xSemaphoreGive(gl_IcapDevObj.MutexSemaphore);
}

//随机数获取
//返回值：32位随机数
u32 Randnumget(void)
{
    u32 timecount = 0;
    u32 randnum = 0;
    timecount = RTC_GetCounter();
    srand(timecount);
    randnum = rand();
    return randnum;
}

