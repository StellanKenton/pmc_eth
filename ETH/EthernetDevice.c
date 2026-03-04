/**
  ******************************************************************************
  * @file    EthernetDevice.c
  * @author  Stellan
  * @version V1.0
  * @date    2026-01-10
  * @brief   EthernetDevice source file
  ******************************************************************************
  */
/*
*******************************************************************************
*                               Include headers
*******************************************************************************
*/
#include "lwip_comm.h" 
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/init.h"
#include "ethernetif.h" 
#include "lwip/lwip_timers.h"
#include "lwip/tcp_impl.h"
#include "lwip/ip_frag.h"
#include "lwip/tcpip.h" 
#include "malloc.h"
#include "delay.h"
#include "usart.h"  
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "led.h"
#include "netif/ethernetif.h" 

#include "SEGGER_RTT.h"
#include "EthernetDevice.h"
#include "lan8720.h"
#include "lwip/tcp.h"
#include "lwrb.h"
/***************************************************************************/
static uint8_t EthernetRecvData[TCP_RX_BUFFER_SIZE]; // TCP接收缓冲区
static lwrb_t tcpRxBuffer; // TCP接收环形缓冲区

ETHERNET_DeviceTypeDef ethDevice;
struct netif ethlwip_netif;  // Reference the global netif from lwip_comm.c
extern __lwip_dev lwipdev;       // Reference the global lwipdev from lwip_comm.c
extern u32 memp_get_memorysize(void);	
extern u8_t *memp_memory;				
extern u8_t *ram_heap;	

u8 lwip_mem_malloc(void)
{
	u32 mempsize;
	u32 ramheapsize; 
	mempsize=memp_get_memorysize();					//Get memp_memory array size
	memp_memory=mymalloc(SRAMIN,mempsize);	//Allocate memory for memp_memory
	ramheapsize=LWIP_MEM_ALIGN_SIZE(MEM_SIZE)+2*LWIP_MEM_ALIGN_SIZE(4*3)+MEM_ALIGNMENT;//Get ram heap size
	ram_heap=mymalloc(SRAMIN,ramheapsize);	//Allocate memory for ram_heap 
	// TCPIP_THREAD_Task_Handler=mymalloc(SRAMIN,TCPIP_THREAD_STACKSIZE*4);//Allocate stack for core task 
	// LWIP_DHCP_TASK_Handler=mymalloc(SRAMIN,LWIP_DHCP_STK_SIZE*4);				 //Allocate memory space for dhcp task stack
	// if(!memp_memory||!ram_heap||!TCPIP_THREAD_Task_Handler||!TCPIP_THREAD_Task_Handler)//If any allocation fails
	// {
	// 	myfree(SRAMIN,memp_memory);
    //     myfree(SRAMIN,ram_heap);
    //     myfree(SRAMIN,TCPIP_THREAD_Task_Handler);
    //     myfree(SRAMIN,LWIP_DHCP_TASK_Handler);
	// 	return 1;
	// }
	return 0;	
}

void lwip_pkt_handle(void)
{
  //Read received data packets from Ethernet interrupt and send to LWIP core 
 ethernetif_input(&ethlwip_netif);
}

uint8_t EthernetDevice_BspInit(void)
{
    lwrb_init(&tcpRxBuffer, EthernetRecvData, TCP_RX_BUFFER_SIZE);

    if(ETH_Mem_Malloc())return 0;
    SEGGER_RTT_printf(0, "Ethernet memory allocated successfully.\n");
    if(LAN8720_Init())return 0;
    if(lwip_mem_malloc())return 0;
    SEGGER_RTT_printf(0, "LAN8720 initialized successfully.\n");
    return 1;
}

/**
  * @brief  Initialize static IP configuration
  * @retval None
  */
void EthernetDevice_InitStaticIP(void)
{
    uint32_t uid0, uid1, uid2;
    
    // Read STM32 unique ID (96 bits)
    uid0 = *(uint32_t*)(0x1FFF7A10);  // UID[31:0]
    uid1 = *(uint32_t*)(0x1FFF7A14);  // UID[63:32]
    uid2 = *(uint32_t*)(0x1FFF7A18);  // UID[95:64]
    
    // Generate MAC address based on unique ID
    // First 3 bytes use locally administered address identifier (bit1=1, bit0=0 in first byte indicates locally administered unicast address)
    ethDevice.NetInfo.MAC[0] = 0x02;  // Locally administered address
    ethDevice.NetInfo.MAC[1] = 0x00;
    ethDevice.NetInfo.MAC[2] = 0x00;
    // Last 3 bytes use partial bits from unique ID
    ethDevice.NetInfo.MAC[3] = (uid0 >> 16) & 0xFF;
    ethDevice.NetInfo.MAC[4] = (uid1 >> 8) & 0xFF;
    ethDevice.NetInfo.MAC[5] = uid2 & 0xFF;
    
    // Set IP address
    ethDevice.NetInfo.IP[0] = STATIC_IP_ADDR0;
    ethDevice.NetInfo.IP[1] = STATIC_IP_ADDR1;
    ethDevice.NetInfo.IP[2] = STATIC_IP_ADDR2;
    ethDevice.NetInfo.IP[3] = STATIC_IP_ADDR3;
    
    // Set subnet mask
    ethDevice.NetInfo.MASK[0] = STATIC_NETMASK0;
    ethDevice.NetInfo.MASK[1] = STATIC_NETMASK1;
    ethDevice.NetInfo.MASK[2] = STATIC_NETMASK2;
    ethDevice.NetInfo.MASK[3] = STATIC_NETMASK3;
    
    // Set gateway
    ethDevice.NetInfo.GW[0] = STATIC_GW0;
    ethDevice.NetInfo.GW[1] = STATIC_GW1;
    ethDevice.NetInfo.GW[2] = STATIC_GW2;
    ethDevice.NetInfo.GW[3] = STATIC_GW3;

}

/**
  * @brief  Initialize DHCP configuration
  * @retval None
  */
void EthernetDevice_InitDHCP(void)
{
    uint32_t uid0, uid1, uid2;
    
    // Read STM32 unique ID (96 bits)
    uid0 = *(uint32_t*)(0x1FFF7A10);  // UID[31:0]
    uid1 = *(uint32_t*)(0x1FFF7A14);  // UID[63:32]
    uid2 = *(uint32_t*)(0x1FFF7A18);  // UID[95:64]
    
    // Generate MAC address based on unique ID
    // First 3 bytes use locally administered address identifier (bit1=1, bit0=0 in first byte indicates locally administered unicast address)
    ethDevice.NetInfo.MAC[0] = 0x02;  // Locally administered address
    ethDevice.NetInfo.MAC[1] = 0x00;
    ethDevice.NetInfo.MAC[2] = 0x00;
    // Last 3 bytes use partial bits from unique ID
    ethDevice.NetInfo.MAC[3] = (uid0 >> 16) & 0xFF;
    ethDevice.NetInfo.MAC[4] = (uid1 >> 8) & 0xFF;
    ethDevice.NetInfo.MAC[5] = uid2 & 0xFF;
    
    // Set IP address, subnet mask, and gateway to 0 (waiting for DHCP assignment)
    ethDevice.NetInfo.IP[0] = 0;
    ethDevice.NetInfo.IP[1] = 0;
    ethDevice.NetInfo.IP[2] = 0;
    ethDevice.NetInfo.IP[3] = 0;
    
    ethDevice.NetInfo.MASK[0] = 0;
    ethDevice.NetInfo.MASK[1] = 0;
    ethDevice.NetInfo.MASK[2] = 0;
    ethDevice.NetInfo.MASK[3] = 0;
    
    ethDevice.NetInfo.GW[0] = 0;
    ethDevice.NetInfo.GW[1] = 0;
    ethDevice.NetInfo.GW[2] = 0;
    ethDevice.NetInfo.GW[3] = 0;
}




void EthernetDevice_CheckLinkStatus(void)
{
    static uint8_t linkCheckCounter = 0;
    
    // Check link status every 100ms (100ms / 10ms = 10 times)
    linkCheckCounter++;
    if(linkCheckCounter >= 10)
    {
        linkCheckCounter = 0;
        ethDevice.ethLinkStatus = LAN8720_Get_Link_Status();
    }
}

/**
  * @brief  TCP接收回调函数
  * @param  arg: 用户参数
  * @param  tpcb: TCP控制块
  * @param  p: pbuf数据包
  * @param  err: 错误码
  * @retval err_t
  */
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    err_t ret_err;
    
    if (p == NULL) {
        // 客户端关闭连接
        SEGGER_RTT_printf(0, "Client closed connection\n");
        tcp_close(tpcb);
        ethDevice.tcp_client_pcb = NULL;
        ethDevice.state = ETHERNET_CLOSE_TCP_STATE;
        ret_err = ERR_OK;
    }
    else if(err != ERR_OK) {
        // 接收错误
        if (p != NULL) {
            pbuf_free(p);
        }
        ret_err = err;
    }
    else {
        // 接收到数据，更新活动时间
        ethDevice.lastActivityTime = xTaskGetTickCount();
        ethDevice.connectionActive = true;
        
        tcp_recved(tpcb, p->tot_len);
        
        SEGGER_RTT_printf(0, "Received %d bytes: %.*s\n", p->tot_len, p->tot_len, (char*)p->payload);
        
        // 将接收到的数据发送回去
        if(tcp_write(tpcb, p->payload, p->tot_len, TCP_WRITE_FLAG_COPY) == ERR_OK) {
            tcp_output(tpcb);
        }
        
        pbuf_free(p);
        ret_err = ERR_OK;
    }
    
    return ret_err;
}

/**
  * @brief  TCP错误回调函数
  * @param  arg: 用户参数
  * @param  err: 错误码
  * @retval None
  */
static void tcp_server_error(void *arg, err_t err)
{
    SEGGER_RTT_printf(0, "TCP error: %d\n", err);
    ethDevice.tcp_client_pcb = NULL;
    ethDevice.state = ETHERNET_CLOSE_TCP_STATE;
}

/**
  * @brief  TCP连接回调函数
  * @param  arg: 用户参数
  * @param  newpcb: 新的TCP控制块
  * @param  err: 错误码
  * @retval err_t
  */
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    err_t ret_err;
    
    if ((err != ERR_OK) || (newpcb == NULL)) {
        ret_err = ERR_VAL;
        return ret_err;
    }
    
    // 保存客户端连接
    ethDevice.tcp_client_pcb = newpcb;
    
    // 保存远程IP地址
    ethDevice.NetInfo.REMOTEIP[0] = ip4_addr1(&newpcb->remote_ip);
    ethDevice.NetInfo.REMOTEIP[1] = ip4_addr2(&newpcb->remote_ip);
    ethDevice.NetInfo.REMOTEIP[2] = ip4_addr3(&newpcb->remote_ip);
    ethDevice.NetInfo.REMOTEIP[3] = ip4_addr4(&newpcb->remote_ip);
    
    SEGGER_RTT_printf(0, "Client connected from %d.%d.%d.%d:%d\n",
        ip4_addr1(&newpcb->remote_ip),
        ip4_addr2(&newpcb->remote_ip),
        ip4_addr3(&newpcb->remote_ip),
        ip4_addr4(&newpcb->remote_ip),
        newpcb->remote_port);
    
    // 设置接收回调
    tcp_recv(newpcb, tcp_server_recv);
    
    // 设置错误回调
    tcp_err(newpcb, tcp_server_error);
    
    // 初始化连接相关计数器
    ethDevice.lastActivityTime = xTaskGetTickCount();
    ethDevice.connectionIdleTime = 0;
    ethDevice.connectionActive = true;
    
    // 切换到连接状态
    ethDevice.state = ETHERNET_CONNECT_STATE;
    
    ret_err = ERR_OK;
    return ret_err;
}

/**
  * @brief  初始化TCP服务器
  * @retval 0: 失败, 1: 成功
  */
static uint8_t tcp_server_init(void)
{
    struct tcp_pcb *tpcb;
    err_t err;
    
    // 创建TCP控制块
    tpcb = tcp_new();
    if (tpcb == NULL) {
        SEGGER_RTT_printf(0, "Failed to create TCP PCB\n");
        return 0;
    }
    
    // 绑定端口
    err = tcp_bind(tpcb, IP_ADDR_ANY, TCP_SERVER_PORT);
    if (err != ERR_OK) {
        SEGGER_RTT_printf(0, "Failed to bind TCP port %d\n", TCP_SERVER_PORT);
        tcp_close(tpcb);
        return 0;
    }
    
    // 开始监听
    tpcb = tcp_listen(tpcb);
    if (tpcb == NULL) {
        SEGGER_RTT_printf(0, "Failed to listen on TCP port %d\n", TCP_SERVER_PORT);
        return 0;
    }
    
    // 设置接受连接回调
    tcp_accept(tpcb, tcp_server_accept);
    
    // 保存TCP控制块
    ethDevice.tcp_pcb = tpcb;
    
    SEGGER_RTT_printf(0, "TCP server started on port %d\n", TCP_SERVER_PORT);
    
    return 1;
}

void EthernetTCPProcess(void)
{   
    sys_prot_t p;
    struct netif *Netif_Init_Flag;
    struct ip_addr ipaddr;  						//IP address
	struct ip_addr netmask; 						//Subnet mask
	struct ip_addr gw;      						//Default gateway 
    u32 u32ip=0,u32netmask=0,u32gw=0;

    EthernetDevice_CheckLinkStatus();
    switch(ethDevice.state)
    {
        case ETHERNET_DEV_INIT_STATE:
            ethDevice.state = ETHERNET_CHECK_LINK_STATE;
            ethDevice.dhcpEnabled = true; // Default enable DHCP
            break;
        case ETHERNET_CHECK_LINK_STATE:
            if(ethDevice.ethLinkStatus){
                if(ethDevice.dhcpEnabled){
                    EthernetDevice_InitDHCP();
                    IP4_ADDR(&ipaddr, 0, 0, 0, 0);
                    IP4_ADDR(&netmask, 0, 0, 0, 0);
                    IP4_ADDR(&gw, 0, 0, 0, 0);
                    SEGGER_RTT_printf(0, "Ethernet link established, starting DHCP...\n");
                } else {
                    EthernetDevice_InitStaticIP();
                    IP4_ADDR(&ipaddr, ethDevice.NetInfo.IP[0], ethDevice.NetInfo.IP[1], ethDevice.NetInfo.IP[2], ethDevice.NetInfo.IP[3]);
                    IP4_ADDR(&netmask, ethDevice.NetInfo.MASK[0], ethDevice.NetInfo.MASK[1], ethDevice.NetInfo.MASK[2], ethDevice.NetInfo.MASK[3]);
                    IP4_ADDR(&gw, ethDevice.NetInfo.GW[0], ethDevice.NetInfo.GW[1], ethDevice.NetInfo.GW[2], ethDevice.NetInfo.GW[3]);
                }
                
                // Set MAC address in ethlwip_netif
                ethlwip_netif.hwaddr[0] = ethDevice.NetInfo.MAC[0];
                ethlwip_netif.hwaddr[1] = ethDevice.NetInfo.MAC[1];
                ethlwip_netif.hwaddr[2] = ethDevice.NetInfo.MAC[2];
                ethlwip_netif.hwaddr[3] = ethDevice.NetInfo.MAC[3];
                ethlwip_netif.hwaddr[4] = ethDevice.NetInfo.MAC[4];
                ethlwip_netif.hwaddr[5] = ethDevice.NetInfo.MAC[5];
                
                SEGGER_RTT_printf(0, "MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                    ethlwip_netif.hwaddr[0], ethlwip_netif.hwaddr[1], ethlwip_netif.hwaddr[2],
                    ethlwip_netif.hwaddr[3], ethlwip_netif.hwaddr[4], ethlwip_netif.hwaddr[5]);
                
                //Initialize tcp ip core, this function will create tcpip_thread core task
                tcpip_init(NULL,NULL);		
                p=sys_arch_protect();   //Enter critical section
                Netif_Init_Flag=netif_add(&ethlwip_netif,&ipaddr,&netmask,&gw,NULL,&ethernetif_init,&tcpip_input);//Add a network interface to the network interface list
                sys_arch_unprotect(p);  //Exit critical section
                if(Netif_Init_Flag==NULL){ //Network interface addition failed 
                    ethDevice.state = ETHERNET_DEV_INIT_STATE; // Reset to initial state to retry initialization
                    SEGGER_RTT_printf(0, "Failed to add network interface, retrying initialization...\n");
                    break;
                } else {//Network interface added successfully, set netif as default and bring netif up
                    netif_set_default(&ethlwip_netif); //Set netif as default network interface
                    netif_set_up(&ethlwip_netif);		//Bring netif up
                }
                if(ethDevice.dhcpEnabled){
                    dhcp_start(&ethlwip_netif);    //Start DHCP
                    ethDevice.state = ETHERNET_DHCP_STATE;
                    SEGGER_RTT_printf(0, "DHCP started, waiting for IP address assignment...\n");
                } else {
                    ethDevice.state = ETHERNET_TCP_STATE;
                    SEGGER_RTT_printf(0, "Static IP configured, entering TCP processing state...\n");
                }
            }
            break;
        case ETHERNET_DHCP_STATE:
            // DHCP处理逻辑
            u32ip=ethlwip_netif.ip_addr.addr;
            u32netmask=ethlwip_netif.netmask.addr;
            u32gw=ethlwip_netif.gw.addr;
            if(u32ip != 0){  // 只要IP不为0就认为DHCP成功
                // 填充DHCP获取的网络信息到ethDevice.NetInfo
                ethDevice.NetInfo.IP[0] = (u32ip >> 0) & 0xFF;
                ethDevice.NetInfo.IP[1] = (u32ip >> 8) & 0xFF;
                ethDevice.NetInfo.IP[2] = (u32ip >> 16) & 0xFF;
                ethDevice.NetInfo.IP[3] = (u32ip >> 24) & 0xFF;
                
                ethDevice.NetInfo.MASK[0] = (u32netmask >> 0) & 0xFF;
                ethDevice.NetInfo.MASK[1] = (u32netmask >> 8) & 0xFF;
                ethDevice.NetInfo.MASK[2] = (u32netmask >> 16) & 0xFF;
                ethDevice.NetInfo.MASK[3] = (u32netmask >> 24) & 0xFF;
                
                ethDevice.NetInfo.GW[0] = (u32gw >> 0) & 0xFF;
                ethDevice.NetInfo.GW[1] = (u32gw >> 8) & 0xFF;
                ethDevice.NetInfo.GW[2] = (u32gw >> 16) & 0xFF;
                ethDevice.NetInfo.GW[3] = (u32gw >> 24) & 0xFF;
                
                ethDevice.state = ETHERNET_TCP_STATE;
                SEGGER_RTT_printf(0, "DHCP successful!\n");
                SEGGER_RTT_printf(0, "IP: %d.%d.%d.%d\n", 
                    ethDevice.NetInfo.IP[0], ethDevice.NetInfo.IP[1], 
                    ethDevice.NetInfo.IP[2], ethDevice.NetInfo.IP[3]);
                SEGGER_RTT_printf(0, "MASK: %d.%d.%d.%d\n", 
                    ethDevice.NetInfo.MASK[0], ethDevice.NetInfo.MASK[1], 
                    ethDevice.NetInfo.MASK[2], ethDevice.NetInfo.MASK[3]);
                SEGGER_RTT_printf(0, "GW: %d.%d.%d.%d\n", 
                    ethDevice.NetInfo.GW[0], ethDevice.NetInfo.GW[1], 
                    ethDevice.NetInfo.GW[2], ethDevice.NetInfo.GW[3]);
            }
            else if(ethlwip_netif.dhcp != NULL && ethlwip_netif.dhcp->tries > LWIP_MAX_DHCP_TRIES) 
            {
                // DHCP超时，使用静态IP
                SEGGER_RTT_printf(0, "DHCP timeout, using static IP...\n");
                dhcp_stop(&ethlwip_netif);
                
                // 设置静态IP
                EthernetDevice_InitStaticIP();
                IP4_ADDR(&(ethlwip_netif.ip_addr), ethDevice.NetInfo.IP[0], ethDevice.NetInfo.IP[1], 
                         ethDevice.NetInfo.IP[2], ethDevice.NetInfo.IP[3]);
                IP4_ADDR(&(ethlwip_netif.netmask), ethDevice.NetInfo.MASK[0], ethDevice.NetInfo.MASK[1], 
                         ethDevice.NetInfo.MASK[2], ethDevice.NetInfo.MASK[3]);
                IP4_ADDR(&(ethlwip_netif.gw), ethDevice.NetInfo.GW[0], ethDevice.NetInfo.GW[1], 
                         ethDevice.NetInfo.GW[2], ethDevice.NetInfo.GW[3]);
                
                ethDevice.state = ETHERNET_TCP_STATE;
                SEGGER_RTT_printf(0, "Static IP: %d.%d.%d.%d\n", 
                    ethDevice.NetInfo.IP[0], ethDevice.NetInfo.IP[1], 
                    ethDevice.NetInfo.IP[2], ethDevice.NetInfo.IP[3]);
            }
            break;
        case ETHERNET_TCP_STATE:
            // 初始化TCP服务器
            if(tcp_server_init()) {
                ethDevice.state = ETHERNET_WAIT_TCP_STATE;
                SEGGER_RTT_printf(0, "Waiting for TCP client connection...\n");
            } else {
                SEGGER_RTT_printf(0, "Failed to initialize TCP server, retrying...\n");
                vTaskDelay(1000 / portTICK_PERIOD_MS); // 延迟1秒后重试
            }
            break;
        case ETHERNET_WAIT_TCP_STATE:
            // 等待TCP客户端连接
            // 此状态下TCP服务器已启动，等待客户端连接
            // 连接建立后会通过tcp_server_accept回调自动切换到ETHERNET_CONNECT_STATE
            break;
        case ETHERNET_CONNECT_STATE:
            // TCP连接已建立，处理数据收发
            // 数据接收和发送在tcp_server_recv回调中处理
            // 如果连接断开，会自动切换到ETHERNET_CLOSE_TCP_STATE
            if(ethDevice.tcp_client_pcb == NULL) {
                // 连接已断开，返回等待连接状态
                ethDevice.state = ETHERNET_WAIT_TCP_STATE;
                SEGGER_RTT_printf(0, "Connection lost, waiting for new connection...\n");
            }
            break;
        case ETHERNET_CLOSE_TCP_STATE:
            // 关闭TCP连接
            if(ethDevice.tcp_client_pcb != NULL) {
                struct tcp_pcb *tpcb = (struct tcp_pcb *)ethDevice.tcp_client_pcb;
                tcp_close(tpcb);
                ethDevice.tcp_client_pcb = NULL;
                SEGGER_RTT_printf(0, "TCP connection closed\n");
            }
            // 返回等待连接状态
            ethDevice.state = ETHERNET_WAIT_TCP_STATE;
            break;
        default:
            break;
    }
}





/******************End of File*************************/
