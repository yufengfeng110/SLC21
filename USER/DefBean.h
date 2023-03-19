#ifndef __DEFBEAN_H
#define __DEFBEAN_H
#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "stm32f10x.h"

//#define __DEBUGCOM

#ifdef __DEBUGCOM
#define DEBUG(format, ...) printf(format, ##__VA_ARGS__);
#else
#define DEBUG(format, ...) 
#endif

//控制器参数存储事件标志宏定义
#define EVENT_CTRLID 				(1<<0)		//存储控制器ID事件标志
#define EVENT_SERVERIP 				(1<<1)		//存储服务器IP事件标志
#define EVENT_SERVERPORT 			(1<<2)		//存储服务器端口事件标志
#define EVENT_RESTORE				(1<<3)		//恢复出厂设置事件标志
#define EVENT_SAVEENERGY			(1<<4)		//存储电量事件标志
#define EVENT_ALL 					(EVENT_CTRLID|EVENT_SERVERIP|EVENT_SERVERPORT|EVENT_RESTORE|EVENT_SAVEENERGY)
//电能计算事件标志宏定义
#define EVENT_FIRSTLAMP 			(1<<0)		//路灯1事件标志
#define EVENT_SECONDLAMP 			(1<<1)		//路灯2事件标志
#define EVENT_ALLLAMP				(EVENT_FIRSTLAMP|EVENT_SECONDLAMP)
//中断优先级宏定义	
#define TIMER2_IRQPRIORITY			2				//定时器2中断优先级
#define TIMER3_IRQPRIORITY			3				//定时器3中断优先级
#define EXTI12_IRQPRIORITY			4				//外部12中断优先级
#define RTC_IRQPRIORITY				6				//实时时钟中断优先级
#define UART1_IRQPRIORITY			7				//串口1中断优先级
#define UART2_IRQPRIORITY			8				//串口2中断优先级
#define UART3_IRQPRIORITY			9				//串口3中断优先级
//
#define	EC600M_RX_BUF				700				//4G模块接收缓冲最大空间  200	
#define	EC600M_BUF_NUM				400				//4G模块缓冲空间		200	
#define CMD_SignHeart_LEN  			50				//登录和心跳命令总长度
#define CMD_HEAD_LEN				13				//消息头长度
#define CMD_RESULT_LEN				28				//命令处理结果长度
#define HEARTOVERTIME  				10              //心跳超时时间60S
#define NET_SOCKET					0				//网络任务socket标识
#define BG_SOCKET					1				//后台任务socket标识
#define OTA_SOCKET					2				//远程升级任务socket标识
#define NETTASK_NODATA				0				//网络任务没收到命令
#define NETTASK_RECVDATA			1				//网络任务收到命令
#define OTATIME						600				//OTA查询间隔
#define APPVERSION                  "1.0.1"			//应用程序版本号
#define UPDATE_ENABLE            	0xAB5665BA    	//升级使能标志
#define FLASH_BIN_ADDR           	0x08010400    	//升级文件存储起始地址
#define	LED_ON						0
#define LED_OFF						1
//灯具控制宏定义
#define LAMP_1						1
#define LAMP_2						2
#define LAMP_OPEN					1
#define LAMP_CLOSE					0
#define AC220_INPUT    				1
#define AC220_OUTPUT12 				2
#define HC4052_A 					PBout(0)
#define HC4052_B 					PBout(1)
#define DEFAULT_DIMVALUE			80
//消息类型宏定义 
#define CMD_REQUEST  				1				//命令请求
#define CMD_RESPONSE  				2				//命令响应
#define EVENTALARM_UPLOAD  			3				//事件告警上报
#define EVENTALARM_RESPONSE 		4				//事件告警响应
#define CMD_DEALREPLY  				5				//命令处理结果回复
//消息ID宏定义
#define CMD_Config					0x1001			//配置命令
#define CMD_LampControl				0x1208			//灯控命令
#define CMD_LampStatus				0x1209			//灯控命令
#define CMD_Restore					0x1211			//恢复出厂设置命令
#define CMD_TimeSynchron			0x1214			//时间同步命令
#define CMD_SigninHeart				0x1300			//登录&心跳命令

//消息参数类型
#define CONTROLLERID_ID				0X01			//控制器ID
#define SERVERIP					0X02			//服务器IP
#define SERVERPORT					0X03			//服务器端口号
#define LUX							0X21			//照度
#define RESOURCEVALUE				0X24			//资源值
#define RESOURCETYPE				0X25			//资源类型
#define DELAYTIME					0X28			//延迟时间
#define LAMPOPEN					0X2C			//开灯
#define LAMPCLOSE					0X2D			//关灯
#define SERIALNUM					0X31			//控制器序列号
#define SYSTEMTIME					0X42			//系统时间
//HLW8112宏定义
#define REG_SYSCON_ADDR         	0x00
#define REG_EMUCON_ADDR         	0x01
#define REG_HFCONST_ADDR        	0x02
#define REG_EMUCON2_ADDR        	0x13
#define REG_ANGLE_ADDR        		0x22			//相角寄存器
#define REG_UFREQ_ADDR          	0x23    		//市电线性频率地址
#define REG_RMSIA_ADDR          	0x24
#define REG_RMSIB_ADDR          	0x25
#define REG_RMSU_ADDR           	0x26
#define REG_PF_ADDR             	0x27
#define REG_ENERGY_PA_ADDR			0x28
#define REG_ENERGY_PB_ADDR			0x29
#define REG_POWER_PA_ADDR       	0x2C
#define REG_POWER_PB_ADDR       	0x2D
#define REG_SAGCYC_ADDR         	0x17
#define REG_SAGLVL_ADDR         	0x18
#define REG_OVLVL_ADDR          	0x19
#define REG_PEAKU_ADDR          	0x32   
#define REG_INT_ADDR          		0x1D
#define REG_IE_ADDR          		0x40
#define REG_IF_ADDR          		0x41
#define REG_RIF_ADDR          		0x42
#define REG_RDATA_ADDR          	0x44
#define REG_CHECKSUM_ADDR			0x6f
#define REG_RMS_IAC_ADDR			0x70
#define REG_RMS_IBC_ADDR			0x71
#define REG_RMS_UC_ADDR				0x72
#define REG_POWER_PAC_ADDR			0x73
#define REG_POWER_PBC_ADDR			0x74
#define REG_POWER_SC_ADDR			0x75
#define REG_ENERGY_AC_ADDR			0x76
#define REG_ENERGY_BC_ADDR			0x77

//EC600M队列宏定义
#define EC600M_Q_NUM				2				//EC600M接收数据队列项目数

#pragma pack(1)
//EC600M结构体
typedef struct
{
	QueueHandle_t	EC600M_RxData_Queue;					//EC600M接收数据队列句柄
	int				g_ec600m_RXCnt;							//接收数据计数
	int				g_ec600m_TXCnt;							//发送数据计数
	char 			g_ec600m_RXBuf[EC600M_RX_BUF]; 			//接收缓冲
	char 			g_ec600m_TXBuf[EC600M_BUF_NUM];			//发送缓冲
}Ec600NetObj;				   	

//和中心通信任务结构体
typedef struct
{
	u32 			firsttime;                              //初始计时
    u32 			recvdatatime;                           //收到数据计时
	u8 				heart_cnt;								//心跳计数		
	int   			CmdRecvCnt; 							//命令接收计数
	int   			CmdTranCnt;								//命令发送计数
	char			CmdRecvBuf[EC600M_RX_BUF];				//命令接收缓冲指针
	// char 			CmdTranBuf[EC600M_BUF_NUM];				//命令发送缓冲指针
}CMDtaskObj;	


//电参数单元
typedef struct
{
	float fdV;  //电压
	float fdA;  //电流
	float fdP;  //功率
	float fdQ;  //电量
	float ffc;  //功率因数
}ADCElecObj,*pADCElecObj;			

//查询数据返回结构
typedef struct
{
	ADCElecObj    	oAC220_In;      //AC220V输入电参数
	ADCElecObj    	oAC220_Out[2];  //AC220V_1/2输出
	float 			temprature;		//内部温度
	int       		iCSQ;			//4G模块信号
	char 			chStatus;     	//DI输入、开关灯状态 lamp1:bit1 lamp2:bit2 DI:bit3
}ADCSearchObj,*pADCSearchObj;

//通用用消息头结构体
typedef struct
{
	u8		CMDType;								//消息类型
	u32		CMDSerialnum;							//消息流水号
	u16		CMDLength;								//消息体长度
	u32 	NoUSE; 									//保留
	u16 	CRC16;									//CRC16校验
	u16 	CMDID;									//消息ID
	u32		ControllerID;							//路灯控制器ID
	u32		luminiareID;							//灯具ID	
}CMDHeadObj;			   									

//登录和心跳命令消息体结构体
typedef struct
{
	u16		ParamType;								//参数类型
	u8		ParamValue[50];							//参数值  50
}StringParamObj;

//配置命令结构体
typedef struct
{
	CMDHeadObj		ConfigHead;							//配置命令头结构体
	StringParamObj	ConfigParam[3];						//配置命令参数结构体数组
}ConfigCmdObj;

//灯控命令结构体
typedef struct
{
	CMDHeadObj		LampCtrlHead;
	StringParamObj	LampCtrlParam;
}LampCtrlCmdObj;

//灯控状态查询命令结构体
typedef struct
{
	CMDHeadObj		LampStatusHead;
	StringParamObj	LampStatusParam;
}LampStatusCmdObj;

//恢复出厂设置命令结构体
typedef struct
{
	CMDHeadObj		RestoreHead;
	StringParamObj	RestoreParam;
}RestoreCmdObj;

//时间同步命令结构体
typedef struct
{
	CMDHeadObj		TimeSynchronHead;
	StringParamObj	TimeSynchronParam;
}TimeSynchronCmdObj;

//路灯控制器配置文件
typedef struct
{
	u8						ControllerID[10];						//路灯控制器ID
	u8						ServerIP[30];							//服务器IP
	u16						ServerPort;								//服务器端口号
	u8						RestoreFlag;							//恢复出厂设置标志位
	u8						Lamp1_status;							//路灯1开关状态
	u8						Lamp2_status;							//路灯2开关状态
	int						Lamp1_dimval;							//路灯1调光值
	int						Lamp2_dimval;							//路灯2调光值
	u16						year;
	u8						month;
	u8						day;
	u8						hour;
	u8						minute;
	u8						second;
}LampConfigObj;

//项目全局变量
typedef struct
{
	SemaphoreHandle_t 		BGtaskSemaphore;				//后台任务用二值信号量
	SemaphoreHandle_t		MutexSemaphore;					//互斥信号量
	EventGroupHandle_t 		oEventGroupHandler;				//写EEPROM事件标志组
	EventGroupHandle_t		ohlw8112_EventHandler;			//电能计算事件标志组
	u8						DevSerialnum[25];				//控制器序列号
	int						socketid;						//链接标识 
	u8						pchUpgradeUrl[100];				//OTA地址
	u8						heartcountflag;					//心跳计数flag			
}IcapDevObj;

//远程升级结构体
// typedef struct
// {
// 	char version[20];
// 	char url[100];
// }WKS_UpdateObj,*pWKS_UpdateObj;
#pragma pack()

extern IcapDevObj			gl_IcapDevObj;
extern ConfigCmdObj 		gl_ConfigCmdObj;
extern Ec600NetObj  		gl_Ec600MObj; 
extern LampConfigObj 		gl_LampConfigObj; 
extern LampCtrlCmdObj 		gl_LampCtrlCmdObj;
extern LampStatusCmdObj 	gl_LampStatusCmdObj;
extern RestoreCmdObj 		gl_RestoreCmdObj;
extern TimeSynchronCmdObj 	gl_TimeSynchronCmdObj;
extern ADCSearchObj 		g_oADCSearchObj;
extern CMDtaskObj 			gl_CMDtaskObj;
#endif

