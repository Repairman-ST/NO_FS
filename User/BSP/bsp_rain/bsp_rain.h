#ifndef   __BSP_RAIN_H
#define   __BSP_RAIN_H

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


// 使用PF9作为长按键，以进入设置模式
#define      RAIN_PIN             	      GPIO_Pin_2

#define      RAIN_GPIO_PORT               GPIOE
#define      RAIN_GPIO_CLK                RCC_AHB1Periph_GPIOE                // 端口时钟                  

#define      RAIN_PIN_RD()	             (RAIN_GPIO_PORT->IDR & RAIN_PIN) 	  //读取IO脚状态返回0或非0		



typedef enum {
	Raining = 0x00,           // 表示在下雨  
	No_Rain = 0x01,           // 表示未下雨
}Rain_Status;
      





/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void          BSP_Rain_Init                (void);

Rain_Status   BSP_Rain_State              (void);


#endif    //__BSP_RAIN_H


