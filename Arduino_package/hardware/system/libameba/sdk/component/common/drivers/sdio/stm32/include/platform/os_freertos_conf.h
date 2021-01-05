#ifndef __SDIO_FREERTOS_H__
#define __SDIO_FREERTOS_H__

#include "stm32_platform.h"

#ifdef WIFI_USE_SDIO
#define BUILD_SDIO
#endif

//#define PLATFORM_FREERTOS
#define RTW_LITTLE_ENDIAN
#define RTW_MEM_STATIC	
#define CONFIG_STM32_HCI
#define SDIO_HOST_INT
//#define SDIO_FORCE_BYTE_MODE

#endif
