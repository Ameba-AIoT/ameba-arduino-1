/*******************************************************************************
 * Copyright (c) 2014, Realtek Semiconductor Corp.
 * All rights reserved.
 *
 * This module is a confidential and proprietary property of RealTek and
 * possession or use of this module requires written permission of RealTek.
 *******************************************************************************
 */

#include "xmport_uart.h"
#include "rtl8195a.h"
#include "xmodem.h"

#if CONFIG_UART_EN

#include "pinmap.h"

extern XMODEM_COM_PORT XComPort;
FWU_DATA_SECTION HAL_RUART_ADAPTER *pxmodem_uart_adp;

_LONG_CALL_ extern VOID HalRuartAdapterLoadDefRtl8195a(VOID *pAdp, u8 UartIdx);
_LONG_CALL_ extern HAL_Status HalRuartInitRtl8195a(VOID *Data);
_LONG_CALL_ extern VOID HalRuartDeInitRtl8195a(VOID *Data);

#define XM_UART_RX_BUF_SZ       1032
FWU_DATA_SECTION u8 XmodemUartRxBuf[XM_UART_RX_BUF_SZ];
FWU_DATA_SECTION u32 volatile XmRxBuf_In;
FWU_DATA_SECTION u32 volatile XmRxBuf_Out;
FWU_DATA_SECTION u32 volatile XmRxBuf_Len;

FWU_TEXT_SECTION void
XmRxBufRst(void)
{
    XmRxBuf_In = 0;
    XmRxBuf_Out = 0;
    XmRxBuf_Len = 0;
}

FWU_TEXT_SECTION u8
XmRxBufPoll(void)
{
	u32 rxbufin = XmRxBuf_In;
	u32 rxbufout = XmRxBuf_Out;
    if (rxbufin != rxbufout) {
        return 1;
    }
    else {
        return 0;
    }
}

FWU_TEXT_SECTION void
XmRxBufPut(char RxData)
{
    XmodemUartRxBuf[XmRxBuf_In] = RxData;
    XmRxBuf_In++;
    if (XmRxBuf_In == XM_UART_RX_BUF_SZ) {
        XmRxBuf_In = 0;
    }
}

FWU_TEXT_SECTION u8
_XmRxBufGet(void)
{
    u8 c;

    c = XmodemUartRxBuf[XmRxBuf_Out];
    XmRxBuf_Out++;
    if (XmRxBuf_Out == XM_UART_RX_BUF_SZ) {
        XmRxBuf_Out = 0;
    }
    return c;
}

FWU_TEXT_SECTION u8
XmRxBufGet(char *pBuf, u32 timeout)
{
	u32 rxbufin = XmRxBuf_In;
    do {
        if (rxbufin != XmRxBuf_Out) {
            *(pBuf) = XmodemUartRxBuf[XmRxBuf_Out];
            XmRxBuf_Out++;
            if (XmRxBuf_Out == XM_UART_RX_BUF_SZ) {
                XmRxBuf_Out = 0;
            }
            return 0;
        } else {
            timeout--;
            HalDelayUs(1);
        }
    } while (timeout > 0);

    return 1;
}

FWU_TEXT_SECTION u8
XmRxBufGetS(char *pBuf, u32 len, u32 timeout)
{
    u32 i=0;
    u32 rxbufin = XmRxBuf_In;
    while ((i<len) && (timeout > 0)) {
        if (rxbufin != XmRxBuf_Out) {
            *(pBuf+i) = XmodemUartRxBuf[XmRxBuf_Out];
            XmRxBuf_Out++;
            i++;
            if (XmRxBuf_Out == XM_UART_RX_BUF_SZ) {
                XmRxBuf_Out = 0;
            }            
        } else {
            timeout--;
            HalDelayUs(1);
        }
    }

    if (i==len) {
        return 0;
    } else {
        return 1;
    }
}

FWU_TEXT_SECTION u32
XmodemUartIrqHandle(
        IN VOID *Data
)
{
    volatile u8 reg_iir;
    u8 IntId;
    volatile u32  RegValue;
    u32 UartIndex;
    PHAL_RUART_ADAPTER pHalRuartAdapter = (PHAL_RUART_ADAPTER) Data;
    int i;

    UartIndex = pHalRuartAdapter->UartIndex;

    for (i=0;i<5;i++) {
        // Maximum process 5 different interrupt events
        reg_iir = HAL_RUART_READ32(UartIndex, RUART_INT_ID_REG_OFF);
//        DBG_UART_INFO("_UartIrqHandle: UartIdx=%d IIR=0x%x IMR=0x%x\n", UartIndex, reg_iir, HAL_RUART_READ32(UartIndex, RUART_INTERRUPT_EN_REG_OFF));
        if ((reg_iir & RUART_IIR_INT_PEND) != 0) {
            // No pending IRQ
            break;
        }

        IntId = (reg_iir & RUART_IIR_INT_ID) >> 1;

        switch (IntId) {
            case ModemStatus:
                pHalRuartAdapter->ModemStatus =  HAL_RUART_READ32(UartIndex, RUART_MODEM_STATUS_REG_OFF);
                break;

            case TxFifoEmpty:
                break;

            case ReceiverDataAvailable:
            case TimeoutIndication:
//                DBG_8195A("%s==>\r\n", __FUNCTION__);
                while (1) {
                    RegValue = (HAL_RUART_READ32(UartIndex, RUART_LINE_STATUS_REG_OFF));                
                    if (RegValue & RUART_LINE_STATUS_REG_DR) {
                        XmRxBufPut(HAL_RUART_READ32(UartIndex, RUART_REV_BUF_REG_OFF)&0xff);
                    }
                    else {
                        break;
                    }
                }

                break;

            case ReceivLineStatus:
                RegValue = (HAL_RUART_READ32(UartIndex, RUART_LINE_STATUS_REG_OFF));
                pHalRuartAdapter->Status |= RegValue & RUART_LINE_STATUS_ERR;
                break;

            default:
                DBG_UART_ERR(ANSI_COLOR_RED"Unknown Interrupt Type\n"ANSI_COLOR_RESET);
                break;
        }
    }

    return 0;
}

FWU_TEXT_SECTION
void xmodem_uart_init(u8 uart_idx, u8 pin_mux, u32 baud_rate)
{
    PHAL_RUART_ADAPTER pHalRuartAdapter = pxmodem_uart_adp;

    HalRuartAdapterLoadDefRtl8195a(pHalRuartAdapter, uart_idx);
    pHalRuartAdapter->PinmuxSelect = pin_mux;
    pHalRuartAdapter->BaudRate = baud_rate;
//    pHalRuartAdapter->FifoControl = 0xC9;   // FIFO enable, DMA Enable
    pHalRuartAdapter->DmaEnable = 0;
    pHalRuartAdapter->Parity = RUART_PARITY_DISABLE;
    pHalRuartAdapter->StopBit = RUART_STOP_BIT_1;
    pHalRuartAdapter->FlowControl = AUTOFLOW_DISABLE;
    pHalRuartAdapter->IrqHandle.IrqFun = NULL;
    
    HalRuartInitRtl8195a(pHalRuartAdapter);
//    HalRuartInit(pHalRuartAdapter);
    XmRxBufRst();

    pHalRuartAdapter->IrqHandle.IrqFun = (IRQ_FUN)XmodemUartIrqHandle;
    HalRuartRegIrqRtl8195a(pHalRuartAdapter);    
    HalRuartIntEnableRtl8195a(pHalRuartAdapter);

    pHalRuartAdapter->Interrupts |= RUART_IER_ERBI | RUART_IER_ELSI;
    HalRuartSetIMRRtl8195a (pHalRuartAdapter);
}

FWU_TEXT_SECTION
void xmodem_uart_func_hook(XMODEM_COM_PORT *pXComPort)
{
//    pXComPort->poll = xmodem_uart_readable;
    pXComPort->poll = (char(*)(void))XmRxBufPoll;
    pXComPort->put = xmodem_uart_putc;
//    pXComPort->get = xmodem_uart_getc;
    pXComPort->get = (char(*)(void))_XmRxBufGet;
}

FWU_TEXT_SECTION
void xmodem_uart_deinit(void) 
{
    PHAL_RUART_ADAPTER pHalRuartAdapter = pxmodem_uart_adp;

    HalRuartDeInitRtl8195a(pHalRuartAdapter);
}


/******************************************************************************
 * READ/WRITE
 ******************************************************************************/
FWU_TEXT_SECTION
char xmodem_uart_readable(void) 
{
    PHAL_RUART_ADAPTER pHalRuartAdapter = pxmodem_uart_adp;
    u8  uart_idx = pHalRuartAdapter->UartIndex;

    if ((HAL_RUART_READ32(uart_idx, RUART_LINE_STATUS_REG_OFF)) & RUART_LINE_STATUS_REG_DR) {
        return 1;
    }
    else {
        return 0;
    }
}

FWU_TEXT_SECTION
char xmodem_uart_writable(void) 
{
    PHAL_RUART_ADAPTER pHalRuartAdapter = pxmodem_uart_adp;
    u8  uart_idx = pHalRuartAdapter->UartIndex;

    if (HAL_RUART_READ32(uart_idx, RUART_LINE_STATUS_REG_OFF) & 
                        (RUART_LINE_STATUS_REG_THRE)) {
       return 1;
    }
    else {
        return 0;
    }
}

FWU_TEXT_SECTION
char xmodem_uart_getc(void) 
{
    PHAL_RUART_ADAPTER pHalRuartAdapter = pxmodem_uart_adp;
    u8  uart_idx = pHalRuartAdapter->UartIndex;

//    while (!xmodem_uart_readable());
    return (int)((HAL_RUART_READ32(uart_idx, RUART_REV_BUF_REG_OFF)) & 0xFF);
}

FWU_TEXT_SECTION
void xmodem_uart_putc(char c) 
{
    PHAL_RUART_ADAPTER pHalRuartAdapter = pxmodem_uart_adp;
    u8  uart_idx = pHalRuartAdapter->UartIndex;
    
    while (!xmodem_uart_writable());
    HAL_RUART_WRITE32(uart_idx, RUART_TRAN_HOLD_REG_OFF, (c & 0xFF));
}

#endif
