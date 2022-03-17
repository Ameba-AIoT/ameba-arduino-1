#ifndef __PLATFORM_THREADX_H__
#define __PLATFORM_THREADX_H__

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned int bool;

#include "os_nolinux.h"

static inline void printk(const char *fmt, ...)
{
	//PLATFORM_TODO
}
#define RTKDEBUG "rtw_sdio: "
#define dbg_host(...)     do {\
	printk(RTKDEBUG __VA_ARGS__);\
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

typedef struct semaphore {
	SCI_SEMAPHORE_PTR sci_sema;
	char sema_name[64];
}rtw_sema;
typedef struct mutex {
	SCI_MUTEX_PTR	sci_mutex;
	char mutex_name[64];
}rtw_mutex;
typedef struct mutex rtw_spinlock_t;

struct timer_list {
	SCI_TIMER_PTR sci_timer;
	char timer_name[64];
	unsigned long expires;
	void (*function)(unsigned long);

	void *context;
};

#define rtw_spin_lock(lock)				//PLATFORM_TODO
#define rtw_spin_unlock(lock)				//PLATFORM_TODO
#define rtw_spin_lock_init(lock)			//PLATFORM_TODO
#define rtw_spin_lock_free(lock)			//PLATFORM_TODO

static inline void rtw_msleep(int ms)
{
	 SCI_Sleep(ms);
}

static inline void rtw_mdelay(int ms)
{
	if (SCI_InThreadContext()) 
	{
		SCI_Sleep(ms);
	} 
	else 
	{
		OS_TickDelay(ms);
	}
}

static inline void rtw_udelay(int us)
{
	unsigned int l,m,k;

	k = CHIP_GetArmClk()/ARM_CLK_13M;
	for (l = 0; l < us; l++) 
	{
		m = k; 
		while(m--){ };
	}
}

static inline void *rtw_kmalloc(unsigned int size, gfp_t flags)
{
	void* ptr = 0;

	ptr = SCI_ALLOCA(size);

	return ptr;

}

static inline void rtw_kfree(void *p)
{
	if(p == NULL)
		return;
	SCI_FREE(p);
}

static inline void *rtw_kzalloc(unsigned int size, gfp_t flags)
{
	void* ptr = 0;

	ptr = SCI_ALLOCA(size);
	if (ptr != 0)
	{
		SCI_MEMSET(ptr,0,(size_t)size);
	}

	return ptr;
}

unsigned int semidx = 0;
static inline void rtw_sema_init(rtw_sema *sem, int val)
{
	strcat(sem->sema_name, "RTW_WIFI_SEM_"#semidx);
	semidx ++;

	sem->sci_sema = SCI_CreateSemaphore(sem->sema_name, val);
}

static inline void rtw_sema_free(rtw_sema *sem)
{
	if (sem == NULL)
		return;
        if (!sem->sci_sema)
                return;
	SCI_DeleteSemaphore(sem->sci_sema);
	semidx --;
}

static inline void rtw_sig_sem(rtw_sema *sem)
{
	if (sem == NULL)
		return;

	return SCI_PutSemaphore(sem);
}

#define WAIT_SEM_INFINITE		0x7FFFFFFF
static inline void rtw_wait_sem(rtw_sema *sem, u32 to_ms)
{
	if (sem == NULL)
		return;
	return SCI_GetSemaphore (sem, SCI_WAIT_FOREVER);
}

unsigned int mutexidx = 0;
static inline void rtw_mutex_init(rtw_mutex *mutex)
{
	strcat(mutex->sema_name, "RTW_WIFI_MUTEX_"#mutexidx);
	mutexidx ++;

	mutex->sci_mutex = SCI_CreateMutex(mutex->mutex_name, SCI_INHERIT);
}

static inline void rtw_mutex_lock(rtw_mutex *mutex)
{
	if (SCI_InThreadContext())
		SCI_GetMutex(mutex->sci_mutex, SCI_WAIT_FOREVER);
	else
		return;
}

static inline void rtw_mutex_unlock(rtw_mutex *mutex)
{
	if (SCI_InThreadContext())
		SCI_PutMutex(mutex);
	else
		return;
}

static inline void rtw_mutex_free(rtw_mutex *mutex)
{
	if(mutex == NULL || mutex->sci_mutex == NULL)
		return;
	SCI_DeleteMutex(mutex->sci_mutex);
	mutexidx --;
}

unsigned int timeridx = 0;
typedef void (*TIMER_FUN)(unsigned long);
static inline void rtw_setup_timer(struct timer_list *timer,
		  void (*func)(unsigned long), unsigned long cntx)
{
	rtw_memset((void*)timer->timer_name, 0, 64);
	strcat(timer->timer_name, "RTW_WIFI_TIMER_"#timeridx);
	timeridx ++;		
	
	timer->sci_timer = SCI_CreateTimer(timer->timer_name, (TIMER_FUN)func, (void*)timer, 1000, 0);
	timer->function = (TIMER_FUN)func;

	timer->context = (void*)cntx;
}

static inline int rtw_mod_timer(struct timer_list *timer, unsigned long ms)
{
	if(!timer)
		return;
	if (!timer->sci_timer)
		return;
	SCI_ChangeTimer(timer->sci_timer, timer->function, ms);
	SCI_ActiveTimer(timer->sci_timer);
}

static inline void _cancel_timer(struct timer_list *ptimer, u8 *bcancelled)
{
	int err = 0;

	if (ptimer && ptimer->sci_timer) {
		err = SCI_DeactiveTimer(ptimer->sci_timer);
		if(!err)
			*bcancelled=  true;
		else
			*bcancelled=  false;
	}
}

static inline void rtw_cancel_timer(struct timer_list *ptimer)
{
	u8 bcancelled = 0;
	u8 count = 0;
	
	while (1) {
		_cancel_timer(ptimer, &bcancelled);
		if (bcancelled == true || count > 5)
			break;
		count ++;
	}
	
	return bcancelled;
}

static inline void rtw_del_timer(struct timer_list *timer)
{
	u8 bcancelled = 0;

	if (timer && timer->sci_timer) {
		_cancel_timer(timer, &bcancelled);
		SCI_DeleteTimer(timer->sci_timer);
	}
	timer->sci_timer = NULL;
	timeridx --;
}

static inline int rtw_request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *data)
{
	//PLATFORM_TODO
	return 0;
}

static inline void rtw_free_irq(unsigned int irq, void *data)
{
	//PLATFORM_TODO
}

static inline int dma_map_sg_attrs(struct device *dev, struct scatterlist *sg,
				   int nents, enum dma_data_direction dir,
				   struct dma_attrs *attrs)
{
	//PLATFORM_TODO
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
