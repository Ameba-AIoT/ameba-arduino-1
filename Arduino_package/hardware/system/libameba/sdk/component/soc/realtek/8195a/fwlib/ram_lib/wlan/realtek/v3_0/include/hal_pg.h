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

#ifndef __HAL_PG_H__
#define __HAL_PG_H__

//
// For VHT series TX power by rate table.
// VHT TX power by rate off setArray = 
// Band:-2G&5G = 0 / 1
// RF: at most 4*4 = ABCD=0/1/2/3
// CCK=0 OFDM=1/2 HT-MCS 0-15=3/4/56 VHT=7/8/9/10/11			
//
#if defined(CONFIG_PLATFORM_8195A) || defined(CONFIG_PLATFORM_8711B)
#define TX_PWR_BY_RATE_NUM_BAND			2
#define TX_PWR_BY_RATE_NUM_RF			4
#else
#define TX_PWR_BY_RATE_NUM_BAND			1
#define TX_PWR_BY_RATE_NUM_RF			1
#endif
#define TX_PWR_BY_RATE_NUM_RATE			84

#define  MAX_PG_GROUP 		13
#define	RF_PATH_MAX		2				// Max 4 for ss larger than 2
#define 	MAX_RF_PATH		RF_PATH_MAX

//It must always set to 4, otherwise read efuse table secquence will be wrong.
#define 	MAX_TX_COUNT				4

#define	MAX_CHNL_GROUP_24G		6 
#define	MAX_CHNL_GROUP_5G		14 

#define CHANNEL_MAX_NUMBER		14	// 14 is the max channel number

typedef struct _TxPowerInfo24G{
	u8 IndexCCK_Base[MAX_RF_PATH][MAX_CHNL_GROUP_24G];
	u8 IndexBW40_Base[MAX_RF_PATH][MAX_CHNL_GROUP_24G];
	//If only one tx, only BW20 and OFDM are used.
	s8 CCK_Diff[MAX_RF_PATH][MAX_TX_COUNT];	
	s8 OFDM_Diff[MAX_RF_PATH][MAX_TX_COUNT];
	s8 BW20_Diff[MAX_RF_PATH][MAX_TX_COUNT];
	s8 BW40_Diff[MAX_RF_PATH][MAX_TX_COUNT];
}TxPowerInfo24G, *PTxPowerInfo24G;

#endif


