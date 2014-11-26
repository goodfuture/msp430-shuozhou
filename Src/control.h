


#ifndef _CONTROL_H
#define _CONTROL_H

typedef struct _SWITCH_TIME
{
	INT8U starthour;	//启动时间
	INT8U startmin;
} SWITCH_TIME;

typedef struct _SYS_PARAM
{
    char pw[8];
    char st[4];
    char mn[16];
    char sim[12];
    char rtd[4];            //实时数据上报间隔
    
#if 1
    INT8U worktype;         //是否开启工作方式的远程控制
    INT8U pumpen;           //提升泵是否使能
    INT8U engineen;         //曝气机是否使能
    INT8U valueen;          //电磁阀是否使能
	INT8U backflowen;       //污泥回流泵是否使能
	INT8U reuseen;          //回用泵是否使能
	INT8U levelen;          //液位是否使能
	
	float dohigh;			//风机关闭的do仪值
	float dolow;			//风机开启的do仪值
	INT16U engine_switchlong;	//曝气机切换时间间隔
	INT16U value_switchlong;	//电磁阀切换时间间隔
	INT16U value_switchlast;	//电磁阀切换时长
#endif
} SYS_PARAM;


extern SYS_PARAM Sys_Param;

void ProcControl(void);
void ControlInit(void);

/////////////////////////////
#define WORKTYPE_AUTO   0
#define WORKTYPE_MANUAL 1
#define WORKTYPE_REMOTE 2

#define DEVICE_STOP     0
#define DEVICE_RUN      1
#define DEVICE_FAULT    2
#define DEVICE_DISABLE  3

#define LEVEL_HIGH      0
#define LEVEL_MID       1
#define LEVEL_LOW       2


//void ClearAllControlTimer(void);

INT8U Get_WorkType(void);
INT8U Get_PumpStat(INT8U Index);
INT8U Get_EngineStat(INT8U Index);
INT8U Get_ValueStat(INT8U Index);
INT8U Get_BackflowStat(INT8U Index);
INT8U Get_ReuseStat(INT8U Index);
INT8U Get_LevelStat(INT8U Index);

void Start_Pump(INT8U Index,INT8U Mutex);
void Stop_Pump(INT8U Index);
void Start_Engine(INT8U Index,INT8U Mutex);
void Stop_Engine(INT8U Index);
void Start_Value(INT8U Index,INT8U Mutex);
void Stop_Value(INT8U Index);
void Start_Backflow(INT8U Index,INT8U Mutex);
void Stop_Backflow(INT8U Index);
void Start_Reuse(INT8U Index,INT8U Mutex);
void Stop_Reuse(INT8U Index);

INT8U Get_PumpControlStat(INT8U Index);
INT8U Get_EngineControlStat(INT8U Index);
INT8U Get_ValueControlStat(INT8U Index);
INT8U Get_BackflowControlStat(INT8U Index);
INT8U Get_ReuseControlStat(INT8U Index);

void Start_PumpAuto(void);
void Stop_PumpAuto(void);
void Start_EngineAuto(int IsSwitch);
void Start_AllEngine(void);
void Stop_EngineAuto(void);
void Start_ValueAuto(void);
void Stop_ValueAuto(void);
void Start_BackflowAuto(void);
void Stop_BackflowAuto(void);
void Start_ReuseAuto(void);
void Stop_ReuseAuto(void);

#endif //end of _CONTROL_H
