/**********************************************************************************
* Copyright (c) 2008-2011  Beijing Autelan Technology Co. Ltd.
* All rights reserved.
*
* filename: syslogService.h
* description:  implementation for the header file of syslogService.c.
* 
*
* 
************************************************************************************/

#ifndef SYSLOGSERVICE_H
#define SYSLOGSERVICE_H

/*
*---------add by piyongping 2008-10-30---------------------------------
*/
#if 1
#define SYSLOGENABLE		1
#define SYSLOGMSGSEND		2
#define SYSLOGMSGLEVEL		3
#define SYSLOGINDEX		4
#define SYSLOGSERVERPATH		5
#define SYSLOGLEVEL		6
#define SYSLOGFACILITY		7
#define SYSLOGCONFINFO		8
#define SYSLOGUPDATAINTERVALTIME		9
#define SYSLOGSAVELOGBAKPATH		10
#define SYSLOGSERVERPORT		11

#endif


// function declarations 
    void init_syslogService(void);
    
    	FindVarMethod var_syslogService;
		FindVarMethod var_wapiSyslogSettingTable ;
#if 1
//add by piyongping 2008-10-30
		WriteMethod write_syslogEnable;
		WriteMethod write_syslogMsgSend;
		WriteMethod	write_syslogMsgLevel;
		WriteMethod write_syslogServerPath;
    	WriteMethod write_syslogLevel;
    	WriteMethod write_syslogFacility;
		WriteMethod write_syslogConfInfo;
		WriteMethod write_syslogServerPort;
		WriteMethod	write_cityhotspotsname;
		WriteMethod write_apattributionac;
		WriteMethod write_syslogUpdataIntervalTime;
		WriteMethod write_syslogSaveLogBakPath;
#endif




/*
*--------add by piyongping 2008-10-30-------------
*/
#if 1

//priority of syslog
static char *syslog_Alarm_Level[] = { 
		"emerg", 
		"alert", 
		"crit", 
		"err", 
		"warning", 
		"notice", 
		"info", 
		"debug" 
};

static char *facility_Names[] = {
	"*",		  	//��ʾ�����豸��Դ
	"auth",       	//��֤ϵͳ��login��su��getty�� 
	"authpriv",   	//ͬLOG_AUTH����ֻ��¼����ѡ��ĵ����û��ɶ����ļ��� 
	"cron",       	//�ػ�����     
	"daemon",     	//����ϵͳ�ػ����̣���routed  
	"kern",       	//�ں˲�������Ϣ
	"security",   	//��ȫԭ��
	"syslog",   	//��syslogd��8���������ڲ���Ϣ     
	"user",    		//����û����̲�������Ϣ 
	"local0",  		//Ϊ����ʹ�ñ��� 
	"local1"  		//Ϊ����ʹ�ñ��� 
};

struct syslogConfpara{
    long dEnable;	
    char strLevel[20];
    char strPath[64];
	char strFacility[20];
    char strMsg[128];
    long dMsgLevel;
}syslogConfpara;

typedef struct syslogConf{
	char dataconf[104];
	struct syslogConf *next;
}syslogConf;
#endif


#if 1
void loadSyslogPara( void );
int loadSyslogConf( const char *strFileName, char *fac , char *level,char *path);
int loadSyslogConf1( const char *strFileName, struct syslogConf **head);
int  syslogScript( char *strType );
long getSyslogLevel( const char *strLevel );
int sendLoggerMsg( int iLevel, char *strMsg );
void free_Syslog_table( struct syslogConf **head);
char *readSyslogFlag(void);
int creatSyslogConf( const char *strFileName);
int copySyslogConf( char *srcPath,  char *desPath );
int saveSyslogConf1(const char *strFileName,char *syslogType,char *key,int index);
int showSyslogConfInfo( struct syslogConf *head,char *fac,char *level,char *path );
int getSyslogFacility( const char *strIndex );
int killallSyslogd( void );
void do_restart_syslog();
#endif


#endif // SYSLOGSERVICE_H 

