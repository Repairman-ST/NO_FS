#ifndef   __BSP_MQ2_H
#define   __BSP_MQ2_H

/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/

#include "stm32f4xx.h"


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/


#define   HIGH_LEVEL_VALUE               2700

typedef enum {
	MQ2_High_Level = 0x00,
	MQ2_Normal_Level = 0x01
} MQ2_State;

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/


MQ2_State   BSP_MQ2_State              (void);


#endif    //__BSP_MQ2_H





