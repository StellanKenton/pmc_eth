/**
  ******************************************************************************
  * @file    Iot_Ethernet.h
  * @author  Stellan
  * @version V1.0
  * @date    2026-01-10
  * @brief   Iot_Ethernet header file
  ******************************************************************************
  */
/*
*******************************************************************************
*                               Include headers
*******************************************************************************
*/
#ifndef __IOT_ETHERNET_H
#define __IOT_ETHERNET_H

#include "stm32f4xx.h"
#include <stdbool.h>
#include <stdint.h>
/***************************************************************************/

typedef enum {
    ETHERNET_INIT_STATE = 0,
    ETHERNET_READY_STATE,
    ETHERNET_FAILED_STATE,
}ETHERNET_STATE;


typedef struct {
    ETHERNET_STATE state;
}ETHERNET_ModuleTypeDef;


ETHERNET_STATE Ethernet_GetState(void);
void Ethernet_HardWareInit(void);
void Ethernet_HardWareProcess(void);









#endif
/******************End of File*************************/
