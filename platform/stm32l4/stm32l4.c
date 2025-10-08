#include "stm32l4.h"
#include "log.h"
#include <string.h>
#include "aes_gcm.h"

UART_HandleTypeDef huart2;

void platform_init_clocks(void) { HAL_Init(); SystemClock_Config(); }
void platform_init_console(void) { MX_USART2_UART_Init(); }
void platform_log(const char *fmt, ...) { va_list args; va_start(args, fmt); char buf[256]; vsnprintf(buf, sizeof(buf), fmt, args); va_end(args); HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY); }
void platform_sleep(void) { __WFI(); }
void platform_deinit_for_jump(void) { __disable_irq(); }
void platform_jump_to_address(uint32_t addr) { typedef void (*app_t)(void); uint32_t jump = *(uint32_t*)(addr+4); __set_MSP(*(uint32_t*)addr); ((app_t)jump)(); }
void platform_enter_safe_mode(void) { while(1); }

int platform_flash_read(uint32_t addr, void *buf, uint32_t len) { memcpy(buf,(void*)addr,len); return 0; }
int platform_flash_write(uint32_t addr, const void *buf, uint32_t len) { HAL_FLASH_Unlock(); for(uint32_t i=0;i<len;i+=8){ uint64_t d; memcpy(&d,(uint8_t*)buf+i,8); if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,addr+i,d)!=HAL_OK){ HAL_FLASH_Lock(); return -1; } } HAL_FLASH_Lock(); return 0; }
int platform_flash_erase(uint32_t addr, uint32_t size){ FLASH_EraseInitTypeDef erase; uint32_t err; erase.TypeErase=FLASH_TYPEERASE_PAGES; erase.PageAddress=addr; erase.NbPages=size/FLASH_PAGE_SIZE; HAL_FLASH_Unlock(); if(HAL_FLASHEx_Erase(&erase,&err)!=HAL_OK){ HAL_FLASH_Lock(); return -1;} HAL_FLASH_Lock(); return 0;}

uint32_t platform_read_active_slot_pointer(void){ return 0x08020000; } 
uint32_t platform_read_image_version(uint32_t slot_addr){ return *(uint32_t*)slot_addr; }
void platform_mark_last_boot_success(uint32_t slot){}
int platform_atomic_swap_slots(uint32_t new_active_slot){ return 0; }

bool platform_crypto_hw_init(void){ __HAL_RCC_CRYP_CLK_ENABLE(); return true; }
bool platform_get_aes_key(uint8_t *buf, size_t len){ memset(buf,0xAA,len); return true; }
bool platform_compute_sha256_on_flash(uint32_t base, uint32_t offset, uint8_t *out32){ memset(out32,0xBB,32); return true; }
