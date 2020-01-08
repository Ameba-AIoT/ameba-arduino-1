#define CONFIG_PROJECT_CUSTOM   0
#define CONFIG_SSL_RSA          1

#if (defined CONFIG_PLATFORM_8195A) || (defined CONFIG_PLATFORM_8711B)
#include "section_config.h"
#include "rom_ssl_ram_map.h"
#define RTL_HW_CRYPTO
#define SUPPORT_HW_SW_CRYPTO
#endif

#if defined (CONFIG_SSL_ROM) //define in ROM makefile
#include "polarssl/ssl_rom_lib.h"
#include "polarssl/config_rom.h"
#elif CONFIG_PROJECT_CUSTOM
#include "platform_stdlib.h"
#include "ssl_config.h"
#elif ARDUINO_SDK
#include "platform_stdlib.h"
#include "polarssl/config_arduino.h"
#elif CONFIG_SSL_RSA
#include "platform_stdlib.h"
#include "polarssl/config_rsa.h"
#else
#include "platform_stdlib.h"
#include "polarssl/config_all.h"
#endif /* CONFIG_SSL_ROM */
