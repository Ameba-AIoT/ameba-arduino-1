#define CONFIG_SSL_RSA          1

#include "rom_ssl_ram_map.h"
#define RTL_HW_CRYPTO
//#define SUPPORT_HW_SW_CRYPTO

//zzw 
#if 0
/* RTL_CRYPTO_FRAGMENT should be less than 16000, and should be 16bytes-aligned */
#if defined (CONFIG_PLATFORM_8195A)
#define RTL_CRYPTO_FRAGMENT                4096
#else
#define RTL_CRYPTO_FRAGMENT               15360
#endif
#else
#define RTL_CRYPTO_FRAGMENT               15360 /* 15*1024 < 16000 */
#endif


#if ARDUINO_SDK
#include "section_config.h"
#include "platform_stdlib.h"
#include "mbedtls/config_arduino.h"
#elif CONFIG_SSL_RSA
#include "platform_stdlib.h"
#include "mbedtls/config_rsa.h"
#else
#include "platform_stdlib.h"
#include "mbedtls/config_all.h"
#endif