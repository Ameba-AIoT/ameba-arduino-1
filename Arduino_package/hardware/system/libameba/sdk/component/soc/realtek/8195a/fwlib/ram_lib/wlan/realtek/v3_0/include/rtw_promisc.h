/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *                                        
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

#ifndef _RTW_PROMISC_H_
#define _RTW_PROMISC_H_
#include <drv_types.h>
#ifdef CONFIG_PROMISC
void promisc_deinit(_adapter *padapter);
void promisc_set_enable(_adapter *padapter, u8 enabled, u8 len_used);
int promisc_recv_func(_adapter *padapter, union recv_frame *rframe);
#endif
int promisc_set(rtw_rcr_level_t enabled, void (*callback)(unsigned char*, unsigned int, void*), unsigned char len_used);
unsigned char promisc_enabled(void);
int promisc_fixed_channel_num(void * fixed_bssid, u8 * ssid, int *ssid_length);
#endif	//_RTW_PROMISC_H_

