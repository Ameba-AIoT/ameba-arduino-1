#ifndef _MMC_SDIO_COMMON_H
#define _MMC_SDIO_COMMON_H

/* os header include */
#ifdef PLATFORM_LINUX
#include "platform/os_linux.h"
#elif defined PLATFORM_THREADX
#include "platform/os_threadx.h"
#elif defined PLATFORM_NONE
#include "platform/os_none.h"
#elif defined PLATFORM_TOPPERS
#include "platform/os_toppers.h"
#else
#include "platform/os_freertos_conf.h"
#include "platform/os_freertos.h"
#endif

#ifndef BIT
#define BIT(x)		((unsigned int)0x00000001 << (x))
#endif

/* from error_base.h */
#define	ENOENT		 2	/* No such file or directory */
#define	EIO		 5	/* I/O error */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EINVAL		22	/* Invalid argument */
#define	ERANGE		34	/* Math result not representable */

/* from arm errno.h */
#define	ENOSYS		38	/* Function not implemented */
#define	ETIME		62	/* Timer expired */
#define	EILSEQ		84	/* Illegal byte sequence */
#define	ETIMEDOUT	110	/* Connection timed out */
#define	ENOMEDIUM	123	/* No medium found */

/*Note ffs(0) = 0, ffs(9) = 0, ffs(0x80000000) = 32. */
static inline u32 rtw_ffs(u32 x)
{
	int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

/*
 * fls - find last bit set.
 * @word: The word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(9) = 3, fls(0x80000000) = 32.
 */
static inline int rtw_fls(int x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

#include "card.h"
#include "../core/core.h"
#include "../core/host.h"
#include "../core/sdio.h"
#ifdef CONFIG_SDIO_SDHCI
#include "../host/sdhci.h"
#endif
#ifdef CONFIG_TMIO_HCI
#include "../host/tmio_mmc.h"
#endif
#ifdef CONFIG_STM32_HCI
#include "../host/stm32f2xx_hci.h"
#endif
#include "../core/sdio_ops.h"
#include "../core/sdio_cis.h"
#include "../core/sdio_io.h"
#include "../core/sdio_irq.h"
#include "../wifi_io.h"
#endif
