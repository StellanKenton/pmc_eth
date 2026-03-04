#ifndef _LWIP_COMM_H
#define _LWIP_COMM_H 
#include "lan8720.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//This code is for learning purposes only, not for commercial use
//ALIENTEK STM32F407 development board
//lwip common driver code	   
//Original author: @ALIENTEK
//Forum: www.openedv.com
//Creation date: 2014/8/15
//Version: V1.0
//Copyright reserved, piracy prohibited
//Copyright(C) Guangzhou Xingyi Electronic Technology Co., Ltd. 2009-2019
//All rights reserved									  
//*******************************************************************************
//Modification history
//None
////////////////////////////////////////////////////////////////////////////////// 	   
 

#define LWIP_MAX_DHCP_TRIES		4   //Maximum number of DHCP retries
   

//lwip control structure
typedef struct  
{
	u8 mac[6];      //MAC address
	u8 remoteip[4];	//Remote host IP address 
	u8 ip[4];       //Local IP address
	u8 netmask[4]; 	//Subnet mask
	u8 gateway[4]; 	//Default gateway IP address
	
	vu8 dhcpstatus;	//dhcp status 
					//0, DHCP address not obtained;
					//1, DHCP obtaining in progress
					//2, DHCP address obtained successfully
					//0XFF, obtaining failed.
}__lwip_dev;
extern __lwip_dev lwipdev;	//lwip control structure
 
void lwip_pkt_handle(void);
void lwip_comm_default_ip_set(__lwip_dev *lwipx);
u8 lwip_comm_mem_malloc(void);
void lwip_comm_mem_free(void);
u8 lwip_comm_init(void);
void lwip_comm_dhcp_creat(void);
void lwip_comm_dhcp_delete(void);
void lwip_comm_destroy(void);
void lwip_comm_delete_next_timeout(void);

#endif
