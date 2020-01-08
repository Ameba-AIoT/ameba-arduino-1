#ifndef _POWER_MANAGEMENT_H_
#define _POWER_MANAGEMENT_H_

#include <inttypes.h>

/** 
 * @class PowerManagementClass PowerManagement.h 
 * @brief Power management in Ameba
 */
class PowerManagementClass {
public:

    /**
     * @brief Allow OS automatically save power while idle
     *
     * As OS consider it would idle for more than 2s, it will invoke system suspend.
     * If wlan is associated with AP, than it will under asslociated idle state.
     */
    static void sleep();

    /**
     * @brief Disallow OS automatically save power while idle
     */
    static void active();

    /**
     * @brief Reserved PLL while sleep
     * 
     * Reserve PLL would keep FIFO of peripherals (Ex. UART) but cost more power (around 5mA).
     * If we don't reserve PLL, it saves more power but we might missing data because FIFO is turned of this way.
     *
     * @param[in] reserve true for reserved, false for non-reserved
     */
    static void setPllReserved(bool reserve);

    /**
     * @brief Enter deepsleep immediately
     *
     * Invoke deepsleep would make system enter deepsleep state immediately.
     * It's the state that saves most power.
     * As it wakeup from deepsleep, the system would behave just like reboot.
     *
     * @param[in] duration_ms wakeup after specific time in unit of millisecond
     */
    static void deepsleep(uint32_t duration_ms);

    /**
     * @brief Check if system is allowed enter any power save state
     *
     * The safe pin is designed as safe lock. (By default RTL8195A use pin 18, and RTL8710 use pin 15)
     * If safe pin is HIGH, then we prevent Ameba enter any power save state.\n\n
     * Under any power save state, we are not able to flash image to Ameba.
     * Thus if user misuse deepsleep and make Ameba enter deepsleep immediately after boot up,
     * then he would find it's hard to flash image.
     * In this case, he can pull up pin 18.
     *
     * @return true if system not allowed enter any power save state, and false vise versa
     */
    static bool safeLock();

    /*
     * @brief Change safe pin
     *
     * Change the default safe pin. It needs configure every time in boot up.
     */
    static bool setSafeLockPin(int ulPin);

    /**
     * @brief Reboot system
     *
     * Reboot system in soft way. Some registers is not powered off in this case, but mostly we could regard this as reboot.
     */
    static void softReset();

private:
    static bool reservePLL;
    static int safeLockPin;
};

extern PowerManagementClass PowerManagement;

#endif