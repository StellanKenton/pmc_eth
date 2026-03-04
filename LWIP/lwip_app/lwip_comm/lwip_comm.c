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
//This is the LWIP common file 
   
__lwip_dev lwipdev;							 
struct netif lwip_netif;				

extern u32 memp_get_memorysize(void);	
extern u8_t *memp_memory;				
extern u8_t *ram_heap;					

//u32 TCPTimer=0;			//TCP polling timer
//u32 ARPTimer=0;			//ARP polling timer
//u32 lwip_localtime;		//lwip local time counter, unit: ms

//lwip core task and DHCP task

//lwip core task stack priority and stack size are defined in lwipopts.h
TaskHandle_t  TCPIP_THREAD_Task_Handler;

//lwip DHCP task
//Task priority
#define LWIP_DHCP_TASK_PRIO		7
//Task stack size	
#define LWIP_DHCP_STK_SIZE 		128  
//Task handle (task stack) allocated from memory pool
TaskHandle_t  LWIP_DHCP_TASK_Handler;
//Task function
void lwip_dhcp_task(void *pvParameters);

#if LWIP_DHCP
u32 DHCPfineTimer=0;	//DHCP fine timer
u32 DHCPcoarseTimer=0;	//DHCP coarse timer
#endif

//Ethernet interrupt handler


//lwip core layer parameters
//Allocate memory for lwip mem and memp
//Return value: 0, success;
//              other, failure
u8 lwip_comm_mem_malloc(void)
{
	u32 mempsize;
	u32 ramheapsize; 
	mempsize=memp_get_memorysize();					//Get memp_memory array size
	memp_memory=mymalloc(SRAMIN,mempsize);	//Allocate memory for memp_memory
	ramheapsize=LWIP_MEM_ALIGN_SIZE(MEM_SIZE)+2*LWIP_MEM_ALIGN_SIZE(4*3)+MEM_ALIGNMENT;//Get ram heap size
	ram_heap=mymalloc(SRAMIN,ramheapsize);	//Allocate memory for ram_heap 
	TCPIP_THREAD_Task_Handler=mymalloc(SRAMIN,TCPIP_THREAD_STACKSIZE*4);//Allocate stack for core task 
	LWIP_DHCP_TASK_Handler=mymalloc(SRAMIN,LWIP_DHCP_STK_SIZE*4);				 //Allocate memory space for dhcp task stack
	if(!memp_memory||!ram_heap||!TCPIP_THREAD_Task_Handler||!TCPIP_THREAD_Task_Handler)//If any allocation fails
	{
		lwip_comm_mem_free();
		return 1;
	}
	return 0;	
}
//Free lwip mem and memp memory
void lwip_comm_mem_free(void)
{ 	
	myfree(SRAMIN,memp_memory);
	myfree(SRAMIN,ram_heap);
	myfree(SRAMIN,TCPIP_THREAD_Task_Handler);
	myfree(SRAMIN,LWIP_DHCP_TASK_Handler);
}
//lwip default IP settings
//lwipx: pointer to lwip control structure
void lwip_comm_default_ip_set(__lwip_dev *lwipx)
{
	u32 sn0;
	sn0=*(vu32*)(0x1FFF7A10);//Read STM32 unique ID first 24 bits as MAC address high bytes
	//Default remote IP: 192.168.1.100
	lwipx->remoteip[0]=192;	
	lwipx->remoteip[1]=168;
	lwipx->remoteip[2]=8;
	lwipx->remoteip[3]=1;
	//MAC address setting (high bytes fixed as: 2.0.0, low bytes from STM32 unique ID)
	lwipx->mac[0]=2;//High bytes (IEEE OUI - Organizationally Unique Identifier) fixed as: 2.0.0
	lwipx->mac[1]=0;
	lwipx->mac[2]=0;
	lwipx->mac[3]=(sn0>>16)&0XFF;//Low bytes from STM32 unique ID
	lwipx->mac[4]=(sn0>>8)&0XFFF;;
	lwipx->mac[5]=sn0&0XFF; 
	//Default local IP: 192.168.1.30
	lwipx->ip[0]=192;		
	lwipx->ip[1]=168;
	lwipx->ip[2]=8;
	lwipx->ip[3]=30;
	//Default subnet mask: 255.255.255.0
	lwipx->netmask[0]=255;	
	lwipx->netmask[1]=255;
	lwipx->netmask[2]=255;
	lwipx->netmask[3]=0;
	//Default gateway: 192.168.1.1
	lwipx->gateway[0]=192;	
	lwipx->gateway[1]=168;
	lwipx->gateway[2]=8;
	lwipx->gateway[3]=1;	
	lwipx->dhcpstatus=0;//No DHCP	
} 

//LWIP initialization (used when LWIP runs with OS)
//Return value: 0, success
//              1, memory error
//              2, LAN8720 initialization failed
//              3, network interface addition failed
u8 lwip_comm_init(void)
{
	sys_prot_t p;
	struct netif *Netif_Init_Flag;			//Save netif_add() return value to check if network initialization succeeded
	struct ip_addr ipaddr;  						//IP address
	struct ip_addr netmask; 						//Subnet mask
	struct ip_addr gw;      						//Default gateway 
	if(ETH_Mem_Malloc())return 1;				//Memory allocation failed
	if(lwip_comm_mem_malloc())return 1;	//Memory allocation failed
	if(LAN8720_Init())return 2;					//LAN8720 initialization failed 
	tcpip_init(NULL,NULL);							//Initialize tcp ip core, this function will create tcpip_thread core task
	lwip_comm_default_ip_set(&lwipdev);	//Set default IP information

#if LWIP_DHCP		//Use dynamic IP
	ipaddr.addr = 0;
	netmask.addr = 0;
	gw.addr = 0;
#else				//Use static IP
	IP4_ADDR(&ipaddr,lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
	IP4_ADDR(&netmask,lwipdev.netmask[0],lwipdev.netmask[1] ,lwipdev.netmask[2],lwipdev.netmask[3]);
	IP4_ADDR(&gw,lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
	printf("static mac:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
	printf("static ip........................%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
	printf("static mask.........................%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
	printf("static gateway..........................%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
#endif
	p=sys_arch_protect();   //Enter critical section
	Netif_Init_Flag=netif_add(&lwip_netif,&ipaddr,&netmask,&gw,NULL,&ethernetif_init,&tcpip_input);//Add a network interface to the network interface list
	sys_arch_unprotect(p);  //Exit critical section
	
	if(Netif_Init_Flag==NULL)return 3;//Network interface addition failed 
	else//Network interface added successfully, set netif as default and bring netif up
	{
		netif_set_default(&lwip_netif); //Set netif as default network interface
		netif_set_up(&lwip_netif);		//Bring netif up
	}
	return 0;//Return OK.
} 

//If using DHCP
#if LWIP_DHCP			
//Create DHCP task
void lwip_comm_dhcp_creat(void)
{
	taskENTER_CRITICAL();      			//Enter critical section
	//Create DHCP task 
	xTaskCreate((TaskFunction_t )lwip_dhcp_task,     	
              (const char*    )"lwip_dhcp_task",   	
              (uint16_t       )LWIP_DHCP_STK_SIZE, 
              (void*          )NULL,				
              (UBaseType_t    )LWIP_DHCP_TASK_PRIO,	
              (TaskHandle_t*  )&LWIP_DHCP_TASK_Handler);  
	taskEXIT_CRITICAL();            //Exit critical section
}
//Delete DHCP task
void lwip_comm_dhcp_delete(void)
{
	dhcp_stop(&lwip_netif); 		    		 //Stop DHCP
	vTaskDelete(LWIP_DHCP_TASK_Handler); //Delete DHCP task
}
//DHCP task function
void lwip_dhcp_task(void *pvParameters)
{
	u32 ip=0,netmask=0,gw=0;
	dhcp_start(&lwip_netif);    //Start DHCP
	lwipdev.dhcpstatus = 0;     //Obtaining DHCP 
	printf("DHCP is starting...\r\n");  	
	while(1)
	{
		printf("DHCP is running...\r\n");
		ip=lwip_netif.ip_addr.addr;			//Get obtained IP address
		netmask=lwip_netif.netmask.addr;//Get subnet mask
		gw=lwip_netif.gw.addr;					//Get default gateway 
		if(ip!=0)												//When IP address is successfully obtained
		{
			lwipdev.dhcpstatus=2;	//DHCP successful
			printf("DHCP MAC:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
		  //Display IP address obtained via DHCP
			lwipdev.ip[3]=(uint8_t)(ip>>24); 
			lwipdev.ip[2]=(uint8_t)(ip>>16);
			lwipdev.ip[1]=(uint8_t)(ip>>8);
			lwipdev.ip[0]=(uint8_t)(ip);
			printf("DHCP IP ADRESS..............%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			//Display subnet mask obtained via DHCP
			lwipdev.netmask[3]=(uint8_t)(netmask>>24);
			lwipdev.netmask[2]=(uint8_t)(netmask>>16);
			lwipdev.netmask[1]=(uint8_t)(netmask>>8);
			lwipdev.netmask[0]=(uint8_t)(netmask);
			printf("DHCP NETWASK............%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			//Display default gateway obtained via DHCP
			lwipdev.gateway[3]=(uint8_t)(gw>>24);
			lwipdev.gateway[2]=(uint8_t)(gw>>16);
			lwipdev.gateway[1]=(uint8_t)(gw>>8);
			lwipdev.gateway[0]=(uint8_t)(gw);
			printf("DHCP GATEWAY..........%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			break;
		}else if(lwip_netif.dhcp->tries>LWIP_MAX_DHCP_TRIES) //Failed to obtain IP address via DHCP and exceeded max retries
		{
			lwipdev.dhcpstatus=0XFF;//DHCP timeout failed.
			//Use static IP address
			IP4_ADDR(&(lwip_netif.ip_addr),lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			IP4_ADDR(&(lwip_netif.netmask),lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			IP4_ADDR(&(lwip_netif.gw),lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			printf("DHCP is sucessful!\r\n");
			printf("MAC::................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
			printf("IP::........................%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			printf("NETMASK::..........................%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			printf("GATEWAY::..........................%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			break;
		}
		delay_xms(250); //Delay 250ms
	}
	lwip_comm_dhcp_delete(); //Delete DHCP task 
}
#endif 
