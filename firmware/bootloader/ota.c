#include "ota.h"
#include "stm32l4.h"
#include "manifest.h"
#include "log.h"
#include "aes_gcm.h"
#include "sha256.h"
#include <string.h>

#define UART_CHUNK_SIZE 256
#define ACK  0x79
#define NACK 0x1F

static bool uart_receive(uint8_t *buf, size_t len) {
    if (HAL_UART_Receive(&huart2, buf, len, HAL_MAX_DELAY) != HAL_OK) {
        log_error("[OTA] UART receive failed\n");
        return false;
    }
    return true;
}

static void uart_send_ack(void) { uint8_t b = ACK; HAL_UART_Transmit(&huart2, &b, 1, HAL_MAX_DELAY); }
static void uart_send_nack(void) { uint8_t b = NACK; HAL_UART_Transmit(&huart2, &b, 1, HAL_MAX_DELAY); }

bool ota_perform_update(void) {
    FirmwareManifest manifest;
    uint8_t buffer[UART_CHUNK_SIZE];
    uint32_t written = 0;

    // Step 1: Receive manifest
    if (!uart_receive((uint8_t*)&manifest, sizeof(FirmwareManifest))) return false;
    uart_send_ack();
    log_info("[OTA] Received manifest: version=%u, size=%u\n", manifest.version, manifest.size);

    // Step 2: Check if update is needed
    uint32_t active_version = platform_read_image_version(platform_read_active_slot_pointer());
    if (manifest.version <= active_version) {
        log_info("[OTA] Firmware up-to-date\n");
        return true;
    }

    // Step 3: Erase flash for new slot
    if (platform_flash_erase(manifest.slot_address, manifest.size) != 0) {
        log_error("[OTA] Flash erase failed\n");
        return false;
    }

    // Step 4: AES-256 GCM init
    uint8_t aes_key[32], aes_iv[12];
    platform_get_aes_key(aes_key, sizeof(aes_key));
    aes_gcm_init(aes_key, sizeof(aes_key), aes_iv, sizeof(aes_iv));

    // Step 5: Receive firmware in chunks
    while (written < manifest.size) {
        uint32_t chunk_size = (manifest.size - written) > UART_CHUNK_SIZE ? UART_CHUNK_SIZE : (manifest.size - written);
        if (!uart_receive(buffer, chunk_size)) {
            uart_send_nack();
            return false;
        }

        uint8_t decrypted[UART_CHUNK_SIZE];
        if (!aes_gcm_decrypt_update(buffer, chunk_size, decrypted)) {
            uart_send_nack();
            return false;
        }

        if (platform_flash_write(manifest.slot_address + written, decrypted, chunk_size) != 0) {
            uart_send_nack();
            log_error("[OTA] Flash write failed at %u\n", written);
            return false;
        }

        written += chunk_size;
        uart_send_ack();
        log_info("[OTA] Written %u/%u bytes\n", written, manifest.size);
    }

    aes_gcm_decrypt_final();

    // Step 6: Receive firmware hash from host for verification
    uint8_t host_hash[32];
    if (!uart_receive(host_hash, sizeof(host_hash))) {
        uart_send_nack();
        return false;
    }

    uint8_t device_hash[32];
    if (!platform_compute_sha256_on_flash(manifest.slot_address, 0, device_hash)) {
        uart_send_nack();
        return false;
    }

    if (memcmp(host_hash, device_hash, 32) != 0) {
        uart_send_nack();
        log_error("[OTA] SHA256 mismatch, OTA failed\n");
        return false;
    }

    uart_send_ack();
    log_info("[OTA] Firmware verified successfully, swapping slots\n");

    platform_atomic_swap_slots(manifest.slot_address);
    return true;
}
