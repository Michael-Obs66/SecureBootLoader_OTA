#ifndef AES_GCM_H
#define AES_GCM_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool aes_gcm_init(const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len);
bool aes_gcm_decrypt_update(const uint8_t *in, size_t in_len, uint8_t *out);
bool aes_gcm_decrypt_final(void);

#endif
