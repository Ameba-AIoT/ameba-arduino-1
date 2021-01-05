#include <stdbool.h>
#include <stdint.h>

/**
 * @breif Setup the FPB to redirect a long call (i.e. call new_func(...) instead of old_func(...)).
 *
 * WARNING: This function will not check vaildation of reg_index and itr_index, please read param description carefully.
 * @param[in] uint32_t address of old function
 * @param[in] uint32_t address of new function
 * @param[in] uint8_t  reg_index must be 0~4, 5 is for the case that address not align to 4
 * @param[in] uint8_t  itr_index must be 6 or 7
 * @return    int    0 if function succeeded, other if a failure occured.
 */
int fpb_redirect_long_call( uint32_t instr_addr, uint32_t target_addr,
							uint8_t  reg_index, uint8_t itr_index);
			
/**
 * @brief Enable FPB patch fuction.
 *
 */
void fpb_control_enable(void);
void fpb_control_disable(void);



