#include "stm32l4.h"
#include "boot.h"
#include "ota.h"
#include "log.h"

int main(void) {
    platform_init_clocks();
    platform_init_console();
    log_info("[BOOT] Bootloader started\n");

    if (!platform_crypto_hw_init()) {
        log_error("[BOOT] Crypto hardware init failed!\n");
        platform_enter_safe_mode();
    }

    uint32_t active_slot = platform_read_active_slot_pointer();

    if (!boot_verify_slot(active_slot)) {
        log_info("[BOOT] Active firmware invalid, performing OTA...\n");
        if (!ota_perform_update()) {
            log_error("[BOOT] OTA failed, entering safe mode\n");
            platform_enter_safe_mode();
        }
    }

    platform_mark_last_boot_success(active_slot);
    log_info("[BOOT] Jumping to firmware at 0x%08X\n", active_slot);
    platform_deinit_for_jump();
    platform_jump_to_address(active_slot);

    platform_enter_safe_mode(); // Should never reach here
}
