// cc.h is part of the LWIP TCP/IP protocol stack
// Author: Adam Dunkels <adam@sics.se>

#ifndef __CC_H__
#define __CC_H__

#include "cpu.h"
#include "stdio.h"
#include "FreeRTOS.h"  //Include this header file when using UCOS

//Platform-independent data types
typedef unsigned   char    u8_t;  	//Unsigned 8-bit integer  
typedef signed     char    s8_t;   	//Signed 8-bit integer 
typedef unsigned   short   u16_t;  	//Unsigned 16-bit integer
typedef signed     short   s16_t;   //Signed 16-bit integer
typedef unsigned   long    u32_t;   //Unsigned 32-bit integer
typedef signed     long    s32_t;   //Signed 32-bit integer
typedef u32_t mem_ptr_t;            //Memory address type
typedef int sys_prot_t;				//Critical section protection type

//Critical section protection when using operating system, using UCOS II as example
//When OS_CRITICAL_METHOD is defined, it means UCOS II is being used
#if OS_CRITICAL_METHOD == 1
#define SYS_ARCH_DECL_PROTECT(lev)
#define SYS_ARCH_PROTECT(lev)		CPU_INT_DIS()
#define SYS_ARCH_UNPROTECT(lev)		CPU_INT_EN()
#endif

#if OS_CRITICAL_METHOD == 3  
#define SYS_ARCH_DECL_PROTECT(lev)	u32_t lev
#define SYS_ARCH_PROTECT(lev)		lev = OS_CPU_SR_Save() 	//UCOS II enter critical section, disable interrupts
#define SYS_ARCH_UNPROTECT(lev)		OS_CPU_SR_Restore(lev)	//UCOS II exit critical section, enable interrupts 
#endif

//Define some macros based on different compilers
#if defined (__ICCARM__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_USE_INCLUDES

#elif defined (__CC_ARM)

#define PACK_STRUCT_BEGIN __packed
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__GNUC__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__TASKING__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#endif

//Some macros used when LWIP calls printf
#define U16_F "4d"
#define S16_F "4d"
#define X16_F "4x"
#define U32_F "8ld"
#define S32_F "8ld"
#define X32_F "8lx"

//Macro definitions
#ifndef LWIP_PLATFORM_ASSERT
#define LWIP_PLATFORM_ASSERT(x) \
    do \
    {   printf("Assertion \"%s\" failed at line %d in %s\r\n", x, __LINE__, __FILE__); \
    } while(0)
#endif

#ifndef LWIP_PLATFORM_DIAG
#define LWIP_PLATFORM_DIAG(x) do {printf x;} while(0)
#endif

#endif /* __CC_H__ */
