#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#undef  DEFAULT_THREAD_PRIO
#define DEFAULT_THREAD_PRIO		2

#define SYS_LIGHTWEIGHT_PROT    1  //When set to 1, use real-time operating system to protect critical sections from interrupt interference

//NO_SYS==1: Do not use operating system
#define NO_SYS                  0  //Use FreeRTOS operating system

//Use 4-byte alignment mode
#define MEM_ALIGNMENT           4  

//MEM_SIZE: heap memory size, if application has large data transmission, this value should be larger 
#define MEM_SIZE                16000 //Memory heap size

//MEMP_NUM_PBUF: number of memp structure pbufs, if application reads data from ROM or static storage, this value should be larger
#define MEMP_NUM_PBUF           20

//MEMP_NUM_UDP_PCB: number of UDP protocol control blocks (PCB). Each active UDP "connection" requires one PCB.
#define MEMP_NUM_UDP_PCB        6

//MEMP_NUM_TCP_PCB: number of simultaneously active TCP connections
#define MEMP_NUM_TCP_PCB        10

//MEMP_NUM_TCP_PCB_LISTEN: number of listening TCP connections
#define MEMP_NUM_TCP_PCB_LISTEN 6

//MEMP_NUM_TCP_SEG: number of TCP segments simultaneously in queue
#define MEMP_NUM_TCP_SEG        15

//MEMP_NUM_SYS_TIMEOUT: number of simultaneously active timeout functions
#define MEMP_NUM_SYS_TIMEOUT    8


/* ---------- Pbuf options ---------- */
//PBUF_POOL_SIZE: number of pbuf memory pools. 
#define PBUF_POOL_SIZE          20

//PBUF_POOL_BUFSIZE: size of each pbuf memory pool. 
#define PBUF_POOL_BUFSIZE       512


/* ---------- TCP options ---------- */
#define LWIP_TCP                1  //Set to 1 to enable TCP
#define TCP_TTL                 255//Time to live

/*TCP receive data segment control, should be 0 when device memory is small*/
#undef TCP_QUEUE_OOSEQ
#define TCP_QUEUE_OOSEQ         0

//Maximum TCP segment size
#define TCP_MSS                 (1500 - 40)	  //Maximum TCP segment, TCP_MSS = (MTU - IP header size - TCP header size)

//TCP send buffer size (bytes).
#define TCP_SND_BUF             (4*TCP_MSS)

//TCP_SND_QUEUELEN: TCP send buffer size (pbuf). Minimum value is (2 * TCP_SND_BUF/TCP_MSS) 
#define TCP_SND_QUEUELEN        (2* TCP_SND_BUF/TCP_MSS)

//TCP receive window
#define TCP_WND                 (2*TCP_MSS)


/* ---------- ICMP options ---------- */
#define LWIP_ICMP                 1 //Enable ICMP protocol

/* ---------- DHCP options ---------- */
//Should be set to 1 when using DHCP, DHCP function not available in LwIP 0.5.1 version.
#define LWIP_DHCP               1

/* ---------- UDP options ---------- */ 
#define LWIP_UDP                1 //Enable UDP
#define UDP_TTL                 255 //UDP packet time to live


/* ---------- Statistics options ---------- */
#define LWIP_STATS 0
#define LWIP_PROVIDE_ERRNO 1


//Frame checksum options, STM32F4x7 can recognize and calculate IP, UDP and ICMP frame checksums via hardware
#define CHECKSUM_BY_HARDWARE //Define CHECKSUM_BY_HARDWARE to use hardware frame checksum


#ifdef CHECKSUM_BY_HARDWARE
  //CHECKSUM_GEN_IP==0: Hardware generates IP packet frame checksum
  #define CHECKSUM_GEN_IP                 0
  //CHECKSUM_GEN_UDP==0: Hardware generates UDP packet frame checksum
  #define CHECKSUM_GEN_UDP                0
  //CHECKSUM_GEN_TCP==0: Hardware generates TCP packet frame checksum
  #define CHECKSUM_GEN_TCP                0 
  //CHECKSUM_CHECK_IP==0: Hardware verifies received IP packet frame checksum
  #define CHECKSUM_CHECK_IP               0
  //CHECKSUM_CHECK_UDP==0: Hardware verifies received UDP packet frame checksum
  #define CHECKSUM_CHECK_UDP              0
  //CHECKSUM_CHECK_TCP==0: Hardware verifies received TCP packet frame checksum
  #define CHECKSUM_CHECK_TCP              0
#else
  //CHECKSUM_GEN_IP==1: Software generates IP packet frame checksum
  #define CHECKSUM_GEN_IP                 1
  // CHECKSUM_GEN_UDP==1: Software generates UDP packet frame checksum
  #define CHECKSUM_GEN_UDP                1
  //CHECKSUM_GEN_TCP==1: Software generates TCP packet frame checksum
  #define CHECKSUM_GEN_TCP                1
  // CHECKSUM_CHECK_IP==1: Software verifies received IP packet frame checksum
  #define CHECKSUM_CHECK_IP               1
  // CHECKSUM_CHECK_UDP==1: Software verifies received UDP packet frame checksum
  #define CHECKSUM_CHECK_UDP              1
  //CHECKSUM_CHECK_TCP==1: Software verifies received TCP packet frame checksum
  #define CHECKSUM_CHECK_TCP              1
#endif


/*
   ----------------------------------------------
   ---------- Sequential API options ----------
   ----------------------------------------------
*/

//LWIP_NETCONN==1: Enable NETCON API (requires api_lib.c)
#define LWIP_NETCONN                    1

/*
   ------------------------------------
   ---------- Socket API options ----------
   ------------------------------------
*/
//LWIP_SOCKET==1: Enable Socket API (requires sockets.c)
#define LWIP_SOCKET                     1

#define LWIP_COMPAT_MUTEX               1

#define LWIP_SO_RCVTIMEO                1 //Enable recv_timeout in netconn structure via LWIP_SO_RCVTIMEO, recv_timeout can block threads

//Operating system related options
#define TCPIP_MBOX_SIZE                 20
#define DEFAULT_UDP_RECVMBOX_SIZE       2000
#define DEFAULT_TCP_RECVMBOX_SIZE       2000
#define DEFAULT_ACCEPTMBOX_SIZE         2000
#define DEFAULT_THREAD_STACKSIZE        512
//Kernel thread stack size
#define TCPIP_THREAD_STACKSIZE         1000      //Kernel thread stack size
#define TCPIP_THREAD_PRIO               (configMAX_PRIORITIES - 2)				//Kernel thread priority
/*
   ----------------------------------------
   ---------- Lwip debug options ----------
   ----------------------------------------
*/
#define LWIP_DEBUG                      0 //Disable DEBUG option

#define ICMP_DEBUG                      LWIP_DBG_OFF //Enable/disable ICMP debug

#endif /* __LWIPOPTS_H__ */
