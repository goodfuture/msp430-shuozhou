#ifndef _PRODATA_H
#define _PRODATA_H

typedef struct _MAX_MIN_DATA {
	float mnt_min;
	float mnt_max;
	double mnt_sum;
	float mnt_avg;
	double mnt_cou;
	long mnt_cnt;

	float hour_min;
	float hour_max;
	double hour_sum;
	float hour_avg;
	double hour_cou;
	long hour_cnt;

	float day_min;
	float day_max;
	double day_sum;
	float day_avg;
	double day_cou;
	long day_cnt;
} MAX_MIN_DATA;

void ClearStatisData(int Cmd);
void ProcData(void);

////////////////////////////////

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned int u16;
typedef signed int s16;
typedef unsigned long u32;

int str2TM(char *str, T_Times *tm);

void resetFlashIndexPage(void);

void saveRtdData(T_Times *tm, char *dat);
void saveMtdData(T_Times *tm, char *dat);
void saveHtdData(T_Times *tm, char *dat);
void saveDtdData(T_Times *tm, char *dat);

int checkoutRtdData(T_Times *start, char *buf);
int checkoutMtdData(T_Times *start, T_Times *end, char *buf);
int checkoutHtdData(T_Times *start, T_Times *end, char *buf);
int checkoutDtdData(T_Times *start, T_Times *end, char *buf);

#endif
