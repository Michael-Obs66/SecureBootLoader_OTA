#include "aes_gcm.h"
#include "stm32l4xx_hal_cryp.h"

static CRYP_HandleTypeDef hcryp;
static uint8_t aes_key[32];
static uint8_t aes_iv[12];

bool aes_gcm_init(const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len){
    if(key_len!=32 || iv_len!=12) return false;
    memcpy(aes_key,key,32);
    memcpy(aes_iv,iv,12);

    __HAL_RCC_CRYP_CLK_ENABLE();
    hcryp.Instance=CRYP;
    hcryp.Init.DataType=CRYP_DATATYPE_8B;
    return HAL_CRYP_Init(&hcryp)==HAL_OK;
}

bool aes_gcm_decrypt_update(const uint8_t *in, size_t in_len, uint8_t *out){
    return HAL_CRYP_AESGCM_Decrypt(&hcryp,in_len,(uint8_t*)in,out,aes_iv,sizeof(aes_iv),NULL,0,1000)==HAL_OK;
}

bool aes_gcm_decrypt_final(void){ HAL_CRYP_DeInit(&hcryp); return true; }
