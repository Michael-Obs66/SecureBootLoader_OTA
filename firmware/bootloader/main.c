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

    uint32_t active_slot_addr = platform_read_active_slot_address();

    // 1) Verify metadata first (version & signature) without modifying any slot
    if (!boot_verify_slot(active_slot_addr)) {
        log_info("[BOOT] Active firmware invalid, performing OTA...\n");
        if (!ota_perform_update()) {
            log_error("[BOOT] OTA failed, entering safe mode\n");
            platform_enter_safe_mode();
        }
        // After OTA, re-read active slot address if OTA may change it
        active_slot_addr = platform_read_active_slot_address();
        if (!boot_verify_slot(active_slot_addr)) {
            log_error("[BOOT] Verified OTA image still invalid\n");
            platform_enter_safe_mode();
        }
    }

    // 2) set boot_attempted flag BEFORE jump
    platform_set_boot_attempted_flag(active_slot_addr);

    // 3) prepare CPU & jump
    platform_deinit_for_jump();
    platform_jump_to_address(active_slot_addr);

    // If returns -> either firmware crashed immediately, or jump failed
    log_error("[BOOT] Jump failed or firmware crashed. Clearing boot_attempted and entering safe mode\n");
    platform_clear_boot_attempted_flag();
    platform_enter_safe_mode();
}
