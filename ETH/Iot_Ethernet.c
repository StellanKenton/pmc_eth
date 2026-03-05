/**
  ******************************************************************************
  * @file    Iot_Ethernet.c
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
#include "Iot_Ethernet.h"
#include "EthernetDevice.h"
#include "SEGGER_RTT.h"
/***************************************************************************/
ETHERNET_ModuleTypeDef g_EthernetModuleInfo;

ETHERNET_STATE Ethernet_GetState(void)
{
    return g_EthernetModuleInfo.state;
}

void Ethernet_HardWareInit(void)
{
    g_EthernetModuleInfo.state = ETHERNET_INIT_STATE;
}

void Ethernet_HardWareProcess(void)
{
    switch(g_EthernetModuleInfo.state)
    {
        case ETHERNET_INIT_STATE:
            if(1){
                g_EthernetModuleInfo.state = ETHERNET_READY_STATE;
                SEGGER_RTT_printf(0, "Ethernet hardware initialized successfully.\n");
            } else {
                g_EthernetModuleInfo.state = ETHERNET_FAILED_STATE;
                SEGGER_RTT_printf(0, "Ethernet hardware initialization failed.\n");
            }
            break;
        case ETHERNET_READY_STATE:
            break;
        case ETHERNET_FAILED_STATE:
            break;
        default:
            break;
    }    
}




 







/******************End of File*************************/
