#ifndef __WDT_H__
#define __WDT_H__

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "device.h"
#include "diag.h"
#include "main.h"
#include "wdt_api.h"

#ifdef __cplusplus
}
#endif


class WDT {
    public:
        WDT(void);
        ~WDT(void);

        void InitWatchdog(uint32_t timeout_ms);
        void StartWatchdog(void);
        void StopWatchdog(void);
        void RefreshWatchdog(void);
        void InitWatchdogIRQ(wdt_irq_handler handler, uint32_t id);
};
#endif
