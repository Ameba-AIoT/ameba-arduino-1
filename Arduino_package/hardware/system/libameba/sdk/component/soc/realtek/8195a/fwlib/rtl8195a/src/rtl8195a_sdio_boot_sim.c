/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2013 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */
#define E_CUT_ROM_DOMAIN     1
#include "rtl8195a.h" 
#include "hal_sdio.h"
#include "rtl_consol.h"
#include "hal_sdr_controller.h"

extern u32 ConfigDebugErr;
extern u32 ConfigDebugInfo;
extern u32 ConfigDebugWarn;

//extern VOID
//HalInitPlatformLogUart(VOID);

extern VOID
HalReInitPlatformLogUartV02(VOID);

extern VOID
HalCpuClkConfig(IN u8 CpuType);

#ifdef CONFIG_TIMER_MODULE
extern u32 HalDelayUs(IN  u32 us);
#endif

extern u32 SdrControllerInit_rom(DRAM_DEVICE_INFO *DramInfo);
extern DRAM_DEVICE_INFO SdrDramInfo_rom;

#if defined(CONFIG_SDIO_BOOT_SIM) || defined(CONFIG_SDIO_BOOT_ROM)
extern COMMAND_TABLE UartLogRomCmdTable[];
extern VOID UartLogCmdExecute(PUART_LOG_CTL   pUartLogCtlExe);
extern _LONG_CALL_ VOID RtlConsolInit(u32 Boot, u32 TBLSz, VOID *pTBL);
extern u32 GetRomCmdNum(VOID);
extern VOID UartLogIrqHandleRam(VOID * Data);
extern VOID UartLogIrqHandle(VOID * Data);

#endif
extern UART_LOG_CTL *pUartLogCtl;
extern HAL_TIMER_OP HalTimerOp;
extern HAL_Status RuartIsTimeout (u32 StartCount, u32 TimeoutCnt);  // use to check timeout
#define SDIO_IsTimeout(StartCount, TimeoutCnt)      RuartIsTimeout(StartCount, TimeoutCnt)

/******************************************************************************
 * Function Prototype Declaration
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION BOOL SDIO_Device_Init_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Device_DeInit_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_IRQ_Handler_Rom(
	IN VOID *pData
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Interrupt_Init_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Interrupt_DeInit_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Enable_Interrupt_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN u32 IntMask
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Disable_Interrupt_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN u32 IntMask
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Clear_ISR_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN u32 IntMask
);

SDIO_ROM_TEXT_SECTION VOID SDIO_TxTask_Rom(
	IN VOID *pData
);

SDIO_ROM_TEXT_SECTION VOID SDIO_RxTask_Rom(
	IN VOID *pData
);

static VOID SDIO_SetEvent_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u32 Event
);

static VOID SDIO_ClearEvent_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u32 Event
);

static BOOL SDIO_IsEventPending_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u32 Event
);

SDIO_ROM_TEXT_SECTION VOID SDIO_IRQ_Handler_BH_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_RX_IRQ_Handler_BH_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

#if 0
VOID SDIO_TX_BD_Buf_Refill( 
	IN PHAL_SDIO_ADAPTER pSDIODev
);
#endif

SDIO_ROM_TEXT_SECTION VOID SDIO_TX_FIFO_DataReady_Rom( 
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION PSDIO_RX_PACKET SDIO_Alloc_Rx_Pkt_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Free_Rx_Pkt_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN PSDIO_RX_PACKET pPkt
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Recycle_Rx_BD_Rom (
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Process_H2C_IOMsg_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Send_C2H_IOMsg_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u32 *C2HMsg
);

SDIO_ROM_TEXT_SECTION u8 SDIO_Send_C2H_PktMsg_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u8 *C2HMsg, 
	IN u16 MsgLen
);

SDIO_ROM_TEXT_SECTION u8 SDIO_Process_RPWM_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION __weak u8 SDIO_Process_RPWM2_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Reset_Cmd_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Return_Rx_Data_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Rx_Data_Transaction_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN SDIO_RX_PACKET *pPkt
);

SDIO_ROM_TEXT_SECTION VOID SDIO_Register_Tx_Callback_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN s8 (*CallbackFun)(VOID *pAdapter, u8 *pPkt, u16 Offset, u16 PktSize),
	IN VOID *pAdapter	
);

SDIO_ROM_TEXT_SECTION s8 SDIO_Rx_Callback_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN VOID *pData,
	IN u16 Offset,
	IN u16 Length,
	IN u8 CmdType
);


SDIO_ROM_TEXT_SECTION u8 SDIO_ReadMem_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u8 *Addr, 
	IN u16 Len
);

SDIO_ROM_TEXT_SECTION u8 SDIO_WriteMem_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u8 *DesAddr,
	IN u8 *SrcAddr, 
	IN u16 Len,
	IN u8 Reply
);

SDIO_ROM_TEXT_SECTION u8 SDIO_SetMem_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u8 *DesAddr,
	IN u8 Data, 
	IN u16 Len,
	IN u8 Reply
);

/******************************************************************************
 * Global Variable Declaration
 ******************************************************************************/
#if 0
SDIO_ROM_BSS_SECTION ALIGNMTO(4) char SDIO_TXBD_Buf[(SDIO_TX_BD_NUM * sizeof(SDIO_TX_BD))];
SDIO_ROM_BSS_SECTION ALIGNMTO(4) char SDIO_TXBD_Hdl_Buf[SDIO_TX_BD_NUM * sizeof(SDIO_TX_BD_HANDLE)];
SDIO_ROM_BSS_SECTION ALIGNMTO(4) char SDIO_TX_Buf[SDIO_TX_BD_NUM][SDIO_TX_BD_BUF_USIZE*SDIO_TX_BUF_SZ_UNIT];

SDIO_ROM_BSS_SECTION ALIGNMTO(8) char SDIO_RXBD_Buf[(SDIO_RX_BD_NUM * sizeof(SDIO_RX_BD))];
SDIO_ROM_BSS_SECTION ALIGNMTO(4) char SDIO_RXBD_Hdl_Buf[SDIO_RX_BD_NUM * sizeof(SDIO_RX_BD_HANDLE)];
SDIO_ROM_BSS_SECTION ALIGNMTO(4) char SDIO_RXPkt_Hdl_Buf[SDIO_RX_PKT_NUM * sizeof(SDIO_RX_PACKET)];
#endif
#define SDIO_TIMEOUT_CNT    (1500*1000/TIMER_TICK_US)   // 1500ms

//SDIO_ROM_BSS_SECTION HAL_SDIO_ADAPTER SDIO_Boot_Dev;
//SDIO_ROM_BSS_SECTION HAL_SDIO_ADAPTER *pSDIO_Boot_Dev;
SDIO_ROM_BSS_SECTION unsigned int SDIO_CriticalNesting;
SDIO_ROM_BSS_SECTION unsigned int SDIO_TimeoutStart;
SDIO_ROM_BSS_SECTION ALIGNMTO(4) RAM_START_FUNCTION ImageEntryFun;


/******************************************************************************
 * External Function & Variable Declaration
 ******************************************************************************/
#if defined ( __ICCARM__ )
#pragma section=".sdio.buf.bss"
    
    SDIO_ROM_BSS_SECTION u8* __sdio_rom_bss_start__;
    SDIO_ROM_BSS_SECTION u8* __sdio_rom_bss_end__;
    
    SDIO_ROM_TEXT_SECTION __weak void __iar_sdio_section_addr_load(void)
    {
        __sdio_rom_bss_start__       = (u8*)__section_begin(".sdio.buf.bss");
        __sdio_rom_bss_end__         = (u8*)__section_end(".sdio.buf.bss");
    }

#else
extern u8 __sdio_rom_bss_start__[];
extern u8 __sdio_rom_bss_end__[];
#endif


SDIO_ROM_TEXT_SECTION __weak void SDIO_EnterCritical( 
    void )
{
    __disable_irq();
	SDIO_CriticalNesting++;
	__asm volatile( "dsb" );
	__asm volatile( "isb" );
	
	if (SDIO_CriticalNesting == 1) {
        if (__get_IPSR() != 0) {
            DBG_8195A("Warring!! Should Not Call EnterCritical() in an ISR\n");
    	}
   }
}
/*-----------------------------------------------------------*/

SDIO_ROM_TEXT_SECTION __weak void SDIO_ExitCritical( 
    void )
{
	if (SDIO_CriticalNesting == 0) {
        DBG_8195A("Warring!! Enter/Exit Critical Nesting doesn't match\n");
        return;
    }
	SDIO_CriticalNesting--;
	if( SDIO_CriticalNesting == 0 )
	{
        __enable_irq();
	}
}

/******************************************************************************
 * Function: SDIO_Device_Init
 * Desc: SDIO device driver initialization.
 *		1. Allocate SDIO TX FIFO buffer and initial TX related register.
 *		2. Allocate SDIO RX Buffer Descriptor and RX Buffer. Initial RX related
 *			register.
 *		3. Register the Interrupt function.
 *		4. Create the SDIO Task and allocate resource(Semaphore).
 *
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak BOOL SDIO_Device_Init_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	int i;
//	SDIO_TX_PACKET *pTxPkt;
	SDIO_RX_PACKET *pPkt;
	SDIO_TX_BD_HANDLE *pTxBdHdl;
	SDIO_RX_BD_HANDLE *pRxBdHdl;
//	int ret;
//	u32 reg;
	
	DBG_SDIO_INFO("SDIO_Device_Init==>\n");

	/* SDIO Function Enable */
    SDIOD_ON_FCTRL(ON);
    SDIOD_OFF_FCTRL(ON);

	/* Enable Clock for SDIO function */
    ACTCK_SDIOD_CCTRL(ON);

    // SDIO_SCLK / SPI_CLK pin pull-low
    GPIO_PullCtrl_8195a(_PA_3, DIN_PULL_LOW);

	// Reset SDIO DMA
	HAL_SDIO_WRITE8(REG_SPDIO_CPU_RST_DMA, BIT_CPU_RST_SDIO_DMA);
	
	/* Initial SDIO TX BD */
//	DBG_SDIO_INFO("Tx BD Init==>\n");	

// TODO: initial TX BD
	pSDIODev->pTXBDAddrAligned = (PSDIO_TX_BD)pvPortMalloc_ROM((SDIO_TX_BD_NUM * sizeof(SDIO_TX_BD)));;
    if (NULL == pSDIODev->pTXBDAddrAligned) {
		goto SDIO_INIT_ERR;
    }
    _memset(pSDIODev->pTXBDAddrAligned, 0, (SDIO_TX_BD_NUM * sizeof(SDIO_TX_BD)));

	HAL_SDIO_WRITE32(REG_SPDIO_TXBD_ADDR, pSDIODev->pTXBDAddrAligned);
	HAL_SDIO_WRITE16(REG_SPDIO_TXBD_SIZE, SDIO_TX_BD_NUM);

	/* Set TX_BUFF_UNIT_SIZE */
	HAL_SDIO_WRITE8(REG_SPDIO_TX_BUF_UNIT_SZ, SDIO_TX_BD_BUF_USIZE);

	DBG_SDIO_INFO("Tx BD Buf Unit Size(%d), Reg=0x%x\n", SDIO_TX_BD_BUF_USIZE, HAL_SDIO_READ8(REG_SPDIO_TX_BUF_UNIT_SZ));	
    
	/* Set DISPATCH_TXAGG_PKT */
	HAL_SDIO_WRITE32(REG_SPDIO_AHB_DMA_CTRL, HAL_SDIO_READ32(REG_SPDIO_AHB_DMA_CTRL)|BIT31);
    // Reset HW TX BD pointer
    pSDIODev->TXBDWPtr = HAL_SDIO_READ32(REG_SPDIO_TXBD_WPTR);
    pSDIODev->TXBDRPtr = pSDIODev->TXBDWPtr;
    pSDIODev->TXBDRPtrReg = pSDIODev->TXBDWPtr;
    HAL_SDIO_WRITE32(REG_SPDIO_TXBD_RPTR, pSDIODev->TXBDRPtrReg);

    pSDIODev->pTXBDHdl = (PSDIO_TX_BD_HANDLE)pvPortMalloc_ROM((SDIO_TX_BD_NUM * sizeof(SDIO_TX_BD_HANDLE)));
    if (NULL == pSDIODev->pTXBDHdl) {
		goto SDIO_INIT_ERR;
    }
    _memset((void*)pSDIODev->pTXBDHdl, 0, (SDIO_TX_BD_NUM * sizeof(SDIO_TX_BD_HANDLE)));

	for (i=0;i<SDIO_TX_BD_NUM;i++)
	{
		pTxBdHdl = pSDIODev->pTXBDHdl + i;
		pTxBdHdl->pTXBD = pSDIODev->pTXBDAddrAligned + i;
        // Allocate buffer for each TX BD
        pTxBdHdl->pTXBD->Address = (u32)pvPortMalloc_ROM((SDIO_TX_BD_BUF_USIZE*SDIO_TX_BUF_SZ_UNIT));
        if (NULL == pTxBdHdl->pTXBD->Address) {
            goto SDIO_INIT_ERR;
        }
		pTxBdHdl->isFree = 1;
//		DBG_SDIO_INFO("TX_BD%d @ 0x%x 0x%x\n", i, pTxBdHdl, pTxBdHdl->pTXBD);	
	}

	/* Init RX BD and RX Buffer */
    pSDIODev->pRXBDAddrAligned = (PSDIO_RX_BD)pvPortMalloc_ROM(SDIO_RX_BD_NUM * sizeof(SDIO_RX_BD)); // must 8-bytes aligned
    if (NULL == pSDIODev->pRXBDAddrAligned) {
        goto SDIO_INIT_ERR;
    }
    _memset((void*)pSDIODev->pRXBDAddrAligned, 0, (SDIO_RX_BD_NUM * sizeof(SDIO_RX_BD)));
	HAL_SDIO_WRITE32(REG_SPDIO_RXBD_ADDR, pSDIODev->pRXBDAddrAligned);
	HAL_SDIO_WRITE16(REG_SPDIO_RXBD_SIZE, SDIO_RX_BD_NUM);

    // Set the threshold of free RX BD count to trigger interrupt
	HAL_SDIO_WRITE16(REG_SPDIO_RX_BD_FREE_CNT, RX_BD_FREE_TH);
	DBG_SDIO_INFO("Rx BD Free Cnt(%d), Reg=0x%x\n", RX_BD_FREE_TH, HAL_SDIO_READ16(REG_SPDIO_RX_BD_FREE_CNT));	
    pSDIODev->pRXBDHdl = (PSDIO_RX_BD_HANDLE)pvPortMalloc_ROM(SDIO_RX_BD_NUM * sizeof(SDIO_RX_BD_HANDLE));
    if (NULL == pSDIODev->pRXBDHdl) {
        goto SDIO_INIT_ERR;
    }
    _memset((void*)pSDIODev->pRXBDHdl, 0, (SDIO_RX_BD_NUM * sizeof(SDIO_RX_BD_HANDLE)));

	for (i=0;i<SDIO_RX_BD_NUM;i++)
	{
		pRxBdHdl = pSDIODev->pRXBDHdl + i;
		pRxBdHdl->pRXBD = pSDIODev->pRXBDAddrAligned + i;
		pRxBdHdl->isFree = 1;
//		DBG_SDIO_INFO("RX_BD%d @ 0x%x 0x%x\n", i, pRxBdHdl, pRxBdHdl->pRXBD);	
	}

	RtlInitListhead(&pSDIODev->FreeRxPktList);	// Init the list for free packet handler
	/* Allocate memory for RX Packets handler */
    pSDIODev->pRxPktHandler = (SDIO_RX_PACKET *)pvPortMalloc_ROM(SDIO_RX_PKT_NUM * sizeof(SDIO_RX_PACKET));
    if (NULL == pSDIODev->pRxPktHandler) {
        goto SDIO_INIT_ERR;
    }
    _memset((void*)pSDIODev->pRxPktHandler, 0, (SDIO_RX_PKT_NUM * sizeof(SDIO_RX_PACKET)));

	/* Add all RX packet handler into the Free Queue(list) */
	for (i=0;i<SDIO_RX_PKT_NUM;i++)
	{
		pPkt = pSDIODev->pRxPktHandler + i;
		RtlListInsertTail(&pPkt->list, &pSDIODev->FreeRxPktList);
	}
	RtlInitListhead(&pSDIODev->RxPktList);	// Init the list for RX packet to be send to the SDIO bus
	
	/* enable the interrupt */
    SDIO_Interrupt_Init_Rom(pSDIODev);

    pSDIODev->CCPWM = 0;
	HAL_SDIO_WRITE8(REG_SPDIO_CCPWM, pSDIODev->CCPWM);

    // Update the CPWM register to indicate the host sdio init is done in image1
    pSDIODev->CCPWM2 = HAL_SDIO_READ16(REG_SPDIO_CCPWM2);
    pSDIODev->CCPWM2 |= SDIO_INIT_DONE;
    pSDIODev->CCPWM2 ^= CPWM2_TOGGLE_BIT;
    HAL_SDIO_WRITE16(REG_SPDIO_CCPWM2, pSDIODev->CCPWM2);
    
	/* Indicate the Host system that the TX/RX is ready */
//	HAL_SDIO_WRITE8(REG_SPDIO_CPU_IND, 0x01);

	DBG_SDIO_INFO("<==SDIO_Device_Init\n");

	return SUCCESS;
	
	SDIO_INIT_ERR:
	return FAIL;
}


/******************************************************************************
 * Function: SDIO_Device_DeInit
 * Desc: SDIO device driver free resource. This function should be called in 
 *			a task.
 *		1. Free TX FIFO buffer
 *
 * Para:
 *		pSDIODev: The SDIO device data structor.
 ******************************************************************************/
//TODO: Call this function in a task

SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Device_DeInit_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
//    int i=0;
//	SDIO_TX_BD_HANDLE *pTxBdHdl;
    
	if (NULL == pSDIODev)
		return;
#if 0
	if (pSDIODev->pRxPktHandler) {
		RtlMfree((u8*)pSDIODev->pRxPktHandler, sizeof(SDIO_RX_PACKET)*SDIO_RX_PKT_NUM);
		pSDIODev->pRxPktHandler = NULL;
	}
	
	if (pSDIODev->pRXBDHdl) {
		RtlMfree((u8 *)pSDIODev->pRXBDHdl, SDIO_RX_BD_NUM * sizeof(SDIO_RX_BD_HANDLE));
		pSDIODev->pRXBDHdl = NULL;
	}
	
	/* Free RX BD */
	if (pSDIODev->pRXBDAddr) {
		RtlMfree((u8 *)pSDIODev->pRXBDAddr, (SDIO_RX_BD_NUM * sizeof(SDIO_RX_BD))+7);
		pSDIODev->pRXBDAddr = NULL;
	}
	
	/* Free TX FIFO Buffer */
    for (i=0;i<SDIO_TX_BD_NUM;i++)
    {
        pTxBdHdl = pSDIODev->pTXBDHdl + i;
        if (pTxBdHdl->pTXBD->Address) {
            RtlMfree((u8 *)pTxBdHdl->pTXBD->Address, (SDIO_TX_BD_BUF_USIZE*SDIO_TX_BUF_SZ_UNIT));
            pTxBdHdl->pTXBD->Address = NULL;
        }
    }

    if (pSDIODev->pTXBDHdl) {
        RtlMfree((u8 *)pSDIODev->pTXBDHdl, (SDIO_TX_BD_NUM * sizeof(SDIO_TX_BD_HANDLE)));
        pSDIODev->pTXBDHdl = NULL;
    }

    if (pSDIODev->pTXBDAddr) {
        RtlMfree(pSDIODev->pTXBDAddr, ((SDIO_TX_BD_NUM * sizeof(SDIO_TX_BD))+3));
        pSDIODev->pTXBDAddr = NULL;
        pSDIODev->pTXBDAddrAligned = NULL;
    }
#endif    
    SDIO_Disable_Interrupt_Rom(pSDIODev, 0xffff);
    SDIO_Interrupt_DeInit_Rom(pSDIODev);

    // Reset SDIO DMA
    HAL_SDIO_WRITE8(REG_SPDIO_CPU_RST_DMA, BIT_CPU_RST_SDIO_DMA);

    /* Disable Clock for SDIO function */
//    ACTCK_SDIOD_CCTRL(OFF);

    /* SDIO Function Disable */
//    SDIOD_ON_FCTRL(OFF);
    SDIOD_OFF_FCTRL(OFF);
}

/******************************************************************************
 * Function: SDIO_TaskUp
 * Desc: For the Task scheduler no running case, use this function to run the
 *			SDIO task main loop.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_TaskUp_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	u16 ISRStatus;
	
//	DiagPrintf("SDIO_TaskUp==>\n");
	pSDIODev->EventSema++;
    if (pSDIODev->EventSema > 1000) {
        pSDIODev->EventSema = 1000;
    }
	if (pSDIODev->EventSema == 1) {
		while (pSDIODev->EventSema > 0) {
            ISRStatus = HAL_SDIO_READ16(REG_SPDIO_CPU_INT_STAS);
            pSDIODev->IntStatus |= ISRStatus;
            HAL_SDIO_WRITE16(REG_SPDIO_CPU_INT_STAS, ISRStatus);    // clean the ISR
            SDIO_SetEvent_Rom(pSDIODev, SDIO_EVENT_IRQ|SDIO_EVENT_C2H_DMA_DONE);

			SDIO_TxTask_Rom(pSDIODev);
            SDIO_RxTask_Rom(pSDIODev);
            
			pSDIODev->EventSema--;
		}
	}
//	DiagPrintf("<==SDIO_TaskUp\n");
}

/******************************************************************************
 * Function: SDIO_IRQ_Handler
 * Desc: SDIO device interrupt service routine
 *		1. Read & clean the interrupt status
 *		2. Wake up the SDIO task to handle the IRQ event
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_IRQ_Handler_Rom(
	IN VOID *pData
)
{
	PHAL_SDIO_ADAPTER pSDIODev = pData;
	u16 ISRStatus;
	
	ISRStatus = HAL_SDIO_READ16(REG_SPDIO_CPU_INT_STAS);
//    DBG_SDIO_INFO("%s:ISRStatus=0x%x\n", __FUNCTION__, ISRStatus);

	pSDIODev->IntStatus |= ISRStatus;
	HAL_SDIO_WRITE16(REG_SPDIO_CPU_INT_STAS, ISRStatus);	// clean the ISR

	SDIO_SetEvent_Rom(pSDIODev, SDIO_EVENT_IRQ|SDIO_EVENT_C2H_DMA_DONE);
//	SDIO_TaskUp_Rom(pSDIODev);
}

/******************************************************************************
 * Function: SDIO_Interrupt_Init
 * Desc: SDIO device interrupt initialization.
 *		1. Register the ISR
 *		2. Initial the IMR register
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Interrupt_Init_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	IRQ_HANDLE	SdioIrqHandle;

	pSDIODev->IntMask = BIT_H2C_DMA_OK | BIT_C2H_DMA_OK | BIT_H2C_MSG_INT | BIT_RPWM1_INT | \
						BIT_RPWM2_INT | BIT_H2C_BUS_RES_FAIL | BIT_RXBD_FLAG_ERR_INT;
	HAL_SDIO_WRITE16(REG_SPDIO_CPU_INT_STAS, pSDIODev->IntMask);	// Clean pending interrupt first
			
	SdioIrqHandle.Data = (u32) pSDIODev;
	SdioIrqHandle.IrqNum = SDIO_DEVICE_IRQ;
	SdioIrqHandle.IrqFun = (IRQ_FUN) SDIO_IRQ_Handler_Rom;
	SdioIrqHandle.Priority = SDIO_IRQ_PRIORITY;
			
	InterruptRegister(&SdioIrqHandle);
	InterruptEn(&SdioIrqHandle);

	HAL_SDIO_WRITE16(REG_SPDIO_CPU_INT_MASK, pSDIODev->IntMask);
}

/******************************************************************************
 * Function: SDIO_Interrupt_DeInit_Rom
 * Desc: SDIO device interrupt De-Initial.
 *		1. UnRegister the ISR
 *		2. Initial the IMR register with 0
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Interrupt_DeInit_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	IRQ_HANDLE	SdioIrqHandle;

	HAL_SDIO_WRITE16(REG_SPDIO_CPU_INT_STAS, 0xffff);	// Clean pending interrupt first
			
	SdioIrqHandle.Data = (u32) pSDIODev;
	SdioIrqHandle.IrqNum = SDIO_DEVICE_IRQ;
	SdioIrqHandle.IrqFun = (IRQ_FUN) SDIO_IRQ_Handler_Rom;
	SdioIrqHandle.Priority = SDIO_IRQ_PRIORITY;
			
    InterruptDis(&SdioIrqHandle);
	InterruptUnRegister(&SdioIrqHandle);
}


/******************************************************************************
 * Function: SDIO_Enable_Interrupt
 * Desc: SDIO enable interrupt by modify the interrupt mask 
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *	 IntMask: The bit map to enable the interrupt.
 ******************************************************************************/
__inline VOID SDIO_Enable_Interrupt_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN u32 IntMask
)
{
	SDIO_EnterCritical();
	pSDIODev->IntMask |= IntMask;
	HAL_SDIO_WRITE16(REG_SPDIO_CPU_INT_MASK, pSDIODev->IntMask);
	SDIO_ExitCritical();
}

/******************************************************************************
 * Function: SDIO_Disable_Interrupt
 * Desc: SDIO disable interrupt by modify the interrupt mask 
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *	 IntMask: The bit map to disable the interrupt.
 ******************************************************************************/
__inline VOID SDIO_Disable_Interrupt_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN u32 IntMask
)
{
	SDIO_EnterCritical();
	pSDIODev->IntMask &= ~IntMask;
	HAL_SDIO_WRITE16(REG_SPDIO_CPU_INT_MASK, pSDIODev->IntMask);
	SDIO_ExitCritical();
}

/******************************************************************************
 * Function: SDIO_Clear_ISR
 * Desc: SDIO clear ISR bit map.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *	 IntMask: The bit map to be clean.
 ******************************************************************************/
__inline VOID SDIO_Clear_ISR_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN u32 IntMask
)
{
	SDIO_EnterCritical();
	pSDIODev->IntStatus &= ~IntMask;
	SDIO_ExitCritical();
}

/******************************************************************************
 * Function: SDIO_TxTask
 * Desc: The SDIO task handler. This is the main function of the SDIO device 
 * 			driver.
 *		1. Handle interrupt events.
 *			* SDIO TX data ready
 *			* Error handling
 *		2. 
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_TxTask_Rom(
	IN VOID *pData
)
{
	PHAL_SDIO_ADAPTER pSDIODev = pData;

	if (SDIO_IsEventPending_Rom(pSDIODev, SDIO_EVENT_IRQ)) {
		SDIO_ClearEvent_Rom(pSDIODev, SDIO_EVENT_IRQ);
		SDIO_IRQ_Handler_BH_Rom(pSDIODev);
        SDIO_TimeoutStart = HalTimerOp.HalTimerReadCount(1);	}
#if 0
	if (SDIO_IsEventPending_Rom(pSDIODev, SDIO_EVENT_TXBD_REFILL)) {
		SDIO_ClearEvent_Rom(pSDIODev, SDIO_EVENT_TXBD_REFILL);
		SDIO_TX_BD_Buf_Refill(pSDIODev);
	}
#endif    

}

/******************************************************************************
 * Function: SDIO_RxTask
 * Desc: The SDIO RX task handler. This is the main function of the SDIO device 
 * 			driver to handle SDIO RX.
 *		1. Handle interrupt events.
 *			* SDIO RX done
 *		2. Send RX data back to the host by fill RX_BD to hardware. 
 *      3. Handle messages from mailbox
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_RxTask_Rom(
	IN VOID *pData
)
{
    PHAL_SDIO_ADAPTER pSDIODev = pData;
    
    if (SDIO_IsEventPending_Rom(pSDIODev, SDIO_EVENT_C2H_DMA_DONE)) {
        SDIO_ClearEvent_Rom(pSDIODev, SDIO_EVENT_C2H_DMA_DONE);
        SDIO_RX_IRQ_Handler_BH_Rom(pSDIODev);
    }
#if 0
    if (SDIO_IsEventPending_Rom(pSDIODev, SDIO_EVENT_RX_PKT_RDY)) {
        SDIO_ClearEvent_Rom(pSDIODev, SDIO_EVENT_RX_PKT_RDY);
        SDIO_Return_Rx_Data_Rom(pSDIODev);
    }
#endif    
}


/******************************************************************************
 * Function: SDIO_SetEvent
 * Desc: Set an event and wake up SDIO task to handle it.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *	 Event: The event to be set.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION static VOID SDIO_SetEvent_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u32 Event
)
{
    if (__get_IPSR() != 0) {
    	pSDIODev->Events |= Event;
    }
    else {
    	SDIO_EnterCritical();
    	pSDIODev->Events |= Event;
    	SDIO_ExitCritical();
    }
}

/******************************************************************************
 * Function: SDIO_ClearEvent
 * Desc: Clean a SDIO event.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *	 Event: The event to be cleaned.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION static VOID SDIO_ClearEvent_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u32 Event
)
{
    if (__get_IPSR() != 0) {
        pSDIODev->Events &= ~Event;
    }
    else {
        SDIO_EnterCritical();
        pSDIODev->Events &= ~Event;
        SDIO_ExitCritical();
    }
}

/******************************************************************************
 * Function: SDIO_IsEventPending
 * Desc: To check is a event pending.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *	 Event: The event to check.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION static BOOL SDIO_IsEventPending_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u32 Event
)
{
	BOOL ret;
	
    if (__get_IPSR() != 0) {
        ret = (pSDIODev->Events & Event) ? 1:0;
    }
    else {
        SDIO_EnterCritical();   
        ret = (pSDIODev->Events & Event) ? 1:0;
        SDIO_ExitCritical();
    }

	return ret;
}

/******************************************************************************
 * Function: SDIO_IRQ_Handler_BH
 * Desc: Process the SDIO IRQ, the button helf.
 *		1. SDIO TX data ready.
 *		2. H2C command ready.
 *		3. Host driver RPWM status updated.
 *		4. SDIO RX data transfer done.
 *		5. SDIO HW/BUS errors.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_IRQ_Handler_BH_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	u32 IntStatus;

    DBG_SDIO_INFO("%s @1 IntStatus=0x%x\n", __FUNCTION__, pSDIODev->IntStatus);

	SDIO_EnterCritical();
	IntStatus = pSDIODev->IntStatus;
	SDIO_ExitCritical();
	
	if (IntStatus & BIT_H2C_DMA_OK) {
		SDIO_Clear_ISR_Rom(pSDIODev, BIT_H2C_DMA_OK);
		SDIO_Disable_Interrupt_Rom(pSDIODev, BIT_H2C_DMA_OK);
		SDIO_TX_FIFO_DataReady_Rom(pSDIODev);
		SDIO_Enable_Interrupt_Rom(pSDIODev, BIT_H2C_DMA_OK);
	}

	if (IntStatus & BIT_H2C_MSG_INT) {
		SDIO_Clear_ISR_Rom(pSDIODev, BIT_H2C_MSG_INT);
		SDIO_Process_H2C_IOMsg_Rom(pSDIODev);
	}

	if (IntStatus & BIT_RPWM1_INT) {
		SDIO_Clear_ISR_Rom(pSDIODev, BIT_RPWM1_INT);
		SDIO_Process_RPWM_Rom(pSDIODev);
	}

	if (IntStatus & BIT_RPWM2_INT) {
		SDIO_Clear_ISR_Rom(pSDIODev, BIT_RPWM2_INT);
		SDIO_Process_RPWM2_Rom(pSDIODev);
	}
	
	if (IntStatus & BIT_SDIO_RST_CMD_INT) {
		SDIO_Clear_ISR_Rom(pSDIODev, BIT_SDIO_RST_CMD_INT);
		SDIO_Reset_Cmd_Rom(pSDIODev);
	}

//    DBG_SDIO_INFO("%s @2 IntStatus=0x%x\n", __FUNCTION__, pSDIODev->IntStatus);
}

/******************************************************************************
 * Function: SDIO_RX_IRQ_Handler_BH
 * Desc: Process the SDIO RX IRQ, the button helf.
 *		1. SDIO RX data transfer done.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_RX_IRQ_Handler_BH_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	u32 IntStatus;

	SDIO_EnterCritical();
	IntStatus = pSDIODev->IntStatus;
	SDIO_ExitCritical();
	
	if (IntStatus & BIT_C2H_DMA_OK) {
		SDIO_Clear_ISR_Rom(pSDIODev, BIT_C2H_DMA_OK);
		SDIO_Recycle_Rx_BD_Rom(pSDIODev);
	}
}

#if 0
/******************************************************************************
 * Function: SDIO_TX_BD_Buf_Refill
 * Desc: To refill all TX BD buffer.
 *		1. Check all TX BD buffer
 *		2. Allocate a new buffer for TX BD buffer is invalid
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
VOID SDIO_TX_BD_Buf_Refill( 
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
    u32 i,j;
    u32 wait_loop;
    PSDIO_TX_BD_HANDLE pTxBdHdl;
    #define WAIT_TIMEOUT    100

    for (i=0;i<SDIO_TX_BD_NUM;i++) {
		pTxBdHdl = pSDIODev->pTXBDHdl + pSDIODev->TXBDRPtrReg;
        if (NULL == pTxBdHdl->pTXBD->Address) {
            for (j=0;j<WAIT_TIMEOUT;j++) {
                pTxBdHdl->pTXBD->Address = (u32)RtlMalloc(SDIO_TX_BD_BUF_USIZE*SDIO_TX_BUF_SZ_UNIT);
                if (NULL == pTxBdHdl->pTXBD->Address) {
                    DBG_SDIO_WARN("%s Alloc Mem(size=%d) Failed\n", __FUNCTION__, SDIO_TX_BD_BUF_USIZE*SDIO_TX_BUF_SZ_UNIT);   
                    RtlMsleepOS(20);                    
                }
                else {
                    pSDIODev->MemAllocCnt++;
                    pSDIODev->TXBDRPtrReg++;
                    if (pSDIODev->TXBDRPtrReg >= SDIO_TX_BD_NUM) {
                        pSDIODev->TXBDRPtrReg = 0;
                    }
                    HAL_SDIO_WRITE16(REG_SPDIO_TXBD_RPTR, pSDIODev->TXBDRPtrReg);
                    break;  // break the for loop
                }
            }
            if (j == WAIT_TIMEOUT) {
                break;  // break the for loop
            }
        }
        else {
            break;  // break the for loop
        }        
    }

    if (pSDIODev->TXBDRPtrReg != pSDIODev->TXBDRPtr) {
        DBG_SDIO_ERR("SDIO_TX_BD_Buf_Refill Err: TXBDRPtrReg=%d TXBDRPtr=%d\n", pSDIODev->TXBDRPtrReg, pSDIODev->TXBDRPtr);   
    }
}
#endif

/******************************************************************************
 * Function: SDIO_TX_Pkt_Handle_Rom
 * Desc: To handle a SDIO TX packet local. It could be works for firmware download. 
 *		1. Parse the TX Desc and then takes the correspond handle.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 * 	 pPkt: The start of packet, includes TX Desc.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_TX_Pkt_Handle_Rom( 
	IN PHAL_SDIO_ADAPTER pSDIODev,
    IN u8 *pPkt
)
{
    union {
    	PSDIO_TX_DESC pTxDesc;
        PSDIO_TX_DESC_MR pTxDescMR;
        PSDIO_TX_DESC_MW pTxDescMW;
        PSDIO_TX_DESC_MS pTxDescMS;
        PSDIO_TX_DESC_JS pTxDescJS;
    } pDesc;
    u8 CmdType;

    pDesc.pTxDesc = (PSDIO_TX_DESC)(pPkt);
    CmdType = pDesc.pTxDesc->type;

    DBG_SDIO_INFO("%s: CmdType=0x%x\n", __FUNCTION__, CmdType);

    switch (CmdType) {
        case SDIO_CMD_MEMRD:
            DBG_SDIO_INFO("Mem Read @ 0x%x, len=%d\n", 
                pDesc.pTxDescMR->start_addr, pDesc.pTxDescMR->read_len);
            SDIO_ReadMem_Rom(pSDIODev, \
                (u8*)pDesc.pTxDescMR->start_addr, \
                pDesc.pTxDescMR->read_len);
            break;

        case SDIO_CMD_MEMWR:
            DBG_SDIO_INFO("Mem Write @ 0x%x, len=%d, reply=%d\n", 
                pDesc.pTxDescMW->start_addr, pDesc.pTxDescMW->write_len, pDesc.pTxDescMW->reply);
            SDIO_WriteMem_Rom(pSDIODev, 
                (u8*)pDesc.pTxDescMW->start_addr, 
                (u8*)(pPkt+pDesc.pTxDescMW->offset), 
                pDesc.pTxDescMW->write_len, 
                pDesc.pTxDescMW->reply);
            break;

        case SDIO_CMD_MEMST:
            DBG_SDIO_INFO("Mem Set @ 0x%x, len=%d, reply=%d\n", 
                pDesc.pTxDescMS->start_addr, pDesc.pTxDescMS->write_len, pDesc.pTxDescMS->reply);
            SDIO_SetMem_Rom(pSDIODev, 
                (u8*)pDesc.pTxDescMS->start_addr, 
                pDesc.pTxDescMS->data, 
                pDesc.pTxDescMS->write_len, 
                pDesc.pTxDescMS->reply);
            break;

        case SDIO_CMD_STARTUP:
            DBG_SDIO_INFO("Jump to Entry Func @ 0x%x\n", 
                pDesc.pTxDescJS->start_fun);

            /* Indicate the Host system that the TX/RX is NOT ready */
            HAL_SDIO_WRITE8(REG_SPDIO_CPU_IND, 0x00);            
            ImageEntryFun.RamStartFun = (void(*)(void))pDesc.pTxDescJS->start_fun;
            break;
    }

}

/******************************************************************************
 * Function: SDIO_TX_FIFO_DataReady
 * Desc: Handle the SDIO FIFO data ready interrupt.
 *		1. Send those data to the target driver via callback fun., like WLan.
 *		2. Allocate a buffer for the TX BD
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_TX_FIFO_DataReady_Rom( 
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
    PSDIO_TX_BD_HANDLE pTxBdHdl;
	PSDIO_TX_DESC pTxDesc;
	volatile u16 TxBDWPtr=0;
	u32 processed_pkt_cnt=0;


//	DBG_SDIO_INFO("SDIO_TX_FIFO_DataReady==>\n");	

	TxBDWPtr = HAL_SDIO_READ16(REG_SPDIO_TXBD_WPTR);
	if (TxBDWPtr == pSDIODev->TXBDRPtr) {
		DBG_SDIO_WARN("SDIO TX Data Read False Triggered!!, TXBDWPtr=0x%x\n", TxBDWPtr);	
		return;
	}
	
	do {
		DBG_SDIO_INFO("SDIO_TX_DataReady: TxBDWPtr=%d TxBDRPtr=%d\n", TxBDWPtr, pSDIODev->TXBDRPtr);
		pTxBdHdl = pSDIODev->pTXBDHdl + pSDIODev->TXBDRPtr;
        pTxDesc = (PSDIO_TX_DESC)(pTxBdHdl->pTXBD->Address);

        DBG_SDIO_INFO("SDIO_TX_DataReady: PktSz=%d Offset=%d\n", pTxDesc->txpktsize, pTxDesc->offset);
        if ((pTxDesc->txpktsize + pTxDesc->offset) > (SDIO_TX_BD_BUF_USIZE*SDIO_TX_BUF_SZ_UNIT)) {
            DBG_SDIO_WARN("SDIO_TX_DataReady Err: Incorrect TxDesc, PktSz=%d Offset=%d BufSize=%d\n", pTxDesc->txpktsize, pTxDesc->offset, \
                (SDIO_TX_BD_BUF_USIZE*SDIO_TX_BUF_SZ_UNIT));
        }
        SDIO_TX_Pkt_Handle_Rom(pSDIODev, (u8*)pTxBdHdl->pTXBD->Address);
        processed_pkt_cnt++;

        pSDIODev->TXBDRPtr++;
        if (pSDIODev->TXBDRPtr >= SDIO_TX_BD_NUM) {
            pSDIODev->TXBDRPtr = 0;
        }
        
        pSDIODev->TXBDRPtrReg = pSDIODev->TXBDRPtr;
        HAL_SDIO_WRITE16(REG_SPDIO_TXBD_RPTR, pSDIODev->TXBDRPtrReg);

		TxBDWPtr = HAL_SDIO_READ16(REG_SPDIO_TXBD_WPTR);
	} while (pSDIODev->TXBDRPtr != TxBDWPtr);
}

/******************************************************************************
 * Function: SDIO_Alloc_Rx_Pkt
 * Desc: Allocate a RX Packet Handle from the queue.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *
 *	Return:
 *		The allocated RX packet handler.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak PSDIO_RX_PACKET SDIO_Alloc_Rx_Pkt_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	_LIST *plist;
	SDIO_RX_PACKET *pPkt;
	
    SDIO_EnterCritical();
	if (RtlIsListEmpty(&pSDIODev->FreeRxPktList)) {
        SDIO_ExitCritical();
        DBG_SDIO_ERR("Error! No Free RX PKT\n");
        return NULL;
	}
	
	plist = RtlListGetNext(&pSDIODev->FreeRxPktList);	
	pPkt = CONTAINER_OF(plist, SDIO_RX_PACKET, list);
	
	RtlListDelete(&pPkt->list);
    SDIO_ExitCritical();
	return pPkt;
}

/******************************************************************************
 * Function: SDIO_Free_Rx_Pkt
 * Desc: Put a RX Packet Handle back to the queue.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 * 	 pPkt: The packet handler to be free.
 *
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Free_Rx_Pkt_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN PSDIO_RX_PACKET pPkt
)
{
    SDIO_EnterCritical();
	RtlListInsertTail(&pPkt->list, &pSDIODev->FreeRxPktList);
    SDIO_ExitCritical();
}

/******************************************************************************
 * Function: SDIO_Recycle_Rx_BD
 * Desc: To recycle some RX BD when C2H RX DMA done.
 *	1. Free the RX packet.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Recycle_Rx_BD_Rom (
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	SDIO_RX_BD_HANDLE *pRxBdHdl;
	SDIO_RX_BD *pRXBD;
	u32 FreeCnt=0;		// for debugging

	DBG_SDIO_INFO("SDIO_Recycle_Rx_BD==> %d %d\n", HAL_SDIO_READ16(REG_SPDIO_RXBD_C2H_RPTR), pSDIODev->RXBDRPtr);
	SDIO_Disable_Interrupt_Rom(pSDIODev, BIT_C2H_DMA_OK);
	while (HAL_SDIO_READ16(REG_SPDIO_RXBD_C2H_RPTR) != pSDIODev->RXBDRPtr)
	{
		pRxBdHdl = pSDIODev->pRXBDHdl + pSDIODev->RXBDRPtr;
		pRXBD = pSDIODev->pRXBDAddrAligned + pSDIODev->RXBDRPtr;
		if (!pRxBdHdl->isFree) {
			if (pRxBdHdl->isPktEnd && (NULL != pRxBdHdl->pPkt)) {
				/* Free this packet */
				// TODO: Free SDIO RX packet, but we may need to just use static memory allocation for ROM code
				// TODO: implement SDIO RX static buffer

				_memset((void *)&(pRxBdHdl->pPkt->PktBuf), 0, sizeof(SDIO_RX_DESC));
                SDIO_EnterCritical();
				RtlListInsertTail(&pRxBdHdl->pPkt->list, &pSDIODev->FreeRxPktList);	// Put packet handle to free queue
				SDIO_ExitCritical();
				FreeCnt++;
				pRxBdHdl->isPktEnd = 0;
				pRxBdHdl->pPkt = NULL;
				DBG_SDIO_INFO("SDIO_Recycle_Rx_BD: Recycle Pkt, RXBDRPtr=%d\n", pSDIODev->RXBDRPtr);
			}
			_memset((void *)pRXBD , 0, sizeof(SDIO_RX_BD));	// clean this RX_BD
			pRxBdHdl->isFree = 1;
		}
		else {
			DBG_SDIO_WARN("SDIO_Recycle_Rx_BD: Warring, Recycle a Free RX_BD,RXBDRPtr=%d\n",pSDIODev->RXBDRPtr);
		}
		pSDIODev->RXBDRPtr++;
		if (pSDIODev->RXBDRPtr >= SDIO_RX_BD_NUM) {
			pSDIODev->RXBDRPtr -= SDIO_RX_BD_NUM;
		}
	}
	SDIO_Enable_Interrupt_Rom(pSDIODev, BIT_C2H_DMA_OK);
	DBG_SDIO_INFO("<==SDIO_Recycle_Rx_BD(%d)\n", FreeCnt);

}

/******************************************************************************
 * Function: SDIO_Process_H2C_IOMsg
 * Desc: Handle the interrupt for HC2 message ready. Read the H2C_MSG register
 *		and process the H2C message.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Process_H2C_IOMsg_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{	
	u32 H2CMsg;

	// TODO: define H2C message type & format, currently we have  30 bits message only, may needs to extend the HW register
	H2CMsg = HAL_SDIO_READ32(REG_SPDIO_CPU_H2C_MSG);
	DBG_SDIO_INFO("H2C_MSG: 0x%x\n", H2CMsg);
	// TODO: May needs to handle endian free
	switch (H2CMsg)
	{
	default:
		break;
	}
	// TODO: Some H2C message needs to be fordward to WLan driver
}

/******************************************************************************
 * Function: SDIO_Send_C2H_IOMsg
 * Desc: Send C2H message to the Host.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Send_C2H_IOMsg_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u32 *C2HMsg
)
{
	u32 TmpC2HMsg;

	// TODO: define C2H message type & format, currently we have  30 bits message only, may needs to extend the HW register
	
	// TODO: May needs to handle endian free
	TmpC2HMsg = HAL_SDIO_READ32(REG_SPDIO_CPU_C2H_MSG);
	TmpC2HMsg = ((TmpC2HMsg ^ (u32)BIT(31)) & (u32)BIT(31)) | *C2HMsg;
	HAL_SDIO_WRITE32(REG_SPDIO_CPU_C2H_MSG, TmpC2HMsg);
}

/******************************************************************************
 * Function: SDIO_Send_C2H_PktMsg
 * Desc: To send a C2H message to the Host through the block read command.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *	 H2CMsg: point to the buffer of the H2C message received.
 *	 MsgLen: The length of this message.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak u8 SDIO_Send_C2H_PktMsg_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u8 *C2HMsg, 
	IN u16 MsgLen
)
{
	u8 *MsgBuf;
    PSDIO_RX_DESC pRxDesc;
    SDIO_RX_PACKET *pPkt;
	
	DBG_SDIO_INFO("C2H_MSG: 0x%x\n", *C2HMsg);

    pPkt = SDIO_Alloc_Rx_Pkt_Rom(pSDIODev);
    if (pPkt == NULL) {
        DBG_SDIO_ERR("SDIO_Send_C2H_PktMsg: Err!! No Free RX PKT!\n");
        return FAIL;
    }
    
    MsgBuf = &pPkt->PktBuf[sizeof(SDIO_RX_DESC)];
    if (MsgLen > (SDIO_RX_BD_BUF_SIZE - sizeof(SDIO_RX_DESC))) {
		DBG_SDIO_ERR("SDIO_Send_C2H_PktMsg: Msg Length(%d) over the preserved buffer size\n", MsgLen);
        MsgLen = (SDIO_RX_BD_BUF_SIZE - sizeof(SDIO_RX_DESC));
    }
	_memcpy((void *)(MsgBuf), (void *)C2HMsg, MsgLen);

    pRxDesc = (PSDIO_RX_DESC)(&pPkt->PktBuf[0]);
    pRxDesc->pkt_len = MsgLen;
    pRxDesc->offset = sizeof(SDIO_RX_DESC);
    pRxDesc->type = SDIO_CMD_C2H;
    pPkt->pData = NULL;     // payload in self buffer
    pPkt->Offset = 0;
#if 0    
    SDIO_EnterCritical();
    RtlListInsertTail(&pPkt->list, &pSDIODev->RxPktList);
    pSDIODev->RxInQCnt++;
    SDIO_ExitCritical();
    SDIO_SetEvent_Rom(pSDIODev, SDIO_EVENT_RX_PKT_RDY);
    SDIO_TaskUp(pSDIODev);
#else
    SDIO_Rx_Data_Transaction_Rom(pSDIODev, pPkt);
#endif
    return SUCCESS;
    
}

/******************************************************************************
 * Function: SDIO_Process_RPWM
 * Desc: To handle RPWM interrupt.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak u8 SDIO_Process_RPWM_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	u8 rpwm;

	rpwm = HAL_SDIO_READ8(REG_SPDIO_CRPWM);

	DBG_SDIO_INFO ("RPWM1: 0x%x\n", rpwm);
	// TODO: forward this RPWM message to WLan

	return 0;
}

/******************************************************************************
 * Function: SDIO_Process_RPWM
 * Desc: To handle RPWM interrupt.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak u8 SDIO_Process_RPWM2_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	u16 rpwm2;

	rpwm2 = HAL_SDIO_READ16(REG_SPDIO_CRPWM2);

	DBG_SDIO_INFO ("RPWM1: 0x%x\n", rpwm2);

	return 0;
}

/******************************************************************************
 * Function: SDIO_Reset_Cmd
 * Desc: Handle the SDIO Reset Command interrupt. We did nothing currently. 
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Reset_Cmd_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	// TODO:
	DBG_SDIO_INFO ("SDIO_Reset_Cmd_Rom: To Do...\n");
	return;
}

#if 0
/******************************************************************************
 * Function: SDIO_Return_Rx_Data
 * Desc: To send all packets in the RX packet list to the Host system via the 
 *		SDIO bus.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Return_Rx_Data_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev
)
{
	SDIO_RX_PACKET *pPkt=NULL;
	SDIO_RX_DESC *pRxDesc;
	SDIO_RX_BD_HANDLE *pRxBdHdl;
	_LIST *plist;
	SDIO_RX_BD *pRXBD;
	u32 Offset=0;
	u16 i;
	u16 RxBdWrite=0;	// to count how much RX_BD used in a Transaction
	u16 RxBdRdPtr=0;	// RX_BD read pointer
    u16 min_rx_bd_num;
	u32 pkt_size;
	u8 isForceBreak=0;
    u8 isListEmpty;

	DBG_SDIO_INFO("SDIO_Return_Rx_Data==> RXBDWPtr=%d\n", pSDIODev->RXBDWPtr);
    SDIO_EnterCritical();

	if (RtlIsListEmpty(&pSDIODev->RxPktList)) {
		DBG_SDIO_INFO("SDIO_Return_Rx_Data: Queue is empty\n");
        SDIO_ExitCritical();
		return;
	}
    SDIO_ExitCritical();

	RxBdRdPtr = pSDIODev->RXBDRPtr;
	do {
		/* Check if we shoule handle the RX_BD recycle ? */
		if (RxBdRdPtr != HAL_SDIO_READ16(REG_SPDIO_RXBD_C2H_RPTR)) {
			SDIO_Recycle_Rx_BD_Rom(pSDIODev);
			RxBdRdPtr = pSDIODev->RXBDRPtr;
		}

		/* check if RX_BD available */
        SDIO_EnterCritical();
		plist = RtlListGetNext(&pSDIODev->RxPktList);	
        SDIO_ExitCritical();
		pPkt = CONTAINER_OF(plist, SDIO_RX_PACKET, list);
        if (pPkt->pData != NULL) {
            // RX_Desc and the payload data is separated, needs 2 RX_BD to transmit this packet
            min_rx_bd_num = MIN_RX_BD_SEND_PKT;
        }
        else {
            // RX_Desc and the payload data is encapsuled together, only need 1 RX_BD
            min_rx_bd_num = 1;
        }
        
		if (RxBdRdPtr != pSDIODev->RXBDWPtr) {
			if (pSDIODev->RXBDWPtr > RxBdRdPtr) {
				if ((pSDIODev->RXBDWPtr - RxBdRdPtr) == (SDIO_RX_BD_NUM - min_rx_bd_num)) 
				{
					DBG_SDIO_WARN("SDIO_Return_Rx_Data: No Available RX_BD, ReadPtr=%d WritePtr=%d\n", \
						RxBdRdPtr, pSDIODev->RXBDWPtr);
					isForceBreak = 1;
					break;	// break the while loop
				}
			} 
			else {
				if ((RxBdRdPtr - pSDIODev->RXBDWPtr) == min_rx_bd_num)
				{
					DBG_SDIO_WARN("SDIO_Return_Rx_Data: No Available RX_BD, ReadPtr=%d WritePtr=%d\n", RxBdRdPtr, pSDIODev->RXBDWPtr);
					isForceBreak = 1;
					break;	// break the while loop
				}
			}
		}

        SDIO_EnterCritical();
		RtlListDelete(&pPkt->list);	// remove it from the SDIO RX packet Queue
		pSDIODev->RxInQCnt--;
        SDIO_ExitCritical();

		// TODO: Add RX_DESC before the packet

		/* a SDIO RX packet will use at least 2 RX_BD, the 1st one is for RX_Desc, 
		   other RX_BDs are for packet payload */
		/* Use a RX_BD to transmit RX_Desc */		
		pRxDesc = (SDIO_RX_DESC*)&(pPkt->PktBuf[0]);
		pRXBD = pSDIODev->pRXBDAddrAligned + pSDIODev->RXBDWPtr;	// get the RX_BD head
		pRxBdHdl = pSDIODev->pRXBDHdl + pSDIODev->RXBDWPtr;
		if (!pRxBdHdl->isFree) {
			DBG_SDIO_ERR("SDIO_Return_Rx_Data: Allocated a non-free RX_BD\n");
		}
		pRxBdHdl->isFree = 0;
		pRxBdHdl->pPkt = pPkt;
		pRXBD->FS = 1;
		pRXBD->PhyAddr = (u32)((u8 *)pRxDesc);
        if (pPkt->pData != NULL) {
            // RX_Desc and the payload data is separated, needs 2 RX_BD to transmit this packet
            pRXBD->BuffSize = sizeof(SDIO_RX_DESC);
            pRxBdHdl->isPktEnd = 0;
        }
        else {
            // RX_Desc and the payload data is encapsuled together, only need 1 RX_BD
            pRXBD->BuffSize = sizeof(SDIO_RX_DESC) + pRxDesc->pkt_len;
            pRxBdHdl->isPktEnd = 1;
        }

		pSDIODev->RXBDWPtr += 1;
		if (pSDIODev->RXBDWPtr >= SDIO_RX_BD_NUM) {
			pSDIODev->RXBDWPtr -= SDIO_RX_BD_NUM;
		}

        if (pPkt->pData != NULL) {
    		/* Take RX_BD to transmit packet payload */
    		pkt_size = pRxDesc->pkt_len;

			pRXBD = pSDIODev->pRXBDAddrAligned + pSDIODev->RXBDWPtr;	// get the RX_BD head
			pRxBdHdl = pSDIODev->pRXBDHdl + pSDIODev->RXBDWPtr;
			pRxBdHdl->isFree = 0;
			pRxBdHdl->pPkt = pPkt;
			pRXBD->FS = 0;
			pRXBD->PhyAddr = (u32)(((u8 *)pPkt->pData)+pPkt->Offset);
			if (pkt_size > MAX_RX_BD_BUF_SIZE)	{
				// if come to here, please enable "SDIO_RX_PKT_SIZE_OVER_16K"
				DBG_SDIO_ERR("SDIO_Return_Rx_Data: The Packet Size bigger than 16K\n");
				pkt_size = MAX_RX_BD_BUF_SIZE;
			}
			pRXBD->BuffSize = pkt_size;
			pRxBdHdl->isPktEnd = 1;
			// Move the RX_BD Write pointer forward
			pSDIODev->RXBDWPtr += 1;
			if (pSDIODev->RXBDWPtr >= SDIO_RX_BD_NUM) {
				pSDIODev->RXBDWPtr -= SDIO_RX_BD_NUM;
			}
        }

        pRXBD->LS = 1;
        RxBdWrite++;
        HAL_SDIO_WRITE16(REG_SPDIO_RXBD_C2H_WPTR, pSDIODev->RXBDWPtr);
        DBG_SDIO_INFO("SDIO_Return_Rx_Data:RXBDWPtr=%d\n", pSDIODev->RXBDWPtr);

        SDIO_EnterCritical();
        isListEmpty = RtlIsListEmpty(&pSDIODev->RxPktList);
        SDIO_ExitCritical();
	} while(!isListEmpty);

	if (RxBdWrite > 0) {
		HAL_SDIO_WRITE8(REG_SPDIO_HCI_RX_REQ, BIT_HCI_RX_REQ);
	}

	if (isForceBreak) {
		// function end with insufficient resource, set event to try again later
		SDIO_SetEvent_Rom(pSDIODev, SDIO_EVENT_RX_PKT_RDY);
		SDIO_TaskUp_Rom(pSDIODev);
	}
	DBG_SDIO_INFO("SDIO_Return_Rx_Data(%d)<==\n", RxBdWrite);
	
}
#endif

/******************************************************************************
 * Function: SDIO_Rx_Data_Transaction_Rom
 * Desc: To send a packets to the Host system via the 
 *		SDIO bus.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Rx_Data_Transaction_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN SDIO_RX_PACKET *pPkt
)
{
	SDIO_RX_DESC *pRxDesc;
	SDIO_RX_BD_HANDLE *pRxBdHdl;
	SDIO_RX_BD *pRXBD;
	u16 RxBdRdPtr=0;	// RX_BD read pointer
//    u16 needed_rx_bd_bum;
	u32 pkt_size;
//	u8 isForceBreak=0;
//    u8 isListEmpty;

	DBG_SDIO_INFO("SDIO_Rx_Data_Transaction_Rom==> RXBDWPtr=%d\n", pSDIODev->RXBDWPtr);

	RxBdRdPtr = pSDIODev->RXBDRPtr;

	/* Check if we shoule handle the RX_BD recycle ? */
	if (RxBdRdPtr != HAL_SDIO_READ16(REG_SPDIO_RXBD_C2H_RPTR)) {
		SDIO_Recycle_Rx_BD_Rom(pSDIODev);
		RxBdRdPtr = pSDIODev->RXBDRPtr;
	}

	/* a SDIO RX packet will use at least 2 RX_BD, the 1st one is for RX_Desc, 
	   other RX_BDs are for packet payload */
	/* Use a RX_BD to transmit RX_Desc */		
	pRxDesc = (SDIO_RX_DESC*)&(pPkt->PktBuf[0]);
	pRXBD = pSDIODev->pRXBDAddrAligned + pSDIODev->RXBDWPtr;	// get the RX_BD head
	pRxBdHdl = pSDIODev->pRXBDHdl + pSDIODev->RXBDWPtr;
	if (!pRxBdHdl->isFree) {
		DBG_SDIO_ERR("SDIO_Rx_Data_Transaction_Rom: Allocated a non-free RX_BD\n");
		DBG_SDIO_ERR("RXBDRPtr=%d RXBDWPtr=%d\n", RxBdRdPtr, pSDIODev->RXBDWPtr);
	}
	pRxBdHdl->isFree = 0;
	pRxBdHdl->pPkt = pPkt;
	pRXBD->FS = 1;
	pRXBD->PhyAddr = (u32)((u8 *)pRxDesc);
    if (pPkt->pData != NULL) {
        // RX_Desc and the payload data is separated, needs 2 RX_BD to transmit this packet
        pRXBD->BuffSize = sizeof(SDIO_RX_DESC);
        pRxBdHdl->isPktEnd = 0;
    }
    else {
        // RX_Desc and the payload data is encapsuled together, only need 1 RX_BD
        pRXBD->BuffSize = sizeof(SDIO_RX_DESC) + pRxDesc->pkt_len;
        pRxBdHdl->isPktEnd = 1;
    }
    
	pSDIODev->RXBDWPtr += 1;
	if (pSDIODev->RXBDWPtr >= SDIO_RX_BD_NUM) {
		pSDIODev->RXBDWPtr -= SDIO_RX_BD_NUM;
	}

    if (pPkt->pData != NULL) {
		/* Take RX_BD to transmit packet payload */
		pkt_size = pRxDesc->pkt_len;
		pRXBD = pSDIODev->pRXBDAddrAligned + pSDIODev->RXBDWPtr;	// get the RX_BD head
		pRxBdHdl = pSDIODev->pRXBDHdl + pSDIODev->RXBDWPtr;
        if (!pRxBdHdl->isFree) {
            DBG_SDIO_ERR("SDIO_Rx_Data_Transaction_Rom: Allocated a non-free RX_BD\n");
            DBG_SDIO_ERR("RXBDRPtr=%d RXBDWPtr=%d\n", RxBdRdPtr, pSDIODev->RXBDWPtr);
        }
		pRxBdHdl->isFree = 0;
		pRxBdHdl->pPkt = pPkt;
		pRXBD->FS = 0;
		pRXBD->PhyAddr = (u32)(((u8 *)pPkt->pData)+pPkt->Offset);
		if (pkt_size > MAX_RX_BD_BUF_SIZE)	{
			// if come to here, please enable "SDIO_RX_PKT_SIZE_OVER_16K"
			DBG_SDIO_ERR("SDIO_Rx_Data_Transaction_Rom: The Packet Size bigger than 16K\n");
			pkt_size = MAX_RX_BD_BUF_SIZE;
		}
		pRXBD->BuffSize = pkt_size;
		pRxBdHdl->isPktEnd = 1;
		// Move the RX_BD Write pointer forward
		pSDIODev->RXBDWPtr += 1;
		if (pSDIODev->RXBDWPtr >= SDIO_RX_BD_NUM) {
			pSDIODev->RXBDWPtr -= SDIO_RX_BD_NUM;
		}
    }

    pRXBD->LS = 1;
    HAL_SDIO_WRITE16(REG_SPDIO_RXBD_C2H_WPTR, pSDIODev->RXBDWPtr);
    DBG_SDIO_INFO("SDIO_Rx_Data_Transaction_Rom:RXBDWPtr=%d\n", pSDIODev->RXBDWPtr);

	HAL_SDIO_WRITE8(REG_SPDIO_HCI_RX_REQ, BIT_HCI_RX_REQ);
}

/******************************************************************************
 * Function: SDIO_Register_Tx_Callback
 * Desc: For a TX data target driver to register its TX callback function, so 
 *		the SDIO driver can use it to fordward a TX packet to the target driver.
 *
 * Para:
 *	pSDIODev: point to the SDIO device handler
 * 	CallbackFun: The function pointer of the callback
 *	pAdapter: a pointer will be use to call the registered CallBack function.
 *				It can be used to point to a handler of the caller, like WLan
 *				Adapter.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Register_Tx_Callback_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN s8 (*CallbackFun)(VOID *pAdapter, u8 *pPkt, u16 Offset, u16 PktSize),
	IN VOID *pAdapter
)
{
	pSDIODev->Tx_Callback = CallbackFun;
	pSDIODev->pTxCb_Adapter = pAdapter;
}

#if 0
/******************************************************************************
 * Function: SDIO_Rx_Callback
 * Desc: The callback function for an packet receiving, which be called from 
 *		the Target (WLan) driver to send a packet to the SDIO host.
 *
 * Para:
 *  pSDIODev: Point to the SDIO device data structer.
 *	pData: Point to the head of the data to be return to the Host system.
 *	Length: The length of the data to be send, in byte.
 *	
 * Return:
 * 	 The result, SUCCESS or FAIL.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak s8 SDIO_Rx_Callback_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev,
	IN VOID *pData,
	IN u16 Offset,
	IN u16 Length,
	IN u8 CmdType
)
{
	PSDIO_RX_DESC pRxDesc;
	SDIO_RX_PACKET *pPkt;

	pPkt = SDIO_Alloc_Rx_Pkt_Rom(pSDIODev);
	if (pPkt == NULL) {
		DBG_SDIO_ERR("RX Callback Err!! No Free RX PKT!\n");
		return FAIL;
	}
	pRxDesc = (PSDIO_RX_DESC)&pPkt->PktBuf[0];
	pRxDesc->pkt_len = Length;
	pRxDesc->offset = sizeof(SDIO_RX_DESC);
    pRxDesc->type = CmdType;
	pPkt->pData = pData;
	pPkt->Offset = Offset;
    SDIO_EnterCritical();
	RtlListInsertTail(&pPkt->list, &pSDIODev->RxPktList);
	pSDIODev->RxInQCnt++;
    SDIO_ExitCritical();
	SDIO_SetEvent_Rom(pSDIODev, SDIO_EVENT_RX_PKT_RDY);
	SDIO_TaskUp_Rom(pSDIODev);

	return SUCCESS;
}
#endif

/******************************************************************************
 * Function: SDIO_ReadMem_Rom
 * Desc: To response the Memory Read command from the Host side by transmit a 
 *       packet with the memory content.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *	 Addr: the memory read start address
 *	 Len: The length of data to read.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak u8 SDIO_ReadMem_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u8 *Addr, 
	IN u16 Len
)
{
    PSDIO_RX_DESC_MR pRxDesc;
    SDIO_RX_PACKET *pPkt;
	
	DBG_SDIO_INFO("SDIO_ReadMem_Rom: Addr=0x%x, Len=%d\n", (u32)Addr, Len);

    pSDIODev->CCPWM2 &= ~SDIO_MEM_RD_DONE;
	HAL_SDIO_WRITE16(REG_SPDIO_CCPWM2, pSDIODev->CCPWM2);    
    
    pPkt = SDIO_Alloc_Rx_Pkt_Rom(pSDIODev);
    if (pPkt == NULL) {
        DBG_SDIO_ERR("SDIO_ReadMem_Rom: Err!! No Free RX PKT!\n");
        return FAIL;
    }
    
    pRxDesc = (PSDIO_RX_DESC_MR)(&pPkt->PktBuf[0]);
    pRxDesc->pkt_len = Len;
    pRxDesc->offset = sizeof(SDIO_RX_DESC);
    pRxDesc->type = SDIO_CMD_MEMRD_RSP;
    pRxDesc->start_addr = (u32)Addr;
    pPkt->pData = Addr;
    pPkt->Offset = 0;
    SDIO_Rx_Data_Transaction_Rom(pSDIODev, pPkt);

    // Update the CPWM2 register to indicate the host command is done
    pSDIODev->CCPWM2 |= SDIO_MEM_RD_DONE;
    pSDIODev->CCPWM2 ^= SDIO_CPWM2_TOGGLE;
	HAL_SDIO_WRITE16(REG_SPDIO_CCPWM2, pSDIODev->CCPWM2);    

    return SUCCESS;
    
}

/******************************************************************************
 * Function: SDIO_WriteMem_Rom
 * Desc: To handle a memory write command from Host side.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *	 DesAddr: the memory write destenation address
 *	 SrcAddr: the memory write source address.
 *	 Len: The length of data to written.
 *	 Reply: Is need to send a packet to response the memory write command.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak u8 SDIO_WriteMem_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u8 *DesAddr,
	IN u8 *SrcAddr, 
	IN u16 Len,
	IN u8 Reply
)
{
    pSDIODev->CCPWM2 &= ~SDIO_MEM_WR_DONE;
	HAL_SDIO_WRITE16(REG_SPDIO_CCPWM2, pSDIODev->CCPWM2);    
    _memcpy(DesAddr, SrcAddr, Len);

    // Update the CPWM2 register to indicate the host command is done
    pSDIODev->CCPWM2 |= SDIO_MEM_WR_DONE;
    pSDIODev->CCPWM2 ^= SDIO_CPWM2_TOGGLE;
	HAL_SDIO_WRITE16(REG_SPDIO_CCPWM2, pSDIODev->CCPWM2);    
    
    if (Reply) {
        PSDIO_RX_DESC_MW pRxDesc;
        SDIO_RX_PACKET *pPkt;
        
        pPkt = SDIO_Alloc_Rx_Pkt_Rom(pSDIODev);
        if (pPkt == NULL) {
            DBG_SDIO_ERR("SDIO_WriteMem_Rom: Err!! No Free RX PKT!\n");
            return FAIL;
        }
        
        pRxDesc = (PSDIO_RX_DESC_MW)(&pPkt->PktBuf[0]);
        pRxDesc->pkt_len = 0;
        pRxDesc->offset = sizeof(SDIO_RX_DESC);
        pRxDesc->type = SDIO_CMD_MEMWR_RSP;
        pRxDesc->start_addr = (u32)DesAddr;
        pRxDesc->write_len = Len;
        pRxDesc->result = 0;
        pPkt->pData = NULL;     // payload in self buffer
        pPkt->Offset = 0;
        SDIO_Rx_Data_Transaction_Rom(pSDIODev, pPkt);
        
    }
    return SUCCESS;
}

/******************************************************************************
 * Function: SDIO_SetMem_Rom
 * Desc: To handle a memory set command from Host side.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 *	 DesAddr: the memory set destenation address
 *	 Data: the value to be write to the memory.
 *	 Len: The length of data to written.
 *	 Reply: Is need to send a packet to response the memory set command.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak u8 SDIO_SetMem_Rom(
	IN PHAL_SDIO_ADAPTER pSDIODev, 
	IN u8 *DesAddr,
	IN u8 Data, 
	IN u16 Len,
	IN u8 Reply
)
{
    pSDIODev->CCPWM2 &= ~SDIO_MEM_ST_DONE;
	HAL_SDIO_WRITE16(REG_SPDIO_CCPWM2, pSDIODev->CCPWM2);    

    _memset(DesAddr, Data, Len);

    // Update the CPWM2 register to indicate the host command is done
    pSDIODev->CCPWM2 |= SDIO_MEM_ST_DONE;
    pSDIODev->CCPWM2 ^= SDIO_CPWM2_TOGGLE;
	HAL_SDIO_WRITE16(REG_SPDIO_CCPWM2, pSDIODev->CCPWM2);    
    
    if (Reply) {
        PSDIO_RX_DESC_MS pRxDesc;
        SDIO_RX_PACKET *pPkt;
        
        pPkt = SDIO_Alloc_Rx_Pkt_Rom(pSDIODev);
        if (pPkt == NULL) {
            DBG_SDIO_ERR("SDIO_SetMem_Rom: Err!! No Free RX PKT!\n");
            return FAIL;
        }
        
        pRxDesc = (PSDIO_RX_DESC_MS)(&pPkt->PktBuf[0]);
        pRxDesc->pkt_len = 0;
        pRxDesc->offset = sizeof(SDIO_RX_DESC);
        pRxDesc->type = SDIO_CMD_MEMST_RSP;
        pRxDesc->start_addr = (u32)DesAddr;
        pRxDesc->write_len = Len;
        pRxDesc->result = 0;
        pPkt->pData = NULL;     // payload in self buffer
        pPkt->Offset = 0;
        SDIO_Rx_Data_Transaction_Rom(pSDIODev, pPkt);        
    }
    return SUCCESS;
}

#if 0
VOID
SYSIrqHandle_Rom (
    IN  VOID        *Data
)
{
    u32 Rtemp;

    //change cpu clk
    HalDelayUs(100);

    Rtemp = (HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_FUNC_EN) | 0x40000000);
    HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_FUNC_EN, Rtemp);
    
    //disable DSTBY timer
    HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_ANA_TIM_CTRL, 0);

    //clear wake event IMR
    HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_SLP_WAKE_EVENT_MSK0, 0);

    //clear wake event ISR
    Rtemp = HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_SLP_WAKE_EVENT_STATUS0);
    HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_SLP_WAKE_EVENT_STATUS0, Rtemp); 

}

VOID
InitSYSIRQ_ROM(VOID)
{
    IRQ_HANDLE          SysHandle;
    
    SysHandle.Data       = (u32) (NULL);
    SysHandle.IrqNum     = SYSTEM_ON_IRQ;
    SysHandle.IrqFun     = (IRQ_FUN) SYSIrqHandle_Rom;
    SysHandle.Priority   = 0;
     
    InterruptRegister(&SysHandle);
    InterruptEn(&SysHandle);
}

VOID
DeInitSYSIRQ_ROM(VOID)
{
    IRQ_HANDLE          SysHandle;
    
    SysHandle.Data       = (u32) (NULL);
    SysHandle.IrqNum     = SYSTEM_ON_IRQ;
    SysHandle.Priority   = 0;
     
    InterruptDis(&SysHandle);
    InterruptUnRegister(&SysHandle);
}
#endif
/******************************************************************************
 * Function: SDIOSlpPG
 * Desc: To make the system to enetr Sleep PG mode
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/


SDIO_ROM_TEXT_SECTION VOID
SDIOSlpPG(
    VOID
)
{
    u32 Rtemp = 0;

    //Backup CPU CLK
//    BackupCPUClk();
    HAL_WRITE32(0x60008000, 0x80006180, 0xffffffff);

    
    //Clear event

    HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_SLP_WAKE_EVENT_STATUS0, HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_SLP_WAKE_EVENT_STATUS0));

    //3 2 Configure power state option:       
    // 2.1 power mode option: 
    HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_PWRMGT_OPTION, 0x74000100);

    // 2.2  sleep power mode option1 
    Rtemp = ((HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_PWRMGT_OPTION_EXT) & 0xffffff00)|0x2);
    HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_PWRMGT_OPTION_EXT, Rtemp);

    while(1) {

        HalDelayUs(100);
        
        if (HAL_READ8(LOG_UART_REG_BASE,0x14)&BIT6){
            
            break;
        }
    }

    //Set Event
    HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_SLP_WAKE_EVENT_MSK0, BIT14);

    //3 Enable low power mode
    //Enable low power mode:       
    Rtemp = 0x00000004;
    HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_PWRMGT_CTRL, Rtemp);
   
    //3 Wait CHIP enter low power mode
    // Wait deep standby timer timeout
    // Wait CHIP resume to norm power mode
    HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_PWRMGT_CTRL);
    __WFI();
}

/******************************************************************************
 * Function: SDIO_PowerSave_Rom
 * Desc: To enter the power saving mode
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/
SDIO_ROM_TEXT_SECTION __weak VOID SDIO_PowerSave_Rom(
    IN PHAL_SDIO_ADAPTER pSDIODev    
)
{
    // request to enter sleep mode
    pSDIODev->CCPWM2 = HAL_SDIO_READ16(REG_SPDIO_CCPWM2);
    pSDIODev->CCPWM2 &= ~(CPWM2_ACT_BIT);

    // enter power gated state
    pSDIODev->CCPWM2 &= ~(CPWM2_DSTANDBY_BIT);
        
    pSDIODev->CCPWM2 ^= CPWM2_TOGGLE_BIT;
    HAL_SDIO_WRITE16(REG_SPDIO_CCPWM2, pSDIODev->CCPWM2);

    SDIO_Device_DeInit_Rom(pSDIODev);
    SDIOSlpPG();

    while (1);

}

/******************************************************************************
 * Function: SDIO_Boot_Up
 * Desc: The main loop for SDIO boot up flow.
 *
 * Para:
 * 	 pSDIODev: The SDIO device data structor.
 ******************************************************************************/

SDIO_ROM_TEXT_SECTION __weak VOID SDIO_Boot_Up_Simu(VOID)
{
    u32 BssLen;
    HAL_SDIO_ADAPTER *pSDIO_Boot_Dev;

#if defined ( __ICCARM__ )   
    __iar_sdio_section_addr_load();
#endif 

    DBG_8195A("%s==>0\r\n", __FUNCTION__);

    // Switch CPU Clock => High
    HalCpuClkConfig(CLK_200M);
#ifdef CONFIG_TIMER_MODULE
    HalDelayUs(1000);
#endif
//    InitSYSIRQ_ROM();

#ifdef CONFIG_SDIO_BOOT_SIM
        IRQ_HANDLE          UartIrqHandle;
        LOG_UART_ADAPTER    UartAdapter;
        
        //4 Release log uart reset and clock
        LOC_UART_FCTRL(ON);
        ACTCK_LOG_UART_CCTRL(ON);
        
        PinCtrl(LOG_UART,S0,ON);
        
        //4 Register Log Uart Callback function
        UartIrqHandle.Data = (u32)NULL;//(u32)&UartAdapter;
        UartIrqHandle.IrqNum = UART_LOG_IRQ;
        UartIrqHandle.IrqFun = (IRQ_FUN) UartLogIrqHandle;
        UartIrqHandle.Priority = 0;
        
        //4 Inital Log uart
        UartAdapter.BaudRate = UART_BAUD_RATE_38400;
        UartAdapter.DataLength = UART_DATA_LEN_8BIT;
        UartAdapter.FIFOControl = 0xC1;
        UartAdapter.IntEnReg = 0x00;
        UartAdapter.Parity = UART_PARITY_DISABLE;
        UartAdapter.Stop = UART_STOP_1BIT;
        
        //4 Initial Log Uart
        HalLogUartInit(UartAdapter);
        
        //4 Register Isr handle
        InterruptUnRegister(&UartIrqHandle); 
        InterruptRegister(&UartIrqHandle); 
        
        UartAdapter.IntEnReg = 0x05;
        
        //4 Initial Log Uart for Interrupt
        HalLogUartInit(UartAdapter);
        
        //4 initial uart log parameters before any uartlog operation
        RtlConsolInit(ROM_STAGE,GetRomCmdNum(),(VOID*)&UartLogRomCmdTable);// executing boot seq., 
#else
    //    HalInitPlatformLogUart();
        HalReInitPlatformLogUartV02();
#endif

    DBG_8195A("%s==>1\r\n", __FUNCTION__);
    // Initial BSS
    BssLen = (__sdio_rom_bss_end__ - __sdio_rom_bss_start__);
	DBG_SDIO_INFO("bss_start=0x%x bss_end=0x%x len=0x%x\n", __sdio_rom_bss_start__, __sdio_rom_bss_end__, BssLen);
    _memset((void *) __sdio_rom_bss_start__, 0, BssLen);
#if 1
    // Initial SDR
    if ( HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_SYSTEM_CFG1) & BIT_SYSCFG_ALDN_STS ) {

        if ( HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EEPROM_CTRL0) & BIT_SYS_AUTOLOAD_SUS ) {
            
            if ( HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_SYSCFG6) & BIT14 ) {
                
	            SdrControllerInit_rom(&SdrDramInfo_rom);
//	            SdrControllerInit(&SdrDramInfo);
            }
        }
    }
#endif    
    ImageEntryFun.RamStartFun = NULL;
    pSDIO_Boot_Dev = (HAL_SDIO_ADAPTER*)pvPortMalloc_ROM(sizeof(HAL_SDIO_ADAPTER));
    if (NULL == pSDIO_Boot_Dev) {
		DBG_SDIO_ERR("SDIO_Boot_Up: MemAlloc Err\r\n");
        return;
    }
    _memset((void*)pSDIO_Boot_Dev, 0, sizeof(HAL_SDIO_ADAPTER));
    DBG_8195A("%s==>2\r\n", __FUNCTION__);

    SDIO_Device_Init_Rom (pSDIO_Boot_Dev);
    SDIO_TimeoutStart = HalTimerOp.HalTimerReadCount(1);
    DBG_8195A("%s==>3\r\n", __FUNCTION__);
	while (1) {
		SDIO_TxTask_Rom(pSDIO_Boot_Dev);
        SDIO_RxTask_Rom(pSDIO_Boot_Dev);
        if (ImageEntryFun.RamStartFun != NULL) {
            break;
        }

        if ((pUartLogCtl->ExecuteCmd) == _TRUE) {
            UartLogCmdExecute((PUART_LOG_CTL)pUartLogCtl);
            CONSOLE_8195A();
            pUartLogCtl->ExecuteCmd = _FALSE;
            SDIO_TimeoutStart = HalTimerOp.HalTimerReadCount(1);
        }

        if (HAL_TIMEOUT == SDIO_IsTimeout(SDIO_TimeoutStart, SDIO_TIMEOUT_CNT)) {
            DBG_8195A("SDIO_PwrSav==>\r\n");
            SDIO_PowerSave_Rom(pSDIO_Boot_Dev);
        }
	}
//    DeInitSYSIRQ_ROM();

    SDIO_Device_DeInit_Rom(pSDIO_Boot_Dev);
    ImageEntryFun.RamStartFun();
}

#undef E_CUT_ROM_DOMAIN
