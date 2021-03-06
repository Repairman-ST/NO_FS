
#ifndef      __INFO_MANAGER_H
#define      __INFO_MANAGER_H

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include "stm32f4xx.h"



/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

typedef struct{
	uint8_t     wifi_pwd[32];                    // wifi密码长度最多30一个用于保存结束符
	uint8_t     wifi_ssid[32];                   // wifi名字长度最多30一个用于保存结束符
	uint8_t     phone[32][13];                   // 最多32条记录，留一个字符保存结束符号
}InfoStruct;



/*
*********************************************************************************************************
*                                               FUNCTIONS
*********************************************************************************************************
*/

void phone_clean(InfoStruct *buf);
void phone_del(InfoStruct *buf, uint8_t index);
uint8_t phone_get(InfoStruct *buf, uint8_t *body, uint8_t index);
uint8_t phone_add(InfoStruct *buf, uint8_t *body, uint8_t phoneLen);


uint8_t wifi_getPWD(InfoStruct *buf, uint8_t *pwd);
uint8_t wifi_getSSID(InfoStruct *buf, uint8_t *ssid);
void wifi_setPWD(InfoStruct *buf, uint8_t *pwd, uint8_t len);
void wifi_setSSID(InfoStruct *buf, uint8_t *ssid, uint8_t len);





#endif









