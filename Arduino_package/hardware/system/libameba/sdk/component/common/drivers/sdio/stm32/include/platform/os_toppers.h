#ifndef __PLATFORM_TOPPERS_H__
#define __PLATFORM_TOPPERS_H__

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned int bool;

#include "os_nolinux.h"
#include "kernel.h"
#include "kernel/kernel_impl.h"
#include "kernel/time_event.h"
#include "t_stddef.h"
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "stdarg.h"
#include "itron.h"
#include "rtl_timer.h"

extern ID sem1_id_sdio[];
extern int_t sem1_is_used[];
extern int_t sema1_num_sdio;
extern ID sem0_id_sdio[];
extern int_t sem0_is_used[];
extern int_t sema0_num_sdio;
#define NUM_SEM1	9
#define NUM_SEM0	4

#define RTKDEBUG "rtw_sdio: "
#define dbg_host(...)     do {\
	syslog(LOG_NOTICE, RTKDEBUG __VA_ARGS__);\
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
	ID id;
	int init_val;
}rtw_spinlock_t, rtw_sema;


typedef struct mutex {
	ID id;
	int init_val;
}rtw_mutex;

#if 0 //in rtl_timer.h
struct timer_list {
	TMEVTB timer;

	void (*function)(unsigned long);
	void *context;
};
#endif

static inline int rtw_request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *data)
{
	//done in sdio.cfg use DEF_INH
	//act_tsk(SDIO_SDHI_IRQ_TASK);
	ena_int(irq);
	return 0;
}

static inline void rtw_free_irq(unsigned int irq, void *data)
{
	//done in sdio.cfg use DEF_INH
	dis_int(irq);
}

static inline void rtw_msleep(int ms)
{
	TMO tmout;
	tmout = (TMO)ms;
	tslp_tsk(tmout);
}

static inline void rtw_mdelay(int ms)
{
	RELTIM delay_time;
	delay_time = (RELTIM)ms;
	dly_tsk(delay_time);
}

static inline void rtw_udelay(int us)
{	
	volatile unsigned long i;
	unsigned long num_loop = us << 1;

	for (i = 0; i < num_loop; i++) {
		sil_dly_nse(512); //0.5us
	}
}

static inline void rtw_sema_init(rtw_sema *sema, int init_val)
{
	int i = 0;

	if (init_val == 1) {
		if (sema1_num_sdio < NUM_SEM1) {
			for (i = 0; i < NUM_SEM1; i++) {
				if (sem1_is_used[i] == 0) {				
					sem1_is_used[i] = 1;
					sema->id = sem1_id_sdio[i];
					sema->init_val = init_val;
					sema1_num_sdio++;
					ini_sem(sema->id);
					//syslog(LOG_NOTICE, "rtw_sema1_init idx:%d semaid:%d. sema:%p\n", i, sema->id, sema);
					return;
				}
			}
		}
	} else if (init_val == 0) {
		if (sema0_num_sdio < NUM_SEM0) {
			for (i = 0; i < NUM_SEM0; i++) {
				if (sem0_is_used[i] == 0) {				
					sem0_is_used[i] = 1;
					sema->id = sem0_id_sdio[i];
					sema->init_val = init_val;
					sema0_num_sdio++;
					ini_sem(sema->id);
					//syslog(LOG_NOTICE, "rtw_sema0_init idx:%d semaid:%d. sema:%p\n", i, sema->id, sema);
					return;
				}
			}
		}
	} else {
		syslog(LOG_NOTICE, "rtw_sema_init Oops: just support init to 0 or 1.\n");
	}

	sema->id = 0;
	syslog(LOG_NOTICE, "rtw_sema_init Oops: fail.\n");
}

static inline void rtw_sema_free(rtw_sema *sema)
{
	int i = 0;

	if (sema->id == 0)
	{
		syslog(LOG_NOTICE, "rtw_sema_free  (semaid == 0). %p", sema);
		return;
	}

	if (sema->init_val == 1) {
		for (i = 0; i < NUM_SEM1; i++) {
			if (sem1_id_sdio[i] == sema->id) {
				sem1_is_used[i] = 0;
				sema1_num_sdio--;
				//syslog(LOG_NOTICE, "rtw_sema1_free idx:%d semaid:%d. sema:%p\n", i, sema->id, sema);
			}
		}
	} else {
		for (i = 0; i < NUM_SEM0; i++) {
			if (sem0_id_sdio[i] == sema->id) {
				sem0_is_used[i] = 0;
				sema0_num_sdio--;
				//syslog(LOG_NOTICE, "rtw_sema0_free idx:%d semaid:%d. sema:%p\n", i, sema->id, sema);
			}
		}
	}
	sema->id = 0;
}

static inline void rtw_sig_sem(rtw_sema *sema)
{
	if (sema->id == 0)
	{
		syslog(LOG_NOTICE, "rtw_sig_sem (semaid == 0). %p", sema);
		return;
	}
	//if (sema->init_val == 0)
	//	syslog(LOG_NOTICE, "rtw_sig_sem sema:%d init_val:%d.\n", sema->id, sema->init_val);

	if (sema->init_val)
		sig_sem(sema->id);
	else
		isig_sem(sema->id);
}

#define WAIT_SEM_INFINITE		0x7FFFFFFF
static inline void rtw_wait_sem(rtw_sema *sema, u32 to_ms)
{
	if (sema == NULL || sema->id == 0)
	{
		syslog(LOG_NOTICE, "rtw_wait_sem (semaid == 0). %p", sema);
		return;
	}
	//if (sema->init_val == 0)
	//	syslog(LOG_NOTICE, "rtw_wait_sem sema:%d init_val:%d.\n", sema->id, sema->init_val);
	wai_sem(sema->id);
}

static inline void rtw_mutex_init(rtw_mutex *pmutex)
{
	rtw_sema *sema;
	sema = (rtw_sema *)pmutex;
	rtw_sema_init(sema, 1);
}

static inline void rtw_mutex_lock(rtw_mutex *pmutex)
{
	rtw_sema *sema;
	sema = (rtw_sema *)pmutex;
	rtw_wait_sem(sema, WAIT_SEM_INFINITE);
}

static inline void rtw_mutex_unlock(rtw_mutex *pmutex)
{
	rtw_sema *sema;
	sema = (rtw_sema *)pmutex;
	rtw_sig_sem(sema);
}

static inline void rtw_mutex_free(rtw_mutex *pmutex)
{
	rtw_sema *sema;
	sema = (rtw_sema *)pmutex;
	rtw_sema_free(sema);
}

static inline void rtw_spin_lock_init(rtw_spinlock_t *plock)
{
	rtw_sema *sema;
	sema = (rtw_sema *)plock;
	rtw_sema_init(sema, 1);
}

static inline  void rtw_spin_lock_free(rtw_spinlock_t *plock)
{
	rtw_sema *sema;
	sema = (rtw_sema *)plock;
	rtw_sema_free(sema);
}

static inline  void rtw_spin_lock(rtw_spinlock_t *plock)
{
	rtw_sema *sema;
	sema = (rtw_sema *)plock;
	rtw_wait_sem(sema, WAIT_SEM_INFINITE);
}

static inline void rtw_spin_unlock(rtw_spinlock_t *plock)
{
	rtw_sema *sema;
	sema = (rtw_sema *)plock;
	rtw_sig_sem(sema);
}

static inline void rtw_setup_timer(struct timer_list *ptimer, void (*pfunc)(unsigned long), unsigned long cntx)
{
	init_timer(ptimer, pfunc, (void*)cntx, NULL, "sdio_timer");
}

static inline int rtw_mod_timer(struct timer_list *ptimer, unsigned long delay_time)
{
	if (ptimer) {
		set_timer(ptimer, delay_time);
	}

	return 0;
}

static inline void rtw_cancel_timer(struct timer_list *ptimer)
{
	if (ptimer) {
		cancel_timer_ex(ptimer);
	}
}

static inline int rtw_del_timer(struct timer_list *ptimer)
{
	if (ptimer) {
		cancel_timer_ex(ptimer);
		del_timer_sync(ptimer);
	}
	return 0;
}

static inline int dma_map_sg_attrs(struct device *dev, struct scatterlist *sg,
				   int nents, enum dma_data_direction dir,
				   struct dma_attrs *attrs)
{
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
