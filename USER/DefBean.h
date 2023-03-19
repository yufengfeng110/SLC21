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

//�����������洢�¼���־�궨��
#define EVENT_CTRLID 				(1<<0)		//�洢������ID�¼���־
#define EVENT_SERVERIP 				(1<<1)		//�洢������IP�¼���־
#define EVENT_SERVERPORT 			(1<<2)		//�洢�������˿��¼���־
#define EVENT_RESTORE				(1<<3)		//�ָ����������¼���־
#define EVENT_SAVEENERGY			(1<<4)		//�洢�����¼���־
#define EVENT_ALL 					(EVENT_CTRLID|EVENT_SERVERIP|EVENT_SERVERPORT|EVENT_RESTORE|EVENT_SAVEENERGY)
//���ܼ����¼���־�궨��
#define EVENT_FIRSTLAMP 			(1<<0)		//·��1�¼���־
#define EVENT_SECONDLAMP 			(1<<1)		//·��2�¼���־
#define EVENT_ALLLAMP				(EVENT_FIRSTLAMP|EVENT_SECONDLAMP)
//�ж����ȼ��궨��	
#define TIMER2_IRQPRIORITY			2				//��ʱ��2�ж����ȼ�
#define TIMER3_IRQPRIORITY			3				//��ʱ��3�ж����ȼ�
#define EXTI12_IRQPRIORITY			4				//�ⲿ12�ж����ȼ�
#define RTC_IRQPRIORITY				6				//ʵʱʱ���ж����ȼ�
#define UART1_IRQPRIORITY			7				//����1�ж����ȼ�
#define UART2_IRQPRIORITY			8				//����2�ж����ȼ�
#define UART3_IRQPRIORITY			9				//����3�ж����ȼ�
//
#define	EC600M_RX_BUF				700				//4Gģ����ջ������ռ�  200	
#define	EC600M_BUF_NUM				400				//4Gģ�黺��ռ�		200	
#define CMD_SignHeart_LEN  			50				//��¼�����������ܳ���
#define CMD_HEAD_LEN				13				//��Ϣͷ����
#define CMD_RESULT_LEN				28				//�����������
#define HEARTOVERTIME  				10              //������ʱʱ��60S
#define NET_SOCKET					0				//��������socket��ʶ
#define BG_SOCKET					1				//��̨����socket��ʶ
#define OTA_SOCKET					2				//Զ����������socket��ʶ
#define NETTASK_NODATA				0				//��������û�յ�����
#define NETTASK_RECVDATA			1				//���������յ�����
#define OTATIME						600				//OTA��ѯ���
#define APPVERSION                  "1.0.1"			//Ӧ�ó���汾��
#define UPDATE_ENABLE            	0xAB5665BA    	//����ʹ�ܱ�־
#define FLASH_BIN_ADDR           	0x08010400    	//�����ļ��洢��ʼ��ַ
#define	LED_ON						0
#define LED_OFF						1
//�ƾ߿��ƺ궨��
#define LAMP_1						1
#define LAMP_2						2
#define LAMP_OPEN					1
#define LAMP_CLOSE					0
#define AC220_INPUT    				1
#define AC220_OUTPUT12 				2
#define HC4052_A 					PBout(0)
#define HC4052_B 					PBout(1)
#define DEFAULT_DIMVALUE			80
//��Ϣ���ͺ궨�� 
#define CMD_REQUEST  				1				//��������
#define CMD_RESPONSE  				2				//������Ӧ
#define EVENTALARM_UPLOAD  			3				//�¼��澯�ϱ�
#define EVENTALARM_RESPONSE 		4				//�¼��澯��Ӧ
#define CMD_DEALREPLY  				5				//��������ظ�
//��ϢID�궨��
#define CMD_Config					0x1001			//��������
#define CMD_LampControl				0x1208			//�ƿ�����
#define CMD_LampStatus				0x1209			//�ƿ�����
#define CMD_Restore					0x1211			//�ָ�������������
#define CMD_TimeSynchron			0x1214			//ʱ��ͬ������
#define CMD_SigninHeart				0x1300			//��¼&��������

//��Ϣ��������
#define CONTROLLERID_ID				0X01			//������ID
#define SERVERIP					0X02			//������IP
#define SERVERPORT					0X03			//�������˿ں�
#define LUX							0X21			//�ն�
#define RESOURCEVALUE				0X24			//��Դֵ
#define RESOURCETYPE				0X25			//��Դ����
#define DELAYTIME					0X28			//�ӳ�ʱ��
#define LAMPOPEN					0X2C			//����
#define LAMPCLOSE					0X2D			//�ص�
#define SERIALNUM					0X31			//���������к�
#define SYSTEMTIME					0X42			//ϵͳʱ��
//HLW8112�궨��
#define REG_SYSCON_ADDR         	0x00
#define REG_EMUCON_ADDR         	0x01
#define REG_HFCONST_ADDR        	0x02
#define REG_EMUCON2_ADDR        	0x13
#define REG_ANGLE_ADDR        		0x22			//��ǼĴ���
#define REG_UFREQ_ADDR          	0x23    		//�е�����Ƶ�ʵ�ַ
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

//EC600M���к궨��
#define EC600M_Q_NUM				2				//EC600M�������ݶ�����Ŀ��

#pragma pack(1)
//EC600M�ṹ��
typedef struct
{
	QueueHandle_t	EC600M_RxData_Queue;					//EC600M�������ݶ��о��
	int				g_ec600m_RXCnt;							//�������ݼ���
	int				g_ec600m_TXCnt;							//�������ݼ���
	char 			g_ec600m_RXBuf[EC600M_RX_BUF]; 			//���ջ���
	char 			g_ec600m_TXBuf[EC600M_BUF_NUM];			//���ͻ���
}Ec600NetObj;				   	

//������ͨ������ṹ��
typedef struct
{
	u32 			firsttime;                              //��ʼ��ʱ
    u32 			recvdatatime;                           //�յ����ݼ�ʱ
	u8 				heart_cnt;								//��������		
	int   			CmdRecvCnt; 							//������ռ���
	int   			CmdTranCnt;								//����ͼ���
	char			CmdRecvBuf[EC600M_RX_BUF];				//������ջ���ָ��
	// char 			CmdTranBuf[EC600M_BUF_NUM];				//����ͻ���ָ��
}CMDtaskObj;	


//�������Ԫ
typedef struct
{
	float fdV;  //��ѹ
	float fdA;  //����
	float fdP;  //����
	float fdQ;  //����
	float ffc;  //��������
}ADCElecObj,*pADCElecObj;			

//��ѯ���ݷ��ؽṹ
typedef struct
{
	ADCElecObj    	oAC220_In;      //AC220V��������
	ADCElecObj    	oAC220_Out[2];  //AC220V_1/2���
	float 			temprature;		//�ڲ��¶�
	int       		iCSQ;			//4Gģ���ź�
	char 			chStatus;     	//DI���롢���ص�״̬ lamp1:bit1 lamp2:bit2 DI:bit3
}ADCSearchObj,*pADCSearchObj;

//ͨ������Ϣͷ�ṹ��
typedef struct
{
	u8		CMDType;								//��Ϣ����
	u32		CMDSerialnum;							//��Ϣ��ˮ��
	u16		CMDLength;								//��Ϣ�峤��
	u32 	NoUSE; 									//����
	u16 	CRC16;									//CRC16У��
	u16 	CMDID;									//��ϢID
	u32		ControllerID;							//·�ƿ�����ID
	u32		luminiareID;							//�ƾ�ID	
}CMDHeadObj;			   									

//��¼������������Ϣ��ṹ��
typedef struct
{
	u16		ParamType;								//��������
	u8		ParamValue[50];							//����ֵ  50
}StringParamObj;

//��������ṹ��
typedef struct
{
	CMDHeadObj		ConfigHead;							//��������ͷ�ṹ��
	StringParamObj	ConfigParam[3];						//������������ṹ������
}ConfigCmdObj;

//�ƿ�����ṹ��
typedef struct
{
	CMDHeadObj		LampCtrlHead;
	StringParamObj	LampCtrlParam;
}LampCtrlCmdObj;

//�ƿ�״̬��ѯ����ṹ��
typedef struct
{
	CMDHeadObj		LampStatusHead;
	StringParamObj	LampStatusParam;
}LampStatusCmdObj;

//�ָ�������������ṹ��
typedef struct
{
	CMDHeadObj		RestoreHead;
	StringParamObj	RestoreParam;
}RestoreCmdObj;

//ʱ��ͬ������ṹ��
typedef struct
{
	CMDHeadObj		TimeSynchronHead;
	StringParamObj	TimeSynchronParam;
}TimeSynchronCmdObj;

//·�ƿ����������ļ�
typedef struct
{
	u8						ControllerID[10];						//·�ƿ�����ID
	u8						ServerIP[30];							//������IP
	u16						ServerPort;								//�������˿ں�
	u8						RestoreFlag;							//�ָ��������ñ�־λ
	u8						Lamp1_status;							//·��1����״̬
	u8						Lamp2_status;							//·��2����״̬
	int						Lamp1_dimval;							//·��1����ֵ
	int						Lamp2_dimval;							//·��2����ֵ
	u16						year;
	u8						month;
	u8						day;
	u8						hour;
	u8						minute;
	u8						second;
}LampConfigObj;

//��Ŀȫ�ֱ���
typedef struct
{
	SemaphoreHandle_t 		BGtaskSemaphore;				//��̨�����ö�ֵ�ź���
	SemaphoreHandle_t		MutexSemaphore;					//�����ź���
	EventGroupHandle_t 		oEventGroupHandler;				//дEEPROM�¼���־��
	EventGroupHandle_t		ohlw8112_EventHandler;			//���ܼ����¼���־��
	u8						DevSerialnum[25];				//���������к�
	int						socketid;						//���ӱ�ʶ 
	u8						pchUpgradeUrl[100];				//OTA��ַ
	u8						heartcountflag;					//��������flag			
}IcapDevObj;

//Զ�������ṹ��
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

