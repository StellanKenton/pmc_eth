#include "netif/ethernetif.h" 
#include "lan8720.h"  
#include "lwip_comm.h" 
#include "netif/etharp.h"  
#include "string.h"  
 
 
// Called by ethernetif_init() to initialize hardware
// netif: pointer to network interface structure
// Return value: ERR_OK, success
//               other, failure
static err_t low_level_init(struct netif *netif)
{
#ifdef CHECKSUM_BY_HARDWARE
	int i; 
#endif 
	netif->hwaddr_len = ETHARP_HWADDR_LEN; // Set MAC address length to 6 bytes
	netif->mtu=1500; // Maximum transfer unit, supports broadcast and ARP functions
	// And the device can broadcast and perform hardware address resolution
	netif->flags = NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP|NETIF_FLAG_LINK_UP;
	
	ETH_MACAddressConfig(ETH_MAC_Address0, netif->hwaddr); // Write MAC address to STM32F4 MAC address register
	ETH_DMATxDescChainInit(DMATxDscrTab, Tx_Buff, ETH_TXBUFNB);
	ETH_DMARxDescChainInit(DMARxDscrTab, Rx_Buff, ETH_RXBUFNB);
#ifdef CHECKSUM_BY_HARDWARE 	// Use hardware frame checksum
	for(i=0;i<ETH_TXBUFNB;i++)	// Enable TCP, UDP and ICMP transmit frame checksum, TCP, UDP and ICMP receive frame checksum is done by DMA automatically
	{
		ETH_DMATxDescChecksumInsertionConfig(&DMATxDscrTab[i], ETH_DMATxDesc_ChecksumTCPUDPICMPFull);
	}
#endif
	ETH_Start(); // Start MAC and DMA				
	return ERR_OK;
} 
// Low-level function for sending data packets (lwip calls this function through netif->linkoutput pointer)
// netif: pointer to network interface structure
// p: pointer to pbuf data structure
// Return value: ERR_OK, send success
//               ERR_MEM, send failure
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	u8 res;
	struct pbuf *q;
	int l = 0;
	u8 *buffer=(u8 *)ETH_GetCurrentTxBuffer(); 
	for(q=p;q!=NULL;q=q->next) 
	{
		memcpy((u8_t*)&buffer[l], q->payload, q->len);
		l=l+q->len;
	} 
	res=ETH_Tx_Packet(l); 
	if(res==ETH_ERROR)return ERR_MEM;// Return error status
	return ERR_OK;
} 
// Low-level function for receiving data packets
// netif: pointer to network interface structure
// Return value: pointer to pbuf data structure
static struct pbuf * low_level_input(struct netif *netif)
{  
	struct pbuf *p, *q;
	u16_t len;
	int l =0;
	FrameTypeDef frame;
	u8 *buffer;
	p = NULL;
	frame=ETH_Rx_Packet();
	len=frame.length;// Get frame size
	buffer=(u8 *)frame.buffer;// Get received data address 
	p=pbuf_alloc(PBUF_RAW,len,PBUF_POOL);// Allocate pbuf memory pool
	if(p!=NULL)
	{
		for(q=p;q!=NULL;q=q->next)
		{
			memcpy((u8_t*)q->payload,(u8_t*)&buffer[l], q->len);
			l=l+q->len;
		}    
	}
	frame.descriptor->Status=ETH_DMARxDesc_OWN;// Set Rx descriptor OWN bit, buffer returns to ETH DMA 
	if((ETH->DMASR&ETH_DMASR_RBUS)!=(u32)RESET)// When Rx Buffer unavailable bit (RBUS) is set, clear it and resume reception
	{ 
		ETH->DMASR=ETH_DMASR_RBUS;// Clear ETH DMA RBUS bit 
		ETH->DMARPDR=0;// Resume DMA reception
	}
	return p;
} 
// Network interface input function (called directly by lwip)
// netif: pointer to network interface structure
// Return value: ERR_OK, receive success
//               ERR_MEM, receive failure
err_t ethernetif_input(struct netif *netif)
{
	err_t err;
	struct pbuf *p;
	p=low_level_input(netif);
	if(p==NULL) return ERR_MEM;
	err=netif->input(p, netif);
	if(err!=ERR_OK)
	{
		LWIP_DEBUGF(NETIF_DEBUG,("ethernetif_input: IP input error\n"));
		pbuf_free(p);
		p = NULL;
	} 
	return err;
} 
// Use low_level_init() function to initialize network interface
// netif: pointer to network interface structure
// Return value: ERR_OK, success
//               other, failure
err_t ethernetif_init(struct netif *netif)
{
	LWIP_ASSERT("netif!=NULL",(netif!=NULL));
#if LWIP_NETIF_HOSTNAME			// LWIP_NETIF_HOSTNAME 
	netif->hostname="lwip";  	// Initialize hostname
#endif 
	netif->name[0]=IFNAME0; 	// Initialize the name field of netif structure
	netif->name[1]=IFNAME1; 	// Defined in this file, no need to care about specific values
	netif->output=etharp_output;// IP layer sends data packets function
	netif->linkoutput=low_level_output;// ARP module sends data packets function
	low_level_init(netif); 		// Low-level hardware initialization function
	return ERR_OK;
}














