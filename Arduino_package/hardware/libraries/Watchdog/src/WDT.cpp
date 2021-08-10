#include "WDT.h"

#define ENABLE 1
#define DISABLE 0

WDT::WDT(){};
WDT::~WDT(){};

/**
 * @brief   Initializes the watch dog, include time setting, mode register
 * @param   timeout_ms: the watch-dog timer timeout value, in ms.
 *           default action of timeout is to reset the whole system.
 * @retval none        
 */
void WDT::InitWatchdog(uint32_t timeout_ms) {
#if 0
    WDG_InitTypeDef WDG_InitStruct;
    u32 CountProcess;
    u32 DivFacProcess;

    WDG_Scalar(timeout_ms, &CountProcess, &DivFacProcess);

    WDG_InitStruct.CountProcess = CountProcess;
    WDG_InitStruct.DivFacProcess = DivFacProcess;

    WDG_Init(&WDG_InitStruct);
#else
    watchdog_init(timeout_ms); // setup 5s watchdog
#endif
}

/**
 * @brief   Start the watchdog counting
 * @param   None
 * @retval none       
 */
void WDT::StartWatchdog(void) {
#if 0
    WDG_Cmd(ENABLE);  // WDG_Cmd used to disable or enable watchdog
#else
    watchdog_start();
#endif
}

/**
 * @brief   Stop the watchdog counting
 * @param   None
 * @retval none       
 */
void WDT::StopWatchdog(void) {
#if 0
    WDG_Cmd(DISABLE);  // WDG_Cmd used to disable or enable watchdog
#else
    watchdog_irq_init(NULL, 0);  //disable second interrupt 
    watchdog_stop();
#endif
}

/**
 * @brief   Refresh the watchdog counting to prevent WDT timeout
 * @param   None
 * @retval none          
 */
void WDT::RefreshWatchdog(void) {
#if 0
    WDG_Refresh();
#else
    watchdog_refresh();
#endif
}

/**
 * @brief   Switch the watchdog timer to interrupt mode and
 *           register a watchdog timer timeout interrupt handler.
 *           The interrupt handler will be called when the watch-dog 
 *           timer is timeout.
 * @param   handler: the callback function for WDT timeout interrupt.
 * @param   id: the parameter for the callback function
 * @retval none           
 */
void WDT::InitWatchdogIRQ(wdt_irq_handler handler, uint32_t id) {
#if 0
    WDG_IrqInit((VOID*)handler, (u32)id);
#else
    watchdog_irq_init(handler, id);
#endif
}
