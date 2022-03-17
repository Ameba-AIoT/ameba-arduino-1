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

#define NULL	0
#define true 1
#define false 0

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
	//PLATFORM_TODO
	char mutex_name[64];
}rtw_sema;
typedef struct mutex {
	//PLATFORM_TODO
	char mutex_name[64];
}rtw_mutex;
typedef struct mutex rtw_spinlock_t;

struct timer_list {
	//PLATFORM_TODO
	char mutex_name[64];
};



#define rtw_spin_lock(lock)				//PLATFORM_TODO
#define rtw_spin_unlock(lock)				//PLATFORM_TODO
#define rtw_spin_lock_init(lock)			//PLATFORM_TODO
#define rtw_spin_lock_free(lock)			//PLATFORM_TODO

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

static inline void rtw_msleep(int ms)
{
	//PLATFORM_TODO
}

static inline void rtw_mdelay(int ms)
{
	//PLATFORM_TODO
}

static inline void rtw_udelay(int us)
{
	//PLATFORM_TODO
}

static inline void *rtw_kmalloc(unsigned int size, gfp_t flags)
{
	void *ret = NULL;
	//PLATFORM_TODO
	return ret;

}

static inline void rtw_kfree(void *p)
{
	//PLATFORM_TODO
}

static inline void *rtw_kzalloc(unsigned int size, gfp_t flags)
{
	void *ret = NULL;
	//PLATFORM_TODO
	return ret;
}

static inline void rtw_sema_init(rtw_sema *sem, int val)
{
	//PLATFORM_TODO
}

static inline void rtw_sema_free(rtw_sema *sem)
{
	//PLATFORM_TODO
}

static inline void rtw_sig_sem(rtw_sema *sem)
{
	//PLATFORM_TODO
}

#define WAIT_SEM_INFINITE		0x7FFFFFFF
static inline void rtw_wait_sem(rtw_sema *sem, u32 to_ms)
{
	//PLATFORM_TODO
}

static inline void rtw_mutex_init(rtw_mutex *mutex)
{
	//PLATFORM_TODO
}

static inline void rtw_mutex_lock(rtw_mutex *lock)
{
	//PLATFORM_TODO
}

static inline void rtw_mutex_unlock(rtw_mutex *lock)
{
	//PLATFORM_TODO
}

static inline void rtw_mutex_free(rtw_mutex *mutex)
{
	//PLATFORM_TODO
}

static inline void rtw_setup_timer(struct timer_list *t,
		  void (*func)(unsigned long), unsigned long data)
{
	//PLATFORM_TODO
}

static inline int rtw_mod_timer(struct timer_list *timer, unsigned long expires)
{
	//PLATFORM_TODO
	return 1;
}

static inline void rtw_cancel_timer(struct timer_list *ptimer)
{
	//PLATFORM_TODO
}

static inline int rtw_del_timer(struct timer_list *timer)
{
	//PLATFORM_TODO
	return 0;
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
