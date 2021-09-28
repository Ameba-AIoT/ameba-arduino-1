#ifndef __HAL_DATA_H__
#define __HAL_DATA_H__


#include "hal_com.h"
#include "OUTSRC/phydm_precomp.h"
#ifdef CONFIG_RTL8723A
#include "rtl8723a/rtl8723a_hal.h"
#endif
#ifdef CONFIG_RTL8188E
#include "rtl8188e/rtl8188e_hal.h"
#endif
#ifdef CONFIG_RTL8195A
#include "rtl8195a_hal.h"
#endif
#if defined(CONFIG_RTL8711B)
#include "rtl8195a_hal.h"
#endif
//
// <Roger_Notes> For RTL8723 WiFi/BT/GPS multi-function configuration. 2010.10.06.
//
typedef enum _RT_MULTI_FUNC{
	RT_MULTI_FUNC_NONE	= 0x00,
	RT_MULTI_FUNC_WIFI 	= 0x01,
	RT_MULTI_FUNC_BT 		= 0x02,
	RT_MULTI_FUNC_GPS 	= 0x04,
}RT_MULTI_FUNC,*PRT_MULTI_FUNC;
//
// <Roger_Notes> For RTL8723 WiFi PDn/GPIO polarity control configuration. 2010.10.08.
//
typedef enum _RT_POLARITY_CTL {
	RT_POLARITY_LOW_ACT 	= 0,
	RT_POLARITY_HIGH_ACT 	= 1,	
} RT_POLARITY_CTL, *PRT_POLARITY_CTL;

// For RTL8723 regulator mode. by tynli. 2011.01.14.
typedef enum _RT_REGULATOR_MODE {
	RT_SWITCHING_REGULATOR 	= 0,
	RT_LDO_REGULATOR 			= 1,
} RT_REGULATOR_MODE, *PRT_REGULATOR_MODE;

#define MAX_BASE_NUM_IN_PHY_REG_PG_2_4G			10 //  CCK:1,OFDM:1, HT:4, VHT:4
#define MAX_BASE_NUM_IN_PHY_REG_PG_5G			9 // OFDM:1, HT:4, VHT:4

struct 	dm_priv	
{
	u8	DM_Type;
	u8	DMFlag;
	u8	InitDMFlag;
	u32	InitODMFlag;

	//* Upper and Lower Signal threshold for Rate Adaptive*/
	int	UndecoratedSmoothedPWDB;
	int	UndecoratedSmoothedCCK;
	int	EntryMinUndecoratedSmoothedPWDB;
	int	EntryMaxUndecoratedSmoothedPWDB;
	int	MinUndecoratedPWDBForDM;
	int	LastMinUndecoratedPWDBForDM;


	//for High Power
	u8 	bDynamicTxPowerEnable;
	u8 	LastDTPLvl;
	u8 	DynamicTxHighPowerLvl;//Add by Jacken Tx Power Control for Near/Far Range 2008/03/06
	
	//for tx power tracking
	u8	bTXPowerTracking;
	u8	TXPowercount;
	u8	bTXPowerTrackingInit;
	u8	TxPowerTrackControl;	//for mp mode, turn off txpwrtracking as default
	
	u8	PowerIndex_backup[6];

	s32	OFDM_Pkt_Cnt;
	u8	RSSI_Select;

	// Add for Reading Initial Data Rate SEL Register 0x484 during watchdog. Using for fill tx desc. 2011.3.21 by Thomas
	u8	INIDATA_RATE[32];
};

#ifdef CONFIG_LITTLE_WIFI_MCU_FUNCTION_THREAD
#define MCUCMDQUEUEDEPTH    15
#define MCUCMDLENGTH        2
#define MACIDNUM            128
typedef struct _cmd_queue_{
    u32                 FwCmdContent[MCUCMDLENGTH];
}CMD_QUEUE;
#endif


typedef struct hal_com_data
{
	HAL_VERSION			VersionID;
	RT_MULTI_FUNC		MultiFunc; // For multi-function consideration.
	RT_POLARITY_CTL		PolarityCtl; // For Wifi PDn Polarity control.
	RT_REGULATOR_MODE	RegulatorMode; // switching regulator or LDO
	u16	CustomerID;
#if defined(CONFIG_PLATFORM_8195A) || defined(CONFIG_PLATFORM_8711B)
	u16 ForcedDataRate;// Force Data Rate. 0: Auto, 0x02: 1M ~ 0x6C: 54M.
#endif

	u16	FirmwareVersion;
	u16	FirmwareVersionRev;
	u16	FirmwareSubVersion;
	u16	FirmwareSignature;
	u8	PGMaxGroup;
	//current WIFI_PHY values
	u32	ReceiveConfig;
	WIRELESS_MODE		CurrentWirelessMode;
	CHANNEL_WIDTH	CurrentChannelBW;
#if defined(CONFIG_PLATFORM_8195A) || defined(CONFIG_PLATFORM_8711B)
	BAND_TYPE			CurrentBandType;	//0:2.4G, 1:5G
#endif
	u8	CurrentChannel;
	u8	nCur40MhzPrimeSC;// Control channel sub-carrier
    u8	CurrentCenterFrequencyIndex1;
	u8	nCur80MhzPrimeSC;   //used for primary 40MHz of 80MHz mode

	u16	BasicRateSet;

	//rf_ctrl
	u8	rf_chip;
	u8	rf_type;
#if defined(CONFIG_PLATFORM_8195A) || defined(CONFIG_PLATFORM_8711B)
	u8	PackageType;
	u8	ChipID;
#endif
	u8	NumTotalRFPath;

	u8	BoardType;

	//
	// EEPROM setting.
	//
	u16	EEPROMVID;
	u16	EEPROMPID;
	u16	EEPROMSVID;
	u16	EEPROMSDID;
	u8	EEPROMCustomerID;
	u8	EEPROMSubCustomerID;
	u8	EEPROMVersion;
	u8	EEPROMRegulatory;

	u8	bTXPowerDataReadFromEEPORM;
	u8	EEPROMThermalMeter;
	u8	bAPKThermalMeterIgnore;

	BOOLEAN 			EepromOrEfuse;
	//u8				EfuseMap[2][HWSET_MAX_SIZE_512]; //92C:256bytes, 88E:512bytes, we use union set (512bytes)
	u8				EfuseUsedPercentage;
#ifdef HAL_EFUSE_MEMORY
	EFUSE_HAL			EfuseHal;
#endif	
	//u8	bIQKInitialized;

	
	u8	Index24G_CCK_Base[MAX_RF_PATH][CHANNEL_MAX_NUMBER];
	u8	Index24G_BW40_Base[MAX_RF_PATH][CHANNEL_MAX_NUMBER];
	//If only one tx, only BW20 and OFDM are used.
	s8	CCK_24G_Diff[MAX_RF_PATH][MAX_TX_COUNT];	
	s8	OFDM_24G_Diff[MAX_RF_PATH][MAX_TX_COUNT];
	s8	BW20_24G_Diff[MAX_RF_PATH][MAX_TX_COUNT];
	s8	BW40_24G_Diff[MAX_RF_PATH][MAX_TX_COUNT];

	u8	Regulation2_4G;
	u8	Regulation5G;
#if defined(CONFIG_PLATFORM_8195A) || defined(CONFIG_PLATFORM_8711B)
	s8	TxPwrByRateOffset[TX_PWR_BY_RATE_NUM_BAND]
						 [TX_PWR_BY_RATE_NUM_RF]
						 [TX_PWR_BY_RATE_NUM_RF]
						 [TX_PWR_BY_RATE_NUM_RATE];
#endif
	//---------------------------------------------------------------------------------//

	//2 Power Limit Table 
	u8	TxPwrLevelCck[RF_PATH_MAX][CHANNEL_MAX_NUMBER];
	u8	TxPwrLevelHT40_1S[RF_PATH_MAX][CHANNEL_MAX_NUMBER];	// For HT 40MHZ pwr
	u8	TxPwrLevelHT40_2S[RF_PATH_MAX][CHANNEL_MAX_NUMBER];	// For HT 40MHZ pwr
	u8	TxPwrHt20Diff[RF_PATH_MAX][CHANNEL_MAX_NUMBER];// HT 20<->40 Pwr diff
	u8	TxPwrLegacyHtDiff[RF_PATH_MAX][CHANNEL_MAX_NUMBER];// For HT<->legacy pwr diff
	
#if defined(CONFIG_PLATFORM_8195A) || defined(CONFIG_PLATFORM_8711B)
	// Store the original power by rate value of the base of each rate section of rf path A & B
	u8	TxPwrByRateBase2_4G[TX_PWR_BY_RATE_NUM_RF]
						[TX_PWR_BY_RATE_NUM_RF]
						[MAX_BASE_NUM_IN_PHY_REG_PG_2_4G];
	u8	TxPwrByRateBase5G[TX_PWR_BY_RATE_NUM_RF]
						[TX_PWR_BY_RATE_NUM_RF]
						[MAX_BASE_NUM_IN_PHY_REG_PG_5G];
#endif

	// For power group
	u8	PwrGroupHT20[RF_PATH_MAX][CHANNEL_MAX_NUMBER];
	u8	PwrGroupHT40[RF_PATH_MAX][CHANNEL_MAX_NUMBER];

	u8	LegacyHTTxPowerDiff;// Legacy to HT rate power diff
	// The current Tx Power Level
	u8	CurrentCckTxPwrIdx;
	u8	CurrentOfdm24GTxPwrIdx;
	u8	CurrentBW2024GTxPwrIdx;
	u8	CurrentBW4024GTxPwrIdx;
	

	// Read/write are allow for following hardware information variables
	u8	framesync;
	u32	framesyncC34;
	u8	framesyncMonitor;
	u8	DefaultInitialGain[4];
	u8	pwrGroupCnt;	
	u32	MCSTxPowerLevelOriginalOffset[MAX_PG_GROUP][16];
	u32	CCKTxPowerLevelOriginalOffset;

	u8	CrystalCap;
	//u32	AntennaTxPath;					// Antenna path Tx
	//u32	AntennaRxPath;					// Antenna path Rx
	//u8	BluetoothCoexist;
	u8	ExternalPA;

	//u8	bLedOpenDrain; // Support Open-drain arrangement for controlling the LED. Added by Roger, 2009.10.16.

	//u32	LedControlNum;
	//u32	LedControlMode;
	//u32	TxPowerTrackControl;
	//u8	b1x1RecvCombine;	// for 1T1R receive combining

	//u8	bCurrentTurboEDCA;
#if defined(CONFIG_RTL8195A) || defined(CONFIG_RTL8711B)
    BOOLEAN     bSwChnl;    
    BOOLEAN     bSetChnlBW; 
    BOOLEAN     bChnlBWInitialized; 
#endif

	BOOLEAN		bNeedIQK;
	
	u32	AcParam_BE; //Original parameter for BE, use for EDCA turbo.	

	BB_REGISTER_DEFINITION_T	PHYRegDef[4];	//Radio A/B/C/D

	u32	RfRegChnlVal[2];

	//RDG enable
	BOOLEAN	 bRDGEnable;

	//for host message to fw
	u8	LastHMEBoxNum;

	u8	fw_ractrl;
	u8	RegTxPause;
	// Beacon function related global variable.
	u32	RegBcnCtrlVal;
	u8	RegFwHwTxQCtrl;
	u8	RegReg542;
	u8	RegCR_1;
	u16	RegRRSR;

	struct dm_priv	dmpriv;
	DM_ODM_T 		odmpriv;
	//_lock			odm_stainfo_lock;
#ifdef DBG_CONFIG_ERROR_DETECT
	struct sreset_priv srestpriv;
#endif

#ifdef CONFIG_BT_COEXIST
	struct btcoexist_priv	bt_coexist;
#endif

	u8	CurAntenna;
	u8	AntDivCfg;
	u8	TRxAntDivType;


	u8	bDumpRxPkt;//for debug
	u8	bDumpTxPkt;//for debug
	u8	FwRsvdPageStartOffset; //2010.06.23. Added by tynli. Reserve page start offset except beacon in TxQ.

	// 2010/08/09 MH Add CU power down mode.
	BOOLEAN		pwrdown;

	// Add for dual MAC  0--Mac0 1--Mac1
	u32	interfaceIndex;

	u8	OutEpQueueSel;
	u8	OutEpNumber;

	// 2010/12/10 MH Add for USB aggreation mode dynamic shceme.
	BOOLEAN		UsbRxHighSpeedMode;

	// 2010/11/22 MH Add for slim combo debug mode selective.
	// This is used for fix the drawback of CU TSMC-A/UMC-A cut. HW auto suspend ability. Close BT clock.
	//BOOLEAN		SlimComboDbg;

	u16	EfuseUsedBytes;

#ifdef CONFIG_P2P
	struct P2P_PS_Offload_t	p2p_ps_offload;
#endif

	u8	AMPDUDensity;

    // Auto FSM to Turn On, include clock, isolation, power control for MAC only
	u8			bMacPwrCtrlOn;

#if defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)

	//
	// For SDIO Interface HAL related
	//

	//
	// SDIO ISR Related
	//
//	u32			IntrMask[1];
//	u32			IntrMaskToSet[1];
//	LOG_INTERRUPT		InterruptLog;
//	u32			sdio_himr;
//	u32			sdio_hisr;

	//
	// SDIO Tx FIFO related.
	//
	// HIQ, MID, LOW, PUB free pages; padapter->xmitpriv.free_txpg
//	u8			SdioTxFIFOFreePage[TX_FREE_PG_QUEUE];
//	_lock		SdioTxFIFOFreePageLock;
//	_thread_hdl_	SdioXmitThread;
//	_sema		SdioXmitSema;
//	_sema		SdioXmitTerminateSema;

	//
	// SDIO Rx FIFO related.
	//
//	u8			SdioRxFIFOCnt;
//	u16			SdioRxFIFOSize;
#endif //CONFIG_SDIO_HCI

#ifdef CONFIG_USB_HCI
	u32	UsbBulkOutSize;

	// Interrupt relatd register information.
	u32	IntArray[3];//HISR0,HISR1,HSISR
	u32	IntrMask[3];
	//u8	C2hArray[16];
#ifdef CONFIG_USB_TX_AGGREGATION
	u8	UsbTxAggMode;
	u8	UsbTxAggDescNum;
#endif
#ifdef CONFIG_USB_RX_AGGREGATION
	u16	HwRxPageSize;				// Hardware setting
	u32	MaxUsbRxAggBlock;

	USB_RX_AGG_MODE	UsbRxAggMode;
	u8	UsbRxAggBlockCount;			// USB Block count. Block size is 512-byte in hight speed and 64-byte in full speed
	u8	UsbRxAggBlockTimeout;
	u8	UsbRxAggPageCount;			// 8192C DMA page count
	u8	UsbRxAggPageTimeout;
#endif
#endif //CONFIG_USB_HCI


#if defined (CONFIG_PCI_HCI) || defined(CONFIG_LX_HCI)
	//
	// EEPROM setting.
	//

	u16	EEPROMDID;
	u16	EEPROMSMID;
	u16	EEPROMChannelPlan;
	
	u8	EEPROMTSSI[2];
	u8	EEPROMBoardType;
	u32	TransmitConfig;	

	u32	IntArray[3];
	u32	IntrMask[3];
	
	u8	bDefaultAntenna;
	u8	bIQKInitialized;
	
	u8	bInterruptMigration;
	u8	bDisableTxInt;
	u8	bGpioHwWpsPbc;

	u16	RxExpectTag;
#ifdef CONFIG_DEBUG_DYNAMIC	
	struct hal_debug	debug_info;
#endif

#endif //CONFIG_PCI_HCI || CONFIG_LX_HCI


#ifdef CONFIG_TX_EARLY_MODE
	u8 			bEarlyModeEnable;
#endif

#ifdef CONFIG_LITTLE_WIFI_MCU_FUNCTION_THREAD
    struct task_struct	littlewifipriv;
    //CMD_QUEUE           FwCmdQueue[MCUCMDQUEUEDEPTH];
    //fw section    
    u32                 WifiMcuCmdBitMap;
    u8                  bConnected[MACIDNUM/8];
    BOOLEAN             PMUTaskRAEn;

    u8                  BcnIgnoreEdccaEn;

    #ifdef CONFIG_POWER_SAVING
    struct task_struct  enter32kpriv;

    #ifdef TDMA_POWER_SAVING
    struct task_struct  TDMApriv;
    #endif //#ifdef TDMA_POWER_SAVING
    
    PS_PARM             PSParmpriv;
    u8                  ScanEn;
    #endif //#ifdef CONFIG_POWER_SAVING
#endif

} HAL_DATA_COMMON, *PHAL_DATA_COMMON;

typedef struct hal_com_data HAL_DATA_TYPE, *PHAL_DATA_TYPE;
#define GET_HAL_DATA(__pAdapter)	((HAL_DATA_TYPE *)((__pAdapter)->HalData))

#endif //__HAL_DATA_H__
