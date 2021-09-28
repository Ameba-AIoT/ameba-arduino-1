#ifndef __PLATFORM_TOPPERS_H__
#define __PLATFORM_TOPPERS_H__

#ifndef inline
#define inline __inline
#endif

#include "stm32f2xx.h"
#include "stm322xg_eval.h"
#include "os_nolinux.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <rtwlan_bsp.h>

/* Define compilor specific symbol */
//
// inline function
//

#if defined ( __ICCARM__ )
#define __inline__                      inline
#define __inline                        inline
#define __inline_definition			//In dialect C99, inline means that a function's definition is provided 
								//only for inlining, and that there is another definition 
								//(without inline) somewhere else in the program. 
								//That means that this program is incomplete, because if 
								//add isn't inlined (for example, when compiling without optimization), 
								//then main will have an unresolved reference to that other definition.

								// Do not inline function is the function body is defined .c file and this 
								// function will be called somewhere else, otherwise there is compile error
#elif defined ( __CC_ARM   )
#define __inline__			__inline	//__linine__ is not supported in keil compilor, use __inline instead
#define inline				__inline
#define __inline_definition			// for dialect C99
#elif defined   (  __GNUC__  )
#define __inline__                      inline
#define __inline                        inline
#define __inline_definition	inline
#endif

#include <freertos/freertos_service.h>
#include <freertos/wrapper.h>

#define syslog	printf("\n\r"); printf
#define RTKDEBUG "rtw_sdio: "
#define dbg_host(...)     do {\
	syslog(RTKDEBUG __VA_ARGS__);\
}while(0)

#ifdef RTW_LITTLE_ENDIAN
#define le16_to_cpup(x)	(*(u16*)(x))
#define le32_to_cpup(x)	(*(u32*)(x))
#define cpu_to_le16(x)	((u16)(x))
#define cpu_to_le32(x) 	((u32)(x))
#else //PLATFORM_TODO
#define le16_to_cpup(x)	(*(u16*)(x))
#define le32_to_cpup(x)	(*(u32*)(x))
#define cpu_to_le16(x)	((u16)(x))
#define cpu_to_le32(x) 	((u32)(x))
#endif

#define rtw_yield		taskYIELD

typedef xSemaphoreHandle	rtw_sema;
typedef xSemaphoreHandle	rtw_mutex;
typedef xSemaphoreHandle	rtw_spinlock_t;
typedef struct timer_list	_timer;

struct timer_list {
	_timerHandle 	timer_hdl;
	unsigned long	data;
	void (*function)(void *);
};


static inline int rtw_request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *data)
{
	return 0;
}

static inline void rtw_free_irq(unsigned int irq, void *data)
{

}

static inline void rtw_msleep(int ms)
{
	vTaskDelay(ms / portTICK_RATE_MS);
}

static inline void rtw_mdelay(int ms)
{
	vTaskDelay(ms / portTICK_RATE_MS);
}

static inline void rtw_udelay(int us)
{	
	WLAN_BSP_UsLoop(us);
}

static inline void rtw_sema_init(rtw_sema *sema, int init_val)
{
	*sema = xSemaphoreCreateCounting(0xffffffff, init_val);	//Set max count 0xffffffff;
}

static inline void rtw_sema_free(rtw_sema *sema)
{
	if(*sema != NULL)
		vSemaphoreDelete(*sema);

	sema = NULL;
}

static inline void rtw_sig_sem(rtw_sema *sema)
{
	xSemaphoreGive(*sema);
}

#define WAIT_SEM_INFINITE		0x7FFFFFFF
static inline void rtw_wait_sem(rtw_sema *sema, u32 to_ms)
{
	if (to_ms == WAIT_SEM_INFINITE) {
		while(xSemaphoreTake(*sema, portMAX_DELAY) != pdTRUE)
			dbg_host("[%s] rtw_wait_sem(%p) failed, retry\n", pcTaskGetTaskName(NULL), sema);
	} else {
		while(xSemaphoreTake(*sema, to_ms / portTICK_RATE_MS) != pdTRUE)
			;
	}
}

static inline void rtw_mutex_init(rtw_mutex *pmutex)
{
	*pmutex = xSemaphoreCreateMutex();
}

static inline void rtw_mutex_lock(rtw_mutex *pmutex)
{
	while(xSemaphoreTake(*pmutex, 60 * 1000 / portTICK_RATE_MS) != pdTRUE)
		dbg_host("[%s] rtw_mutex_lock(%p) failed, retry\n", pcTaskGetTaskName(NULL), pmutex);
}

static inline void rtw_mutex_unlock(rtw_mutex *pmutex)
{
	xSemaphoreGive(*pmutex);
}

static inline void rtw_mutex_free(rtw_mutex *pmutex)
{
	if(*pmutex != NULL)
		vSemaphoreDelete(*pmutex);

	*pmutex = NULL;
}

static inline void rtw_spin_lock_init(rtw_spinlock_t *plock)
{
	*plock = xSemaphoreCreateMutex();
}

static inline  void rtw_spin_lock_free(rtw_spinlock_t *plock)
{
	if(*plock != NULL)
		vSemaphoreDelete(*plock);

	*plock = NULL;
}

static inline  void rtw_spin_lock(rtw_spinlock_t *plock)
{
	while(xSemaphoreTake(*plock, 60 * 1000 / portTICK_RATE_MS) != pdTRUE)
		dbg_host("[%s] rtw_spin_lock(%p) failed, retry\n", pcTaskGetTaskName(NULL), plock);
}

static inline void rtw_spin_unlock(rtw_spinlock_t *plock)
{
	xSemaphoreGive(*plock);
}

static inline int dma_map_sg_attrs(struct device *dev, struct scatterlist *sg,
				   int nents, enum dma_data_direction dir,
				   struct dma_attrs *attrs)
{
	if (dir == DMA_FROM_DEVICE)
		SD_LowLevel_DMA_RxConfig((u32 *)sg->dma_address, sg->length);
	else
		SD_LowLevel_DMA_TxConfig((u32 *)sg->dma_address, sg->length);

	return 1;
}

static inline void dma_unmap_sg_attrs(struct device *dev, struct scatterlist *sg,
				      int nents, enum dma_data_direction dir,
				      struct dma_attrs *attrs)
{
	//PLATFORM_TODO
}

#define dma_map_sg(d, s, n, r) dma_map_sg_attrs(d, s, n, r, NULL)
#define dma_unmap_sg(d, s, n, r) dma_unmap_sg_attrs(d, s, n, r, NULL)
/* DMA Ralated End */

#endif
