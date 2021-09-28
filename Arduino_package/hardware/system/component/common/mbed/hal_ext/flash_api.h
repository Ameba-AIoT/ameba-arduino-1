/* mbed Microcontroller Library
 *******************************************************************************
 * Copyright (c) 2014, Realtek Semiconductor Corp.
 * All rights reserved.
 *
 * This module is a confidential and proprietary property of RealTek and
 * possession or use of this module requires written permission of RealTek.
 *******************************************************************************
 */

 #ifndef MBED_EXT_FLASH_API_EXT_H
#define MBED_EXT_FLASH_API_EXT_H

#include "device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct flash_s flash_t;

/**
  * global data structure
  */   
extern flash_t	        flash;

enum {
  FLASH_COMPLETE = 0,
  FLASH_ERROR_2 = 1,
};

//void flash_init         		(flash_t *obj);
void flash_erase_sector			(flash_t *obj, uint32_t address);
void flash_erase_block(flash_t * obj, uint32_t address);
int  flash_read_word			(flash_t *obj, uint32_t address, uint32_t * data);
int  flash_write_word			(flash_t *obj, uint32_t address, uint32_t data);
int  flash_stream_read          (flash_t *obj, uint32_t address, uint32_t len, uint8_t * data);
int  flash_stream_write         (flash_t *obj, uint32_t address, uint32_t len, uint8_t * data);
void flash_write_protect        (flash_t *obj, uint32_t protect);
int flash_get_status(flash_t * obj);
int flash_set_status(flash_t * obj, uint32_t data);
void flash_reset_status(flash_t * obj);
int flash_burst_write(flash_t * obj, uint32_t address, uint32_t Length, uint8_t * data);
int flash_burst_read(flash_t * obj, uint32_t address, uint32_t Length, uint8_t * data);
int flash_set_extend_addr(flash_t * obj, uint32_t data);
int flash_get_extend_addr(flash_t * obj);
int flash_read_id(flash_t *obj, uint8_t *buf, uint8_t len);
int flash_read_unique_id(flash_t *obj, uint8_t *buf, uint8_t len);
void flash_set_lock_mode(uint32_t mode);
void flash_global_lock(void);
void flash_global_unlock(void);
void flash_individual_lock(uint32_t address);
void flash_individual_unlock(uint32_t address);
int flash_read_individual_lock_state(uint32_t address);

#ifdef __cplusplus
}
#endif


#endif
