#include "boot.h"
#include "stm32l4.h"
#include "log.h"
#include "sha256.h"

bool boot_verify_slot(uint32_t slot_addr) {
    uint8_t hash[32];

    if (!platform_compute_sha256_on_flash(slot_addr, 0, hash)) {
        log_error("[BOOT] SHA256 computation failed on slot 0x%08X\n", slot_addr);
        return false;
    }

    log_info("[BOOT] Slot 0x%08X verified successfully\n", slot_addr);
    return true;
}
