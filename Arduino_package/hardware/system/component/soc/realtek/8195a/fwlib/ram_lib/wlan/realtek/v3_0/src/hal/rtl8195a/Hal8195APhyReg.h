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
#ifndef __INC_HAL8195APHYREG_H__
#define __INC_HAL8195APHYREG_H__

#include "Hal8192CPhyReg.h"

//
// Page8(0x800)
//
#define		rFPGA0_RFMOD				0x800	//RF mode & CCK TxSC // RF BW Setting??
#define		rTxAGC_B_Rate18_06			0x830
#define		rTxAGC_B_Rate54_24			0x834
#define		rTxAGC_B_CCK1_55_Mcs32		0x838
#define		rTxAGC_B_Mcs03_Mcs00		0x83c
#define		rTxAGC_B_Mcs07_Mcs04		0x848
#define		rTxAGC_B_Mcs11_Mcs08		0x84c
#define		rTxAGC_B_CCK11_A_CCK2_11	0x86c
#define		rTxAGC_B_Mcs15_Mcs12		0x868

//
// Page9(0x900)
//
#define 	rS0S1_PathSwitch   			0x948
#define		rRXDFIR_Filter				0x954

//
// PageB(0xB00)
//
#define rPdp_AntA						0xb00
#define rPdp_AntA_4						0xb04
#define rPdp_AntA_8						0xb08
#define rPdp_AntA_C						0xb0c
#define rPdp_AntA_10					0xb10
#define rPdp_AntA_14					0xb14
#define rPdp_AntA_18					0xb18
#define rPdp_AntA_1C					0xb1c
#define rPdp_AntA_20					0xb20
#define rPdp_AntA_24					0xb24

#define rConfig_Pmpd_AntA 				0xb28
#define rConfig_ram64x16				0xb2c

#define rBndA							0xb30
#define rHssiPar						0xb34

#define rConfig_AntA					0xb68
#define rConfig_AntB					0xb6c

#define rPdp_AntB						0xb70
#define rPdp_AntB_4						0xb74
#define rPdp_AntB_8						0xb78
#define rPdp_AntB_C						0xb7c
#define rPdp_AntB_10					0xb80
#define rPdp_AntB_14					0xb84
#define rPdp_AntB_18					0xb88
#define rPdp_AntB_1C					0xb8c
#define rPdp_AntB_20					0xb90
#define rPdp_AntB_24					0xb94

#define rConfig_Pmpd_AntB				0xb98

#define rBndB							0xba0

#define rAPK							0xbd8
#define rPm_Rx0_AntA					0xbdc
#define rPm_Rx1_AntA					0xbe0
#define rPm_Rx2_AntA					0xbe4
#define rPm_Rx3_AntA					0xbe8
#define rPm_Rx0_AntB					0xbec
#define rPm_Rx1_AntB					0xbf0
#define rPm_Rx2_AntB					0xbf4
#define rPm_Rx3_AntB					0xbf8

//
// PageC(0xC00)
//
#define		rOFDM0_XARxAFE				0xc10	// RxIQ DC offset, Rx digital filter, DC notch filter
#define		rOFDM0_TxPseudoNoiseWgt		0xce4	// Double ADC

//
// PageE(0xE00)
//
#define		rTxAGC_A_Rate18_06			0xe00
#define		rTxAGC_A_Rate54_24			0xe04
#define		rTxAGC_A_CCK1_Mcs32			0xe08
#define		rTxAGC_A_Mcs03_Mcs00		0xe10
#define		rTxAGC_A_Mcs07_Mcs04		0xe14
#define		rTxAGC_A_Mcs11_Mcs08		0xe18
#define		rTxAGC_A_Mcs15_Mcs12		0xe1c
#define		rRx_Wait_CCA				0xe70	// Rx ADC clock

//for PutRegsetting & GetRegSetting BitMask
#define		bMaskH3Bytes				0xffffff00

//
// RL6052 Register definition
//
#define		RF_WE_LUT					0xEF


#endif

