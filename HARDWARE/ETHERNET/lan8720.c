#include "lan8720.h"
#include "stm32f4x7_eth.h"
#include "usart.h" 
#include "delay.h"
#include "malloc.h" 

ETH_DMADESCTypeDef *DMARxDscrTab;	//Ethernet DMA receive descriptor data structure pointer
ETH_DMADESCTypeDef *DMATxDscrTab;	//Ethernet DMA transmit descriptor data structure pointer 
uint8_t *Rx_Buff; 					//Ethernet underlying receive buffers pointer 
uint8_t *Tx_Buff; 					//Ethernet underlying transmit buffers pointer
  
static void ETHERNET_NVICConfiguration(void);
//LAN8720 initialization
//Return value: 0, success;
//              other, failure
u8 LAN8720_Init(void)
{
	u8 rval=0;
	GPIO_InitTypeDef GPIO_InitStructure;
  
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOG, ENABLE);//Enable GPIO clock for RMII interface
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);   //Enable SYSCFG clock
  
	SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_RMII); //Use RMII interface between MAC and PHY

	/*Configure Ethernet pins for RMII interface
	  ETH_MDIO -------------------------> PA2
	  ETH_MDC --------------------------> PC1
	  ETH_RMII_REF_CLK------------------> PA1
	  ETH_RMII_CRS_DV ------------------> PA7
	  ETH_RMII_RXD0 --------------------> PC4
	  ETH_RMII_RXD1 --------------------> PC5
	  ETH_RMII_TX_EN -------------------> PG11
	  ETH_RMII_TXD0 --------------------> PG13
	  ETH_RMII_TXD1 --------------------> PG14
	  ETH_RESET-------------------------> PD3*/
					
	  //Configure PA1 PA2 PA7
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;  
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_ETH); //Configure alternate function for the pin
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_ETH);

	//Configure PC1, PC4 and PC5
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource1, GPIO_AF_ETH); //Configure alternate function for the pin
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_ETH);
                                
	//Configure PG11, PG14 and PG13 
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_ETH);
    
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_ETH);
	
	//Configure PD3 as output pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	//Push-pull output
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;  
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	LAN8720_RST=0;					//Hardware reset LAN8720
	delay_ms(50);	
	LAN8720_RST=1;				 	//Reset complete 
	ETHERNET_NVICConfiguration();	//Configure interrupt priority
	rval=ETH_MACDMA_Config();		//Configure MAC and DMA
	return !rval;					//ETH return value: 0=failure; 1=success; need to invert 
}

//Ethernet interrupt service configuration
void ETHERNET_NVICConfiguration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = ETH_IRQn;  //Ethernet interrupt
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0X00;  //Interrupt group 2, preemption priority
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0X00;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


//Get LAN8720 speed mode
//Return value:
//001: 10M half-duplex
//101: 10M full-duplex
//010: 100M half-duplex
//110: 100M full-duplex
//Other: error
u8 LAN8720_Get_Speed(void)
{
	u8 speed;
	speed=((ETH_ReadPHYRegister(0x00,31)&0x1C)>>2); //Read current speed and duplex mode from LAN8720 register 31
	return speed;
}

//Get Ethernet connection status
//Return value:
//0: Not connected
//1: Connected
u8 LAN8720_Get_Link_Status(void)
{
	u16 regval;
	regval = ETH_ReadPHYRegister(LAN8720_PHY_ADDRESS, 1); //Read LAN8720 PHY Basic Status Register (Register 1)
	if(regval & (1<<2)) //Check bit2 (Link Status bit)
	{
		return 1; //Link established
	}
	return 0; //Not connected
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//The following section contains STM32F407 driver/interface functions.

//Initialize ETH MAC layer and DMA configuration
//Return value: ETH_ERROR, configuration failed (0)
//              ETH_SUCCESS, configuration successful (1)
u8 ETH_MACDMA_Config(void)
{
	u8 rval;
	ETH_InitTypeDef ETH_InitStructure; 
	
	//Enable Ethernet MAC and MAC transmit/receive clocks
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx |RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);
                        
	ETH_DeInit();  								//Reset Ethernet on AHB bus
	ETH_SoftwareReset();  						//Software reset
	while (ETH_GetSoftwareResetStatus() == SET);//Wait for software reset to complete 
	ETH_StructInit(&ETH_InitStructure); 	 	//Initialize structure to default values  

	///Configure MAC parameters 
	ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Disable;  			//Disable auto-negotiation, manual configuration
	ETH_InitStructure.ETH_Speed = ETH_Speed_100M;                          			//Speed set to 100M
	ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;                     			//Full-duplex mode
	ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;					//Disable loopback
	ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable; 		//Disable retry transmission
	ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable; 	//Disable automatic PAD/CRC stripping 
	ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;						//Disable receiving all frames
	ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;//Do not filter broadcast frames
	ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;			//Disable promiscuous mode address filtering  
	ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;//Use perfect address filtering for multicast addresses   
	ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;	//Use perfect address filtering for unicast addresses 
#ifdef CHECKSUM_BY_HARDWARE
	ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable; 			//Enable IPv4, TCP/UDP/ICMP frame checksum offload   
#endif
	//When using frame checksum offload, store-and-forward mode must be enabled. Store-and-forward mode ensures
	//the entire frame is stored in FIFO so MAC can calculate/verify frame checksum. DMA can only process frames
	//when checksum is correct, otherwise discard error frames
	ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; //Drop frames with TCP/IP checksum errors
	ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;     //Enable store-and-forward mode for received data    
	ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;   //Enable store-and-forward mode for transmitted data  

	ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;     	//Disable forwarding error frames  
	ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;	//Do not forward undersized good frames 
	ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;  		//Enable processing second frame
	ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;  	//Enable DMA transfer address alignment
	ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;            			//Enable fixed burst function    
	ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;     		//DMA transmit maximum burst length is 32 beats   
	ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;			//DMA receive maximum burst length is 32 beats
	ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;
    
	rval=ETH_Init(&ETH_InitStructure,LAN8720_PHY_ADDRESS);		//Configure ETH
	if(rval==ETH_SUCCESS)//Configuration successful
	{
		ETH_DMAITConfig(ETH_DMA_IT_NIS|ETH_DMA_IT_R,ENABLE);  	//Enable Ethernet receive interrupt	
	}
	return rval;
}

extern void lwip_pkt_handle(void);		//Defined in lwip_comm.c
//Ethernet DMA receive interrupt handler
void ETH_IRQHandler(void)
{
	while(ETH_GetRxPktSize(DMARxDescToGet)!=0) 	//Check if data packet received
	{ 
		lwip_pkt_handle();		
	}
	ETH_DMAClearITPendingBit(ETH_DMA_IT_R); 	//Clear DMA interrupt flag
	ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);	//Clear DMA normal interrupt flag
}  
//Receive a data packet
//Return value: received data packet frame structure
FrameTypeDef ETH_Rx_Packet(void)
{ 
	u32 framelength=0;
	FrameTypeDef frame={0,0};   
	//Check if current descriptor is owned by ETHERNET DMA (when set) / CPU (when reset)
	if((DMARxDescToGet->Status&ETH_DMARxDesc_OWN)!=(u32)RESET)
	{	
		frame.length=ETH_ERROR; 
		if ((ETH->DMASR&ETH_DMASR_RBUS)!=(u32)RESET)  
		{ 
			ETH->DMASR = ETH_DMASR_RBUS;//Clear ETH DMA RBUS bit 
			ETH->DMARPDR=0;//Resume DMA reception
		}
		return frame;//Error, OWN bit is set
	}  
	if(((DMARxDescToGet->Status&ETH_DMARxDesc_ES)==(u32)RESET)&& 
	((DMARxDescToGet->Status & ETH_DMARxDesc_LS)!=(u32)RESET)&&  
	((DMARxDescToGet->Status & ETH_DMARxDesc_FS)!=(u32)RESET))  
	{       
		framelength=((DMARxDescToGet->Status&ETH_DMARxDesc_FL)>>ETH_DMARxDesc_FrameLengthShift)-4;//Get received frame length (excluding 4-byte CRC)
 		frame.buffer = DMARxDescToGet->Buffer1Addr;//Get location where frame is stored
	}else framelength=ETH_ERROR;//Error  
	frame.length=framelength; 
	frame.descriptor=DMARxDescToGet;  
	//Update ETH DMA global Rx descriptor to next Rx descriptor
	//Fetch next DMA Rx descriptor for next buffer
	DMARxDescToGet=(ETH_DMADESCTypeDef*)(DMARxDescToGet->Buffer2NextDescAddr);   
	return frame;  
}
//Transmit a data packet
//FrameLength: data packet length
//Return value: ETH_ERROR, transmission failed (0)
//              ETH_SUCCESS, transmission successful (1)
u8 ETH_Tx_Packet(u16 FrameLength)
{   
	//Check if current descriptor is owned by ETHERNET DMA (when set) / CPU (when reset)
	if((DMATxDescToSet->Status&ETH_DMATxDesc_OWN)!=(u32)RESET)return ETH_ERROR;//Error, OWN bit is set 
 	DMATxDescToSet->ControlBufferSize=(FrameLength&ETH_DMATxDesc_TBS1);//Set frame length, bits[12:0]
	DMATxDescToSet->Status|=ETH_DMATxDesc_LS|ETH_DMATxDesc_FS;//Set last segment and first segment bits (1 frame in one segment)
  	DMATxDescToSet->Status|=ETH_DMATxDesc_OWN;//Set Tx descriptor OWN bit, return buffer to ETH DMA
	if((ETH->DMASR&ETH_DMASR_TBUS)!=(u32)RESET)//When Tx Buffer unavailable bit (TBUS) is set, clear it and resume transmission
	{ 
		ETH->DMASR=ETH_DMASR_TBUS;//Clear ETH DMA TBUS bit 
		ETH->DMATPDR=0;//Resume DMA transmission
	} 
	//Update ETH DMA global Tx descriptor to next Tx descriptor
	//Fetch next DMA Tx descriptor for next buffer transmission 
	DMATxDescToSet=(ETH_DMADESCTypeDef*)(DMATxDescToSet->Buffer2NextDescAddr);    
	return ETH_SUCCESS;   
}
//Get current transmit Tx buffer address
//Return value: Tx buffer address
u32 ETH_GetCurrentTxBuffer(void)
{  
  return DMATxDescToSet->Buffer1Addr;//Return Tx buffer address  
}

//Allocate memory for ETH underlying send/receive
//Return value: 0, success
//              other, failure
u8 ETH_Mem_Malloc(void)
{ 
	DMARxDscrTab=mymalloc(SRAMIN,ETH_RXBUFNB*sizeof(ETH_DMADESCTypeDef));//Allocate memory
	DMATxDscrTab=mymalloc(SRAMIN,ETH_TXBUFNB*sizeof(ETH_DMADESCTypeDef));//Allocate memory  
	Rx_Buff=mymalloc(SRAMIN,ETH_RX_BUF_SIZE*ETH_RXBUFNB);	//Allocate memory
	Tx_Buff=mymalloc(SRAMIN,ETH_TX_BUF_SIZE*ETH_TXBUFNB);	//Allocate memory
	if(!DMARxDscrTab||!DMATxDscrTab||!Rx_Buff||!Tx_Buff)
	{
		ETH_Mem_Free();
		return 1;	//Allocation failed
	}	
	return 0;		//Allocation successful
}

//Free ETH underlying send/receive memory
void ETH_Mem_Free(void)
{ 
	myfree(SRAMIN,DMARxDscrTab);//Free memory
	myfree(SRAMIN,DMATxDscrTab);//Free memory
	myfree(SRAMIN,Rx_Buff);		//Free memory
	myfree(SRAMIN,Tx_Buff);		//Free memory  
}























