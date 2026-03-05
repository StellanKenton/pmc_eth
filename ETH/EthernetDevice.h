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
#define LWIP_MAX_DHCP_TRIES  10  // Maximum DHCP retry attempts (increased for better reliability)

// TCP连接超时配置
#define TCP_WAIT_CONNECT_TIMEOUT    300000  // 等待连接超时时间(ms) - 5分钟
#define TCP_CONNECTION_IDLE_TIMEOUT 60000   // 连接空闲超时时间(ms) - 1分钟
#define TCP_KEEPALIVE_INTERVAL      10000   // 心跳间隔(ms) - 10秒

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

#define TCP_SERVER_PORT  8080  // TCP服务器端口
#define TCP_RX_BUFFER_SIZE 4096 // 接收缓冲区大小
#define TCP_TX_BUFFER_SIZE 1024 // 发送缓冲区大小


typedef enum {
    ETHERNET_DEV_INIT_STATE = 0,
    ETHERNET_CHECK_LINK_STATE,
    ETHERNET_DHCP_STATE,
    ETHERNET_TCP_STATE,
    ETHERNET_WAIT_TCP_STATE,
    ETHERNET_CONNECT_STATE,
    ETHERNET_CLOSE_TCP_STATE,
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
    void *tcp_pcb;          // TCP控制块指针
    void *tcp_client_pcb;   // TCP客户端连接控制块指针
    uint32_t waitConnectTimeout;    // 等待连接超时计数器
    uint32_t connectionIdleTime;    // 连接空闲时间计数器
    uint32_t lastActivityTime;      // 最后活动时间
    bool connectionActive;          // 连接活跃标志
    volatile bool client_connected_flag;
    volatile bool client_closed_flag;
}ETHERNET_DeviceTypeDef;


void EthernetDevice_Init(void);
uint8_t EthernetDevice_BspInit(void);
void EthernetDevice_InitStaticIP(void);
void EthernetDevice_InitDHCP(void);
void EthernetTCPProcess(void);
void lwip_pkt_handle(void);
void lwip_mem_free(void);







#endif
/******************End of File*************************/
