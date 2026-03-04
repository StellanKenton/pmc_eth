/**
  ******************************************************************************
  * @file    EthernetDevice.h
  * @author  Stellan
  * @version V1.0
  * @date    2026-01-10
  * @brief   EthernetDevice header file
  ******************************************************************************
  */
/*
*******************************************************************************
*                               Include headers
*******************************************************************************
*/
#ifndef __ETHERNET_DEVICE_H
#define __ETHERNET_DEVICE_H

#include "stm32f4xx.h"
#include <stdbool.h>
#include <stdint.h>
/***************************************************************************/
#define Ethernet_taskPeriod  10

// Static IP configuration
#define STATIC_IP_ADDR0   192
#define STATIC_IP_ADDR1   168
#define STATIC_IP_ADDR2   8
#define STATIC_IP_ADDR3   18

#define STATIC_NETMASK0   255
#define STATIC_NETMASK1   255
#define STATIC_NETMASK2   255
#define STATIC_NETMASK3   0

#define STATIC_GW0        192
#define STATIC_GW1        168
#define STATIC_GW2        8
#define STATIC_GW3        1


typedef enum {
    ETHERNET_DEV_INIT_STATE = 0,
    ETHERNET_CHECK_LINK_STATE,
    ETHERNET_DHCP_STATE,
    ETHERNET_TCP_STATE,
    ETHERNET_WORKING_STATE,
    ETHERNET_DEV_FAILED_STATE,
}EthDevice_State;

typedef struct {
    uint8_t MAC[6];
    uint8_t IP[4];
    uint8_t MASK[4];
    uint8_t GW[4];
    uint8_t REMOTEIP[4];
}EthNetInfo_TypeDef;



typedef struct {
    EthDevice_State state;
    EthNetInfo_TypeDef NetInfo;
    bool dhcpEnabled;
    bool ethLinkStatus;
}ETHERNET_DeviceTypeDef;


void EthernetDevice_Init(void);
uint8_t EthernetDevice_BspInit(void);
void EthernetDevice_InitStaticIP(void);
void EthernetDevice_InitDHCP(void);









#endif
/******************End of File*************************/
