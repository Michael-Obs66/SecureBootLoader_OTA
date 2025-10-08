#ifndef STM32L4_H
#define STM32L4_H
#include <stdint.h>
#include <stdbool.h>
#include "stm32l4xx_hal.h"

extern UART_HandleTypeDef huart2;

void platform_init_clocks(void);
void platform_init_console(void);
void platform_log(const char *fmt, ...);
void platform_sleep(void);
void platform_deinit_for_jump(void);
void platform_jump_to_address(uint32_t addr);
void platform_enter_safe_mode(void);

int platform_flash_read(uint32_t addr, void *buf, uint32_t len);
int platform_flash_write(uint32_t addr, const void *buf, uint32_t len);
int platform_flash_erase(uint32_t addr, uint32_t size);

uint32_t platform_read_active_slot_pointer(void);
uint32_t platform_read_image_version(uint32_t slot_addr);
void platform_mark_last_boot_success(uint32_t slot);
int platform_atomic_swap_slots(uint32_t new_active_slot);

bool platform_crypto_hw_init(void);
bool platform_get_aes_key(uint8_t *buf, size_t len);
bool platform_compute_sha256_on_flash(uint32_t base, uint32_t offset, uint8_t *out32);

#endif
