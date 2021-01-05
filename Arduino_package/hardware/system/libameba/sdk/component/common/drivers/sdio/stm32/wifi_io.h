#ifndef _WIFI_IO_H
#define _WIFI_IO_H

#include "core/sdio_irq.h"

struct sdio_driver {
	int fake;
};

//typedef unsigned long UINT32;
typedef struct _SDIO_BUS_OPS{
	int (*bus_probe)(void);
	int (*bus_remove)(void);
	int (*enable)(struct sdio_func*func);	/*enables a SDIO function for usage*/
	int (*disable)(struct sdio_func *func); 
	int (*reg_driver)(struct sdio_driver*); /*register sdio function device driver callback*/
	void (*unreg_driver)(struct sdio_driver *); 
	int (*claim_irq)(struct sdio_func *func, void(*handler)(struct sdio_func *));
	int (*release_irq)(struct sdio_func*func);
	void (*claim_host)(struct sdio_func*func);	/*exclusively claim a bus for a certain SDIO function*/
	void (*release_host)(struct sdio_func *func); 
	unsigned char (*readb)(struct sdio_func *func, unsigned int addr, int *err_ret);/*read a single byte from a SDIO function*/
	unsigned short (*readw)(struct sdio_func *func, unsigned int addr, int *err_ret);	/*read a 16 bit integer from a SDIO function*/
	unsigned int (*readl)(struct sdio_func *func, unsigned int addr, int *err_ret); /*read a 32 bit integer from a SDIO function*/
	void (*writeb)(struct sdio_func *func, unsigned char b,unsigned int addr, int *err_ret);	/*write a single byte to a SDIO function*/
	void (*writew)(struct sdio_func *func, unsigned short b,unsigned int addr, int *err_ret);	/*write a 16 bit integer to a SDIO function*/
	void (*writel)(struct sdio_func *func, unsigned int b,unsigned int addr, int *err_ret); /*write a 32 bit integer to a SDIO function*/
	int (*memcpy_fromio)(struct sdio_func *func, void *dst,unsigned int addr, int count);/*read a chunk of memory from a SDIO functio*/
	int (*memcpy_toio)(struct sdio_func *func, unsigned int addr,void *src, int count);  /*write a chunk of memory to a SDIO function*/ 
}SDIO_BUS_OPS;

extern SDIO_BUS_OPS rtw_sdio_bus_ops;
extern struct sdio_func *wifi_sdio_func;
extern int sdio_bus_probe(void);
extern int sdio_bus_remove(void);
#endif

