#include "ota.h"
#include "stm32l4.h"
#include "manifest.h"
#include "log.h"
#include "aes_gcm.h"
#include <string.h>

#define UART_CHUNK_SIZE 256

static bool uart_receive_manifest(FirmwareManifest *manifest) {
    if (HAL_UART_Receive(&huart2, (uint8_t*)manifest, sizeof(FirmwareManifest), HAL_MAX_DELAY) != HAL_OK) {
        log_error("[OTA] Failed to receive manifest\n");
        return false;
    }
    log_info("[OTA] Manifest: version=%u, size=%u, slot=0x%08X\n",
             manifest->version, manifest->size, manifest->slot_address);
    return true;
}

static bool uart_receive_chunk(uint8_t *buf, size_t len) {
    if (HAL_UART_Receive(&huart2, buf, len, HAL_MAX_DELAY) != HAL_OK) {
        log_error("[OTA] Failed to receive chunk\n");
        return false;
    }
    return true;
}

bool ota_perform_update(void) {
    FirmwareManifest manifest;
    if (!uart_receive_manifest(&manifest)) return false;

    uint32_t active_version = platform_read_image_version(platform_read_active_slot_pointer());
    if (manifest.version <= active_version) {
        log_info("[OTA] No new firmware available\n");
        return true;
    }

    log_info("[OTA] OTA start: %u bytes\n", manifest.size);

    uint8_t buffer[UART_CHUNK_SIZE];
    uint32_t written = 0;

    platform_flash_erase(manifest.slot_address, manifest.size);

    uint8_t aes_key[32];
    uint8_t aes_iv[12];
    platform_get_aes_key(aes_key, sizeof(aes_key));
    aes_gcm_init(aes_key, sizeof(aes_key), aes_iv, sizeof(aes_iv));

    while (written < manifest.size) {
        uint32_t chunk_size = (manifest.size - written) > UART_CHUNK_SIZE ? UART_CHUNK_SIZE : (manifest.size - written);
        if (!uart_receive_chunk(buffer, chunk_size)) return false;

        uint8_t decrypted[UART_CHUNK_SIZE];
        aes_gcm_decrypt_update(buffer, chunk_size, decrypted);

        if (platform_flash_write(manifest.slot_address + written, decrypted, chunk_size) != 0) {
            log_error("[OTA] Flash write failed at %u\n", written);
            return false;
        }
        written += chunk_size;
        log_info("[OTA] Written %u/%u bytes\n", written, manifest.size);
    }

    aes_gcm_decrypt_final();

    log_info("[OTA] Verifying SHA256...\n");
    uint8_t hash[32];
    if (!platform_compute_sha256_on_flash(manifest.slot_address, 0, hash)) {
        log_error("[OTA] SHA256 verification failed\n");
        return false;
    }

    platform_atomic_swap_slots(manifest.slot_address);
    log_info("[OTA] OTA completed successfully\n");
    return true;
}
