/* mbed Microcontroller Library
 *******************************************************************************
 * Copyright (c) 2014, Realtek Semiconductor Corp.
 * All rights reserved.
 *
 * This module is a confidential and proprietary property of RealTek and
 * possession or use of this module requires written permission of RealTek.
 *******************************************************************************
 */

#include "xmport_loguart.h"
#include "rtl8195a.h"
#include "xmodem.h"
#include "rtl_consol.h"
#include "hal_log_uart.h"

extern XMODEM_COM_PORT XComPort;
extern u32 ConfigDebugErr;
extern u32 ConfigDebugWarn;
extern u32 ConfigDebugInfo;
extern u32 CfgSysDebugErr;
extern u32 CfgSysDebugInfo;
extern u32 CfgSysDebugWarn;

FWU_DATA_SECTION u32 ComIrqBkUp;
FWU_DATA_SECTION u32 ConfigDebugErrBkUp;
FWU_DATA_SECTION u32 ConfigDebugWarnBkUp;
FWU_DATA_SECTION u32 ConfigDebugInfoBkUp;
FWU_DATA_SECTION u32 CfgSysDebugErrBkUp;
FWU_DATA_SECTION u32 CfgSysDebugInfoBkUp;
FWU_DATA_SECTION u32 CfgSysDebugWarnBkUp;

FWU_DATA_SECTION HAL_LOG_UART_ADAPTER XMLogUartAdapter;

FWU_TEXT_SECTION
void xmodem_loguart_putc(char c) 
{
	u32 CounterIndex = 0;

    while(1) {
        if (HAL_UART_READ8(UART_LINE_STATUS_REG_OFF) & 0x60) {
        	break;
        }
        
        HalDelayUs(100);
		CounterIndex++;
        if (CounterIndex >= 10000) {
        	break;
        }
	}
    HAL_UART_WRITE8(UART_TRAN_HOLD_OFF, c);  		  
}

#if 0
FWU_TEXT_SECTION
char xmodem_loguart_getc(void) 
{
    return HAL_UART_READ8(UART_REV_BUF_OFF);
}

FWU_TEXT_SECTION
char xmodem_loguart_readable(void) 
{
    return (HAL_UART_READ8(UART_LINE_STATUS_REG_OFF) & BIT0);
}

FWU_TEXT_SECTION
void xmodem_loguart_init(void) 
{
    ComIrqBkUp = HAL_UART_READ32(UART_INTERRUPT_EN_REG_OFF);
    HAL_UART_WRITE32(UART_INTERRUPT_EN_REG_OFF, 0); // disable all IRQ

    // turn off all debug message
    ConfigDebugErrBkUp = ConfigDebugErr;
    ConfigDebugWarnBkUp = ConfigDebugWarn;
    ConfigDebugInfoBkUp = ConfigDebugInfo;
    CfgSysDebugErrBkUp = CfgSysDebugErr;
    CfgSysDebugInfoBkUp = CfgSysDebugInfo;
    CfgSysDebugWarnBkUp = CfgSysDebugWarn;

    ConfigDebugErr = 0;
    ConfigDebugWarn = 0;
    ConfigDebugInfo = 0;
    CfgSysDebugErr = 0;
    CfgSysDebugInfo = 0;
    CfgSysDebugWarn = 0;    
}

FWU_TEXT_SECTION
void xmodem_loguart_func_hook(XMODEM_COM_PORT *pXComPort)
{
    pXComPort->poll = xmodem_loguart_readable;
    pXComPort->put = xmodem_loguart_putc;
    pXComPort->get = xmodem_loguart_getc;
}

#else
extern void XmRxBufRst(void);
extern u8 XmRxBufPoll(void);
extern void XmRxBufPut(char RxData);
extern u8 _XmRxBufGet(void);

FWU_TEXT_SECTION
static VOID XMLogUartIrqLineStatusHandle(HAL_LOG_UART_ADAPTER *pUartAdapter)
{
    u8 line_status;

    line_status = HAL_UART_READ8(UART_LINE_STATUS_REG_OFF);
    pUartAdapter->LineStatus = line_status;
}

FWU_TEXT_SECTION
static VOID XMLogUartIrqRxRdyHandle(HAL_LOG_UART_ADAPTER *pUartAdapter)
{
    volatile u8 line_status;
    
    while (1) {
        line_status = HAL_UART_READ8(UART_LINE_STATUS_REG_OFF);
        if (line_status & LSR_DR) {
            XmRxBufPut(HAL_UART_READ8(UART_REV_BUF_OFF));
        } else {
            break;
        }
    }
}

FWU_TEXT_SECTION
VOID XMLogUartIrqHandle(VOID * Data)
{
    HAL_LOG_UART_ADAPTER *pUartAdapter=Data;
    volatile u32 iir;
    u32 i;

    for (i=0;i<7;i++) {
        iir = HAL_UART_READ32(UART_INTERRUPT_IDEN_REG_OFF) & 0x0F;
        switch (iir) {
            case IIR_RX_LINE_STATUS:
                // Overrun/parity/framing errors or break interrupt
                XMLogUartIrqLineStatusHandle (pUartAdapter);
                break;
                
            case IIR_RX_RDY:
                // Receiver data available
                XMLogUartIrqRxRdyHandle (pUartAdapter);
                break;

            case IIR_CHAR_TIMEOUT:
                // No characters in or out of the RCVR FIFO during the
                // last 4 character times and there is at least 1
                // character in it during this time
                // TODO:
                XMLogUartIrqRxRdyHandle (pUartAdapter);
                break;

            case IIR_THR_EMPTY:
                // Transmitter holding register empty
//                HalLogUartIrqTxEmptyHandle(pUartAdapter);
                break;

            case IIR_MODEM_STATUS:
                // Clear to send or data set ready or 
                // ring indicator or data carrier detect.
                // TODO:
                break;

            case IIR_BUSY:
                // master has tried to write to the Line
                // Control Register while the DW_apb_uart is busy
                // TODO:
                break;

            case IIR_NO_PENDING:
                return;

            default:
                DBG_UART_WARN("HalLogUartIrqHandle: UnKnown Interrupt ID\n");
                break;
        }
    }
}


FWU_TEXT_SECTION
void xmodem_loguart_init(u32 BaudRate) 
{
    u32 SetData;
    u32 Divisor;
    u32 Dlh;
    u32 Dll;
    u32 SysClock;
    u32 SampleRate,Remaind;
    PIRQ_HANDLE pIrqHandle;
    HAL_LOG_UART_ADAPTER *pUartAdapter;

    pUartAdapter = &XMLogUartAdapter;

    pUartAdapter->BaudRate = BaudRate;
    pUartAdapter->Parity = LCR_PARITY_NONE;   // No parity
    pUartAdapter->Stop = LCR_STOP_1B;     // 1 stop bit
    pUartAdapter->DataLength = 8 - 5;   // 8 bit
    pUartAdapter->FIFOControl = FCR_FIFO_EN | FCR_TX_TRIG_HF | FCR_RX_TRIG_1CH;
    pUartAdapter->IntEnReg = IER_ELSI;  
    // Eable Rx data ready & Line Status interrupt
    pUartAdapter->IntEnReg |= (IER_ELSI | IER_ERBFI);

    ComIrqBkUp = HAL_UART_READ32(UART_INTERRUPT_EN_REG_OFF);
    HAL_UART_WRITE32(UART_INTERRUPT_EN_REG_OFF, 0); // disable all IRQ

    LOC_UART_FCTRL(OFF);
    LOC_UART_FCTRL(ON);
    ACTCK_LOG_UART_CCTRL(ON);

    /*
        Interrupt enable Register
        7: THRE Interrupt Mode Enable
        2: Enable Receiver Line Status Interrupt
        1: Enable Transmit Holding Register Empty Interrupt
        0: Enable Received Data Available Interrupt
        */
    // disable all interrupts
    HAL_UART_WRITE32(UART_INTERRUPT_EN_REG_OFF, 0);

    /*
        Line Control Register
        7:   DLAB, enable reading and writing DLL and DLH register, and must be cleared after
        initial baud rate setup
        3:   PEN, parity enable/disable
        2:   STOP, stop bit
        1:0  DLS, data length
        */

    // set DLAB bit to 1
    HAL_UART_WRITE32(UART_LINE_CTL_REG_OFF, 0x80);

    // set up buad rate division 
    
#if CONFIG_CHIP_A_CUT
    SysClock = (StartupHalGetCpuClk()>>2);
#else 
    SysClock = (HalGetCpuClk()>>2);
#endif
    SampleRate = (16 * (pUartAdapter->BaudRate));
    
    Divisor= SysClock/SampleRate;
    
    Remaind = ((SysClock*10)/SampleRate) - (Divisor*10);
    
    if (Remaind>4) {
        Divisor++;
    }  

    Dll = Divisor & 0xff;
    Dlh = (Divisor & 0xff00)>>8;
    HAL_UART_WRITE32(UART_DLL_OFF, Dll);
    HAL_UART_WRITE32(UART_DLH_OFF, Dlh);

    // clear DLAB bit 
    HAL_UART_WRITE32(UART_LINE_CTL_REG_OFF, 0);

    // set data format
    SetData = pUartAdapter->Parity | pUartAdapter->Stop | pUartAdapter->DataLength;
    HAL_UART_WRITE32(UART_LINE_CTL_REG_OFF, SetData);

    /* FIFO Control Register
        7:6  level of receive data available interrupt
        5:4  level of TX empty trigger
        2    XMIT FIFO reset
        1    RCVR FIFO reset
        0    FIFO enable/disable
        */
    // FIFO setting, enable FIFO and set trigger level (2 less than full when receive
    // and empty when transfer 
    HAL_UART_WRITE32(UART_FIFO_CTL_REG_OFF, pUartAdapter->FIFOControl);

    /*
        Interrupt Enable Register
        7: THRE Interrupt Mode enable
        2: Enable Receiver Line status Interrupt
        1: Enable Transmit Holding register empty INT32
        0: Enable received data available interrupt
        */
    HAL_UART_WRITE32(UART_INTERRUPT_EN_REG_OFF, pUartAdapter->IntEnReg);

    // Register IRQ
    pIrqHandle = &pUartAdapter->IrqHandle;

    pIrqHandle->Data = (u32)pUartAdapter;
    pIrqHandle->IrqNum = UART_LOG_IRQ;
    pIrqHandle->IrqFun = (IRQ_FUN) XMLogUartIrqHandle;
    pIrqHandle->Priority = 14;

    // Re-Register IRQ
    InterruptUnRegister(pIrqHandle); // un_register old IRQ first
    InterruptRegister(pIrqHandle);
    InterruptEn(pIrqHandle);

    XmRxBufRst();

    // turn off all debug message
    ConfigDebugErrBkUp = ConfigDebugErr;
    ConfigDebugWarnBkUp = ConfigDebugWarn;
    ConfigDebugInfoBkUp = ConfigDebugInfo;
    CfgSysDebugErrBkUp = CfgSysDebugErr;
    CfgSysDebugInfoBkUp = CfgSysDebugInfo;
    CfgSysDebugWarnBkUp = CfgSysDebugWarn;

    ConfigDebugErr = 0;
    ConfigDebugWarn = 0;
    ConfigDebugInfo = 0;
    CfgSysDebugErr = 0;
    CfgSysDebugInfo = 0;
    CfgSysDebugWarn = 0;    

    return;
}

FWU_TEXT_SECTION
void xmodem_loguart_func_hook(XMODEM_COM_PORT *pXComPort)
{
    pXComPort->put = xmodem_loguart_putc;
    pXComPort->poll = (char(*)(void))XmRxBufPoll;
    pXComPort->get = (char(*)(void))_XmRxBufGet;
}

#endif

FWU_TEXT_SECTION
void xmodem_loguart_deinit(void) 
{
    // recovery debug message
    ConfigDebugErr = ConfigDebugErrBkUp;
    ConfigDebugWarn = ConfigDebugWarnBkUp;
    ConfigDebugInfo = ConfigDebugInfoBkUp;
    CfgSysDebugErr = CfgSysDebugErrBkUp;
    CfgSysDebugInfo = CfgSysDebugInfoBkUp;
    CfgSysDebugWarn = CfgSysDebugWarnBkUp;

    // Re-store IRQ setting
    HAL_UART_WRITE32(UART_INTERRUPT_EN_REG_OFF, ComIrqBkUp);
    CONSOLE_8195A();    
}

