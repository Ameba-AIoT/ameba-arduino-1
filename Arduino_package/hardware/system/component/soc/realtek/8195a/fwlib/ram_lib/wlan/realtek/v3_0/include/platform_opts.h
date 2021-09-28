/**
 ******************************************************************************
 *This file contains general configurations for ameba platform
 ******************************************************************************
*/

#ifndef __PLATFORM_OPTS_H__
#define __PLATFORM_OPTS_H__


/**
 * For AT cmd Log service configurations
 */
#define SUPPORT_LOG_SERVICE	1
#if SUPPORT_LOG_SERVICE
#define LOG_SERVICE_BUFLEN     64 //can't larger than UART_LOG_CMD_BUFLEN(127)
#define CONFIG_LOG_HISTORY	0
#if CONFIG_LOG_HISTORY
#define LOG_HISTORY_LEN    5
#endif
#define SUPPORT_INTERACTIVE_MODE		1//on/off wifi_interactive_mode
#endif

/**
 * For interactive mode configurations, depents on log service
 */
#if SUPPORT_INTERACTIVE_MODE
#define CONFIG_INTERACTIVE_MODE     1
#define CONFIG_INTERACTIVE_EXT   0
#else
#define CONFIG_INTERACTIVE_MODE     0
#define CONFIG_INTERACTIVE_EXT   0
#endif
/******************************************************************************/


/**
 * For Wlan configurations
 */
#define CONFIG_WLAN	1
#if CONFIG_WLAN
#define CONFIG_LWIP_LAYER	1
#define CONFIG_INIT_NET         0 //init lwip layer when start up

//on/off relative commands in log service
#define CONFIG_SSL_CLIENT       0
#define CONFIG_WEBSERVER        0
#define CONFIG_OTA_UPDATE       0
#define CONFIG_BSD_TCP          0
#define CONFIG_ENABLE_P2P       0//on/off p2p cmd in log_service or interactive mode

/* For WPS and P2P */
#define CONFIG_WPS
#if defined(CONFIG_WPS) 
#define CONFIG_ENABLE_WPS       1
#endif

/* For Simple Link */
#define CONFIG_INCLUDE_SIMPLE_CONFIG	0

#endif //end of #if CONFIG_WLAN
/*******************************************************************************/

/**
 * For iNIC configurations
 */
#define CONFIG_INIC_EN 0//enable iNIC mode
#if CONFIG_INIC_EN
#define CONFIG_LWIP_LAYER	0
#define CONFIG_INIC_SDIO_HCI	1 //for SDIO or USB iNIC
#define CONFIG_INIC_USB_HCI	0
#endif

#endif
