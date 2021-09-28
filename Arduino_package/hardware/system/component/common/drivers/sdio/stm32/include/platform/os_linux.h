#ifndef __PLATFORM_LINUX_H__
#define __PLATFORM_LINUX_H__

#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/slab.h>//mem
#include <linux/kthread.h>

#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>

#include <linux/irq.h>
#include <linux/platform_device.h>

#ifdef PLATFORM_NONE
#include <linux/export.h>
#define clk_disable(a)
#define sci_glb_set(a, b)
#define sci_glb_clr(a, b)
#define sci_glb_raw_read(a)

#define REG_AHB_AHB_CTL0	0x0000
#define REG_AHB_SOFT_RST	0x0010

#define BIT_SDIO1_EB		BIT(19)
#define BIT_SD1_SOFT_RST	BIT(16)

#define REG_GLB_CLK_GEN5	0x007C
#endif

#define RTKDEBUG "rtw_sdio: "
#define dbg_host(...)     do {\
	printk(RTKDEBUG __VA_ARGS__);\
}while(0)

//==============================================================
#include <linux/dmaengine.h>
#include <linux/pagemap.h>
typedef struct dma_async_tx_descriptor rtw_dma_async_tx_descriptor;
typedef struct dma_chan rtw_dma_chan;
typedef unsigned int rtw_dma_cookie_t;
typedef struct spinlock rtw_spinlock_t;
typedef struct semaphore	rtw_sema;
typedef struct mutex	rtw_mutex;

#define rtw_dma_request_channel(mask, x, y) __dma_request_channel(&(mask), x, y)
static inline void rtw_dma_release_channel(rtw_dma_chan *chan)
{
	dma_release_channel(chan);
}
//==============================================================
#define rtw_spin_lock(lock)				spin_lock(lock)
#define rtw_spin_unlock(lock)				spin_unlock(lock)
#define rtw_spin_lock_init(lock)			spin_lock_init(lock)
#define rtw_spin_lock_free(lock)			//nothing
#define rtw_request_irq(a,b,c,d,e)			request_irq(a,b,c,d,e)
#define rtw_free_irq(a,b)					free_irq(a,b)

static inline unsigned int rtw_min(const unsigned int a, const unsigned int b)
{
	return min(a, b);
}

static inline void rtw_msleep(int ms)
{
	msleep(ms);
}

static inline void rtw_mdelay(int ms)
{
	mdelay(ms);
}

static inline void rtw_udelay(int us)
{
	udelay(us);
}

static inline void rtw_memcpy(void* dst, void* src, unsigned int sz)
{
	memcpy(dst, src, sz);
}

static inline void rtw_memset(void *pbuf, int c, unsigned int  sz)
{
	memset(pbuf, c, sz);
}

static inline void *rtw_kmalloc(unsigned int size, gfp_t flags)
{
	void *ret = kmalloc(size, flags);
	return ret;

}

static inline void rtw_kfree(void *p)
{
	kfree(p);
}

static inline void *rtw_kzalloc(unsigned int size, gfp_t flags)
{
	return kzalloc(size, flags);
}

static inline char *rtw_strcpy(char *dest, const char *src)
{
	return strcpy(dest, src);
}

static inline unsigned int rtw_strlen(const char *s)
{
	return strlen(s);
}

static inline void rtw_sema_init(rtw_sema *sem, int val)
{
	sema_init(sem, val);
}

static inline void rtw_sema_free(rtw_sema *sem)
{
	//Nothing
}

static inline void rtw_sig_sem(rtw_sema *sem)
{
	up(sem);
}

#define WAIT_SEM_INFINITE		0x7FFFFFFF
static inline void rtw_wait_sem(rtw_sema *sem, u32 to_ms)
{
	down(sem);
}

static inline void rtw_mutex_init(rtw_mutex *mutex)
{
	mutex_init(mutex);
}

static inline void rtw_mutex_lock(rtw_mutex *lock)
{
	mutex_lock(lock);
}

static inline void rtw_mutex_unlock(rtw_mutex *lock)
{
	mutex_unlock(lock);
}

static inline void rtw_mutex_free(rtw_mutex *mutex)
{
	//Nothing
}

static inline void rtw_setup_timer(struct timer_list *t,
		  void (*func)(unsigned long), unsigned long data)
{
	setup_timer(t, func, data);
}

static inline int rtw_mod_timer(struct timer_list *timer, unsigned long ms)
{
	return mod_timer(timer, jiffies + (ms/1000) * HZ);
}

static inline void rtw_cancel_timer(struct timer_list *timer)
{
	del_timer(timer);
}

static inline void rtw_del_timer(struct timer_list *timer)
{
	del_timer(timer);
}

#endif
